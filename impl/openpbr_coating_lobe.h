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

#ifndef OPENPBR_COATING_LOBE_H
#define OPENPBR_COATING_LOBE_H

// Specify the type of the lobe underneath the coating.
#include "openpbr_aggregate_lobe.h"

// Specify the type of lobe representing reflection from the coating.
#include "openpbr_ior_reflection_coefficient.h"
#include "openpbr_microfacet_multiple_scattering_data.h"
#include "openpbr_vndf_microfacet_distributions.h"
#define OPENPBR_MICROFACET_DISTRIBUTION_TYPE OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution
#define OPENPBR_REFLECTION_COEFFICIENT_TYPE OpenPBR_IorReflectionCoefficient
#define OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE                                                                                                         \
    OpenPBR_MinimalMicrofacetReflectionLobe_AnisotropicGGXSmithVNDFMicrofacetDistribution_IorReflectionCoefficient

// Instantiate the generic minimal microfacet lobe template using the above configuration.
#include "openpbr_generic_minimal_microfacet_lobe.h"

// Undefine macros to avoid potential conflicts.
#undef OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE
#undef OPENPBR_REFLECTION_COEFFICIENT_TYPE
#undef OPENPBR_MICROFACET_DISTRIBUTION_TYPE

#include "../openpbr_diffuse_specular.h"
#include "openpbr_lobe_utils.h"

/////////////////
// CoatingLobe //
/////////////////

// Represents a coating layer and whatever is beneath it (the aggregate base lobe in this case).
// Handles energy balance between these layers in a reciprocal way.
// When "inside" is true, represents a coated surface hit from the inside;
// applies a tint but doesn't do much else.
struct OpenPBR_CoatingLobe_AggregateLobe
{
    // Common properties.
    OpenPBR_AggregateLobe base_lobe;
    vec3 normal_ff;
    vec3 tint;
    float presence;
    bool inside;

    // Properties used only when entering the material (not when exiting).
    OpenPBR_MinimalMicrofacetReflectionLobe_AnisotropicGGXSmithVNDFMicrofacetDistribution_IorReflectionCoefficient coat_reflection_lobe;
    float in_reflected;
    vec3 in_base_layer_scale;
};

// Define how light should be tinted as it passes through the coat in one direction.
// Light that enters the coat, reflects from the base layer, and exits the coat will
// be tinted by this function two times, once for each passage.
vec3 openpbr_coat_passage_color_multiplier(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe, const float cosine)
{
    if (cosine > 0.0f && openpbr_min3(lobe.tint) < 1.0f)
    {
        // The user-specified coat color is defined to be the square of the coat transmission color
        // at normal incidence, so taking the square root of the tint produces the coat
        // transmission color at normal incidence (compensated for the presence of the coat).
        const vec3 coat_transmission_at_normal_incidence = sqrt(lobe.tint);
        const float refracted_cosine = sqrt(max(
            0.0f,
            1.0f - (1.0f - openpbr_square(cosine)) / openpbr_square(lobe.coat_reflection_lobe.refl_trans_coeff.eta_t_over_eta_i)));  // Snell's law
        const float distance_scale = 1.0f / refracted_cosine;  // passage distance relative to coat thickness in normal direction
        const vec3 coat_transmission_along_refracted_ray = openpbr_safe_pow(coat_transmission_at_normal_incidence, vec3(distance_scale));
        return mix(vec3(1.0f), coat_transmission_along_refracted_ray, lobe.presence);  // account for fractional coat coverage
    }
    else  // inside or no tint
    {
        return vec3(1.0f);
    }
}

float openpbr_proportion_reflected(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe, const float cosine)
{
    if (cosine > 0.0f)
    {
        const float proportion_reflected_from_full_coat =
            openpbr_compute_dielectric_energy_reflected(lobe.coat_reflection_lobe.refl_trans_coeff.eta_t_over_eta_i,
                                                        openpbr_get_isotropic_alpha(lobe.coat_reflection_lobe.microfacet_distr),
                                                        cosine);
        return proportion_reflected_from_full_coat * lobe.presence;
    }
    else  // inside
    {
        return 0.0f;
    }
}

vec3 openpbr_base_layer_scale_one_side(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                       const float cosine,
                                       const float proportion_reflected_from_this_side)
{
    return openpbr_coat_passage_color_multiplier(lobe, cosine) * (1.0f - proportion_reflected_from_this_side);
}

vec3 openpbr_base_layer_scale_incoming(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe)
{
    return lobe.in_base_layer_scale;
}

