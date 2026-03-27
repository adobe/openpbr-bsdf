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

#ifndef OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H
#define OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H

#include "openpbr_reflection_transmission_coefficient.h"

#include "../openpbr_constants.h"
#include "../openpbr_diffuse_specular.h"
#include "openpbr_dispersion_utils.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_specialization_constants.h"
#include "openpbr_vndf_microfacet_distribution.h"

///////////////////////////////////////////////////////
// ComprehensiveMicrofacetReflectionTransmissionLobe //
///////////////////////////////////////////////////////

struct OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe
{
    vec3 normal_ff;
    OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr;
    OpenPBR_ComprehensiveReflectionTransmissionCoefficient refl_trans_coeff;
    vec3 eta_t_over_eta_i;
    vec3 path_throughput;
};

vec3 openpbr_channel_probabilities(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe)
{
    return lobe.path_throughput / openpbr_sum(lobe.path_throughput);
}

bool openpbr_validate_half_vector_for_transmission(
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
    const vec3 half_vector)
{
    // The computed half vector should point into the less dense medium.
    // If the face-forwarded normal points into the less dense medium, half_vector is valid if it faces forward into the less dense medium.
    // If the face-forwarded normal points into the denser medium, half_vector is valid if it faces backward into the less dense medium.
    return (lobe.eta_t_over_eta_i.r >= 1.0f && dot(lobe.normal_ff, half_vector) > 0.0f) ||
           (lobe.eta_t_over_eta_i.r < 1.0f && dot(lobe.normal_ff, half_vector) < 0.0f);
}

void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                             const vec3 normal_ff,
                             const OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr,
                             const OpenPBR_ComprehensiveReflectionTransmissionCoefficient refl_trans_coeff,
                             const vec3 eta_t_over_eta_i,
                             const vec3 path_throughput)
{
    OPENPBR_ASSERT(openpbr_is_normalized(normal_ff), "The input normal is assumed to be normalized");

    lobe = OPENPBR_MAKE_STRUCT_5(
        OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe, normal_ff, microfacet_distr, refl_trans_coeff, eta_t_over_eta_i, path_throughput);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe)
                                               lobe)
{
    return OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency)
               ? OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection | OpenPBR_BsdfLobeTypeTransmission
               : OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
}

OpenPBR_DiffuseSpecular
openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
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
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && idotn * odotn < 0.0f)  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            vec3 D_other_terms_F;
            for (int channel = 0; channel < OpenPBR_NumRgbChannels; ++channel)
            {
                const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i[channel]);
                if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                {
                    D_other_terms_F[channel] = 0.0f;  // invalid transmission
                    continue;
                }

                const float idoth = dot(view_direction, half_vector);
                const float odoth = dot(light_direction, half_vector);
                if (idoth * odoth >= 0.0f)
                {
                    D_other_terms_F[channel] = 0.0f;  // invalid transmission
                    continue;
                }

                const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
                const float other_terms = openpbr_square(lobe.eta_t_over_eta_i[channel]) * abs(idoth) * abs(odoth) /
                                          (openpbr_square(idoth + lobe.eta_t_over_eta_i[channel] * odoth) * idotn);
                const float F = openpbr_transmission_coefficient(lobe.refl_trans_coeff, abs(idoth))[channel];

                D_other_terms_F[channel] = D * other_terms * F;
            }

            const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

            return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
        }
        else
        {
            const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i.r);
            if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                return openpbr_make_zero_diffuse_specular();  // invalid transmission

            const float idoth = dot(view_direction, half_vector);
            const float odoth = dot(light_direction, half_vector);
            if (idoth * odoth >= 0.0f)
                return openpbr_make_zero_diffuse_specular();  // invalid transmission

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            const float other_terms =
                openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);
            const vec3 F = openpbr_transmission_coefficient(lobe.refl_trans_coeff, abs(idoth));

            const vec3 D_other_terms_F = D * other_terms * F;

            const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

            return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
        }
    }
    else  // neither reflection nor transmission
        return openpbr_make_zero_diffuse_specular();

    // If cosine term needs to be excluded, code like this could be used:
    //     if (!IncludeCosineTerm)
    //         result /= odotn;
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
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
        const float F_prob = openpbr_reflection_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

        const float F_prob_D_other_terms = F_prob * D * other_terms;

        const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

        return G_masking * F_prob_D_other_terms;
    }
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && idotn * odotn < 0.0f)  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            vec3 F_prob_D_other_terms;
            for (int channel = 0; channel < OpenPBR_NumRgbChannels; ++channel)
            {
                const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i[channel]);
                if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                {
                    F_prob_D_other_terms[channel] = 0.0f;  // invalid transmission
                    continue;
                }

                const float idoth = dot(view_direction, half_vector);
                const float odoth = dot(light_direction, half_vector);
                if (idoth * odoth >= 0.0f)
                {
                    F_prob_D_other_terms[channel] = 0.0f;  // invalid transmission
                    continue;
                }

                const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
                const float other_terms = openpbr_square(lobe.eta_t_over_eta_i[channel]) * abs(idoth) * abs(odoth) /
                                          (openpbr_square(idoth + lobe.eta_t_over_eta_i[channel] * odoth) * idotn);
                const float F_prob = openpbr_transmission_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

                F_prob_D_other_terms[channel] = F_prob * D * other_terms;
            }

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

            return G_masking * dot(F_prob_D_other_terms, openpbr_channel_probabilities(lobe));
        }
        else
        {
            const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i.r);
            if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                return 0.0f;  // invalid transmission

            const float idoth = dot(view_direction, half_vector);
            const float odoth = dot(light_direction, half_vector);
            if (idoth * odoth >= 0.0f)
                return 0.0f;  // invalid transmission

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            const float other_terms =
                openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);
            const float F_prob = openpbr_transmission_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

            const float F_prob_D_other_terms = F_prob * D * other_terms;

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

            return G_masking * F_prob_D_other_terms;
        }
    }
    else  // neither reflection nor transmission
        return 0.0f;
}

bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const float idotn = dot(view_direction, lobe.normal_ff);

    // Choose microfacet normal_ff (in the caller's coordinate space).
    const vec3 half_vector = openpbr_sample_ggx_smith_vndf(lobe.microfacet_distr, view_direction, lobe.normal_ff, OPENPBR_SWIZZLE(rand, xy));

    const float idoth = dot(view_direction, half_vector);
    // Sampled microfacet normal is expected to be in incident hemisphere,
    // but occasionally is not due to rounding error.
    if (idoth < 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    // Select reflection or transmission.
    OpenPBR_AllCoefficientsAndProbabilities coeffs_and_probs =
        openpbr_all_coefficients_and_probabilities(lobe.refl_trans_coeff, lobe.path_throughput, idoth);
    const vec3 F_refl = coeffs_and_probs.reflection_coefficient;
    const vec3 F_trans = coeffs_and_probs.transmission_coefficient;
    const float P_refl = coeffs_and_probs.reflection_probability;
    const float P_trans = coeffs_and_probs.transmission_probability;

    if (P_refl + P_trans == 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    if (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) || rand.z < P_refl)  // reflection
    {
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

        // If the input normal and view direction are normalized, this assertion should never fail.
        OPENPBR_ASSERT(odoth >= 0.0f, "odoth is expected to be non-negative");

        const float G_shadowing = openpbr_eval_smith_g1(lobe.microfacet_distr, light_direction, odotn);
        weight = openpbr_make_diffuse_specular_from_specular((F_refl * (1.0f / P_refl)) * G_shadowing);

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        if (D <= 0.0f)
        {
            openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
            return false;
        }

        const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
        pdf = P_refl * D * G_masking * idoth / (4.0f * odoth * idotn);

        // It's very important to return the correct type to ensure correct treatment by outside systems.
        sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;

        return true;
    }
    else  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            const float remapped_rand_z = (rand.z - P_refl) / P_trans;
            int selected_color_channel;
            const vec3 color_channel_probabilities = openpbr_channel_probabilities(lobe);
            if (remapped_rand_z < color_channel_probabilities[0])
                selected_color_channel = 0;
            else if (remapped_rand_z < color_channel_probabilities[0] + color_channel_probabilities[1])
                selected_color_channel = 1;
            else
                // TODO: Address numerical vulnerabilities.
                selected_color_channel = 2;

            if (!openpbr_refract(view_direction, half_vector, idoth, lobe.eta_t_over_eta_i[selected_color_channel], light_direction))
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // Check if transmission is on correct side of surface.
            const float odotn = dot(light_direction, lobe.normal_ff);
            if (idotn * odotn >= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // TODO: Only perform the necessary calculations.
            pdf = openpbr_calculate_lobe_pdf(lobe, view_direction, light_direction);
            if (pdf == 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }
            const OpenPBR_DiffuseSpecular bsdf_cos = openpbr_calculate_lobe_value(lobe, view_direction, light_direction);
            weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);

            // It's very important to return the correct type to ensure correct treatment by outside systems.
            sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeTransmission;

            return true;
        }
        else
        {
            if (!openpbr_refract(view_direction, half_vector, idoth, lobe.eta_t_over_eta_i.r, light_direction))
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // Check if transmission is on correct side of surface.
            const float odotn = dot(light_direction, lobe.normal_ff);
            if (idotn * odotn >= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            const float odoth = dot(light_direction, half_vector);

            const float G_shadowing = openpbr_eval_smith_g1(lobe.microfacet_distr, light_direction, odotn);
            weight = openpbr_make_diffuse_specular_from_specular((F_trans * (1.0f / P_trans)) * G_shadowing);

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            if (D <= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
            pdf = P_trans * D * G_masking * openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) /
                  (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);

            // It's very important to return the correct type to ensure correct treatment by outside systems.
            sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeTransmission;

            return true;
        }
    }
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe)
                                             lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    const float idotn = dot(view_direction, lobe.normal_ff);
    // Here we assume that the microfacet distribution is a GGX one and calculate the estimated weight accordingly.
    return openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(
        lobe.refl_trans_coeff, path_throughput, idotn, openpbr_get_isotropic_alpha(lobe.microfacet_distr));
}

#endif  // !OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H
