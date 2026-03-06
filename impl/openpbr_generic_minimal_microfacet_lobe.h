/***************************************************************************
 * Copyright 2026 Adobe
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

// Important usage note:
// OPENPBR_MICROFACET_DISTRIBUTION_TYPE and OPENPBR_REFLECTION_COEFFICIENT_TYPE must be defined before including this file.
// The name of this lobe must also be defined as OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE.
#ifndef OPENPBR_MICROFACET_DISTRIBUTION_TYPE
#error "OPENPBR_MICROFACET_DISTRIBUTION_TYPE must be defined before including generic_minimal_microfacet_lobe.h"
#endif
#ifndef OPENPBR_REFLECTION_COEFFICIENT_TYPE
#error "OPENPBR_REFLECTION_COEFFICIENT_TYPE must be defined before including generic_minimal_microfacet_lobe.h"
#endif
#ifndef OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE
#error "OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE must be defined before including generic_minimal_microfacet_lobe.h"
#endif

#include "../openpbr_diffuse_specular.h"
#include "openpbr_lobe_utils.h"

/////////////////////////////////////
// MinimalMicrofacetReflectionLobe //
/////////////////////////////////////

struct OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE
{
    vec3 normal_ff;
    OPENPBR_MICROFACET_DISTRIBUTION_TYPE microfacet_distr;
    OPENPBR_REFLECTION_COEFFICIENT_TYPE refl_trans_coeff;
};

void openpbr_initialize_lobe(ADDRESS_SPACE_THREAD INOUT(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe,
                             const vec3 normal_ff,
                             const OPENPBR_MICROFACET_DISTRIBUTION_TYPE microfacet_distr,
                             const OPENPBR_REFLECTION_COEFFICIENT_TYPE refl_trans_coeff)
{
    lobe = MAKE_STRUCT_3(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE, normal_ff, microfacet_distr, refl_trans_coeff);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe)
{
    return OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    const float idotn = dot(lobe.normal_ff, view_direction);
    const float odotn = dot(lobe.normal_ff, light_direction);

    if (idotn * odotn > 0.0f)  // reflection
    {
        const vec3 half_vector = openpbr_fast_normalize(view_direction + light_direction);

        const float idoth = dot(view_direction, half_vector);

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        const float other_terms = 1.0f / (4.0f * idotn);
        const vec3 F = openpbr_reflection_coefficient(lobe.refl_trans_coeff, abs(idoth));

        const vec3 D_other_terms_F = D * other_terms * F;

        const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

        return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
    }
    else  // not reflection
    {
        return openpbr_make_zero_diffuse_specular();
    }
}

float openpbr_calculate_lobe_pdf(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    const float idotn = dot(lobe.normal_ff, view_direction);
    const float odotn = dot(lobe.normal_ff, light_direction);

    if (idotn * odotn > 0.0f)  // reflection
    {
        const vec3 half_vector = openpbr_fast_normalize(view_direction + light_direction);

        const float idoth = dot(view_direction, half_vector);
        const float odoth = dot(light_direction, half_vector);

        // Return invalid PDF if half vector rounding error would result in negative PDF
        if (idoth * odoth < 0.0f)
            return 0.0f;

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        const float other_terms = idoth / (4.0f * odoth * idotn);

        const float D_other_terms = D * other_terms;

        const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

        return G_masking * D_other_terms;
    }
    else  // not reflection
    {
        return 0.0f;
    }
}

bool openpbr_sample_lobe(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         ADDRESS_SPACE_THREAD OUT(vec3) light_direction,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_DiffuseSpecular) weight,
                         ADDRESS_SPACE_THREAD OUT(float) pdf,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const float idotn = dot(view_direction, lobe.normal_ff);

    // Choose microfacet normal_ff (in world space).
    const vec3 half_vector = openpbr_sample_ggx_smith_vndf(lobe.microfacet_distr, view_direction, lobe.normal_ff, SWIZZLE(rand, xy));

    const float idoth = dot(view_direction, half_vector);
    // Sampled microfacet normal is expected to be in incident hemisphere,
    // but occasionally is not due to rounding error.
    if (idoth < 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    // The light direction is normalized by construction, but can be significantly off due to rounding error.
    light_direction = openpbr_fast_normalize(-view_direction + half_vector * 2.0f * idoth);

    // Check if reflection is on correct side of surface.
    const float odotn = dot(lobe.normal_ff, light_direction);
    if (idotn * odotn <= 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    const float odoth = dot(light_direction, half_vector);

    const vec3 F_refl = openpbr_reflection_coefficient(lobe.refl_trans_coeff, idoth);
    const float G_shadowing = openpbr_eval_smith_g1(lobe.microfacet_distr, light_direction, odotn);
    weight = openpbr_make_diffuse_specular_from_specular(F_refl * G_shadowing);

    const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
    if (D <= 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
    pdf = D * G_masking * idoth / (4.0f * odoth * idotn);

    // It's very important to return the correct type to ensure correct treatment by outside systems.
    sampled_type = openpbr_get_lobe_type(lobe);

    return true;
}

float openpbr_estimate_lobe_contribution(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    const float idotn = dot(view_direction, lobe.normal_ff);
    // Here we assume that the microfacet distribution is a GGX one and calculate the estimated weight accordingly.
    return openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(
        lobe.refl_trans_coeff, path_throughput, idotn, openpbr_get_isotropic_alpha(lobe.microfacet_distr));
}