vec3 openpbr_base_layer_scale_outgoing(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                       const vec3 light_direction)
{
    const float odotn = dot(light_direction, lobe.normal_ff);
    const float out_reflected = openpbr_proportion_reflected(lobe, odotn);
    return openpbr_base_layer_scale_one_side(lobe, odotn, out_reflected);
}

vec3 openpbr_base_layer_scale_complete(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                       const vec3 light_direction)
{
    const vec3 incoming = lobe.in_base_layer_scale;
    const vec3 outgoing = openpbr_base_layer_scale_outgoing(lobe, light_direction);
    return incoming * outgoing;
}

// Assumes that this is an external hit.
float openpbr_coat_reflection_probability(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                          const vec3 view_direction)
{
    // Here we calculate the expected contribution of the coat reflection lobe relative to the overall lobe
    // and we use that to set the probability of sampling the coat reflection lobe.

    // TODO: Take the real path throughput into account (for both standalone lobes),
    //       either by saving it in the lobe struct or by passing it in.
    OPENPBR_CONSTEXPR_LOCAL vec3 PlaceholderPathThroughput = vec3(1.0f);

    // Here we can use lobe.in_reflected instead of calling estimate_lobe_contribution
    // because we know the inner details of the coat reflection lobe.
    const float coat_reflection_lobe_contribution = lobe.in_reflected;

    const float unscaled_base_lobe_contribution = openpbr_estimate_lobe_contribution(lobe.base_lobe, view_direction, PlaceholderPathThroughput);
    const float base_lobe_scale = openpbr_max3(lobe.in_base_layer_scale);
    const float scaled_base_lobe_contribution = unscaled_base_lobe_contribution * base_lobe_scale;

    return openpbr_safe_divide(coat_reflection_lobe_contribution, coat_reflection_lobe_contribution + scaled_base_lobe_contribution, 0.5f);
}

OpenPBR_DiffuseSpecular openpbr_combine_coat_and_base_evals(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_DiffuseSpecular) eval_coat,
                                                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_DiffuseSpecular) eval_base,
                                                            const vec3 light_direction)
{
    return openpbr_add_diffuse_specular(openpbr_scale_diffuse_specular(eval_coat, lobe.presence),
                                        openpbr_scale_diffuse_specular(eval_base, openpbr_base_layer_scale_complete(lobe, light_direction)));
}

// Assumes that child lobe has already been initialized.
void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const vec3 tint,
                             const float presence,
                             const vec3 extra_base_layer_scale,  // such as darkening due to internal reflections
                             const bool inside)
{
    lobe.normal_ff = normal_ff;
    lobe.tint = tint;
    lobe.presence = presence;
    lobe.inside = inside;

    // Precompute certain properties. The order of these calls is important because
    // they use information in the lobe struct that is set by previous calls.
    const float idotn = dot(view_direction, lobe.normal_ff);
    lobe.in_reflected = openpbr_proportion_reflected(lobe, idotn);
    lobe.in_base_layer_scale = openpbr_base_layer_scale_one_side(lobe, idotn, lobe.in_reflected) *
                               extra_base_layer_scale;  // (We can incorporate the entire extra scale here because it's currently not view-dependent.
                                                        //  This lets us avoid storing it and allows it to be applied to emission as well.)
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe)
{
    if (lobe.inside)
    {
        return openpbr_get_lobe_type(lobe.base_lobe);
    }
    else
    {
        return openpbr_get_lobe_type(lobe.coat_reflection_lobe) | openpbr_get_lobe_type(lobe.base_lobe);
    }
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    if (lobe.inside)
    {
        const float odotn = dot(light_direction, lobe.normal_ff);
        const vec3 base_layer_scale = openpbr_coat_passage_color_multiplier(lobe, -odotn);  // negative to tint only transmission

        return openpbr_scale_diffuse_specular(openpbr_calculate_lobe_value(lobe.base_lobe, view_direction, light_direction), base_layer_scale);
    }
    else
    {
        return openpbr_combine_coat_and_base_evals(lobe,
                                                   openpbr_calculate_lobe_value(lobe.coat_reflection_lobe, view_direction, light_direction),
                                                   openpbr_calculate_lobe_value(lobe.base_lobe, view_direction, light_direction),
                                                   light_direction);
    }
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    if (lobe.inside)
    {
        return openpbr_calculate_lobe_pdf(lobe.base_lobe, view_direction, light_direction);
    }
    else
    {
        return openpbr_combine_coat_and_base_pdfs(openpbr_calculate_lobe_pdf(lobe.coat_reflection_lobe, view_direction, light_direction),
                                                  openpbr_calculate_lobe_pdf(lobe.base_lobe, view_direction, light_direction),
                                                  openpbr_coat_reflection_probability(lobe, view_direction));
    }
}

// Given a view direction, sample a light direction,
// evaluate the throughput and pdf for this direction,
// and return whether a sample was successfully generated.
// Output arguments are only set when the function returns true.
bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    if (lobe.inside)
    {
        const bool success = openpbr_sample_lobe(lobe.base_lobe, rand, view_direction, light_direction, weight, pdf, sampled_type);
        if (success)
        {
            const float odotn = dot(light_direction, lobe.normal_ff);
            const vec3 base_layer_scale = openpbr_coat_passage_color_multiplier(lobe, -odotn);  // negative to tint only transmission

            weight = openpbr_scale_diffuse_specular(weight, base_layer_scale);
        }

        return success;
    }
    else
    {
        bool success;

        const float coat_reflection_prob = openpbr_coat_reflection_probability(lobe, view_direction);

        float rand_x = rand.x;
        if (rand_x < coat_reflection_prob)  // coat
        {
            const float inverse_coat_reflection_prob = 1.0f / coat_reflection_prob;
            rand_x *= inverse_coat_reflection_prob;
            openpbr_clamp_remapped_random_number(rand_x);

            success = openpbr_sample_lobe(
                lobe.coat_reflection_lobe, vec3(rand_x, rand.y, rand.z), view_direction, light_direction, weight, pdf, sampled_type);
            if (success)
            {
                if (!bool(sampled_type & OpenPBR_BsdfLobeTypeSpecular))  // non-delta
                {
                    OpenPBR_DiffuseSpecular bsdf_cos = openpbr_scale_diffuse_specular(weight, pdf);

                    bsdf_cos = openpbr_combine_coat_and_base_evals(
                        lobe, bsdf_cos, openpbr_calculate_lobe_value(lobe.base_lobe, view_direction, light_direction), light_direction);
                    pdf = openpbr_combine_coat_and_base_pdfs(
                        pdf, openpbr_calculate_lobe_pdf(lobe.base_lobe, view_direction, light_direction), coat_reflection_prob);

                    weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);
                }
                else  // delta
                {
                    weight = openpbr_scale_diffuse_specular(weight, inverse_coat_reflection_prob);
                }
            }
        }
        else  // base
        {
            const float base_prob = 1.0f - coat_reflection_prob;
            const float inverse_base_prob = 1.0f / base_prob;
            rand_x = (rand_x - coat_reflection_prob) * inverse_base_prob;
            openpbr_clamp_remapped_random_number(rand_x);

            success = openpbr_sample_lobe(lobe.base_lobe, vec3(rand_x, rand.y, rand.z), view_direction, light_direction, weight, pdf, sampled_type);
            if (success)
            {
                if (!bool(sampled_type & OpenPBR_BsdfLobeTypeSpecular))  // non-delta
                {
                    OpenPBR_DiffuseSpecular bsdf_cos = openpbr_scale_diffuse_specular(weight, pdf);

                    bsdf_cos = openpbr_combine_coat_and_base_evals(
                        lobe, openpbr_calculate_lobe_value(lobe.coat_reflection_lobe, view_direction, light_direction), bsdf_cos, light_direction);
                    pdf = openpbr_combine_coat_and_base_pdfs(
                        openpbr_calculate_lobe_pdf(lobe.coat_reflection_lobe, view_direction, light_direction), pdf, coat_reflection_prob);

                    weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);
                }
                else  // delta
                {
                    const vec3 remaining_scale_factors = openpbr_base_layer_scale_complete(lobe, light_direction) * inverse_base_prob;
                    weight = openpbr_scale_diffuse_specular(weight, remaining_scale_factors);
                }
            }
        }

        return success;
    }
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_CoatingLobe_AggregateLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    const float unscaled_base_lobe_contribution = openpbr_estimate_lobe_contribution(lobe.base_lobe, view_direction, path_throughput);

    if (lobe.inside)
    {
        return unscaled_base_lobe_contribution;
    }
    else
    {
        // Here we can use lobe.in_reflected instead of calling estimate_lobe_contribution
        // because we know the inner details of the coat reflection lobe.
        const float coat_reflection_lobe_contribution = lobe.in_reflected;

        const float base_lobe_scale = openpbr_max_component_of_throughput_weighted_color(path_throughput, lobe.in_base_layer_scale);
        const float scaled_base_lobe_contribution = unscaled_base_lobe_contribution * base_lobe_scale;

        return coat_reflection_lobe_contribution + scaled_base_lobe_contribution;
    }
}

#endif  // OPENPBR_COATING_LOBE_H
