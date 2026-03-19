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

#ifndef OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENTS_H
#define OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENTS_H

#include "openpbr_lobe_utils.h"
#include "openpbr_reflection_transmission_coefficient_structs.h"
#include "openpbr_specialization_constants.h"
#include "openpbr_thin_film_iridescence_utils.h"

// The reflection/transmission class below is designed to handle all of the specular reflection and transmission
// of the base surface of OpenPBR. It has a transparent dielectric component, an opaque dielectric component,
// a metallic component, and a thin-film iridescence component.

////////////////////////////////////////////////////
// ComprehensiveReflectionTransmissionCoefficient //
////////////////////////////////////////////////////

struct OpenPBR_ComprehensiveReflectionTransmissionCoefficient
{
    vec3 eta_t_over_eta_i_for_transparent_part;  // ignoring the effect of the thin film
    vec3 eta_t_over_eta_i_for_opaque_part;       // ignoring the effect of the thin film
    vec3 scale_for_reflection_for_transparent_part;
    vec3 scale_for_reflection_for_opaque_part;
    vec3 transmission;
    vec3 f0_for_metal;
    vec3 f82_tint_for_metal;
    float metal_amount;
    float thin_film_weight;
    float thin_film_thickness_nm;
    float thin_film_exterior_ior;  // absolute IOR - dispersion not supported - TODO: support dispersion
                                   // (note that the "exterior" here can be the interior of an object)
    float thin_film_ior;           // absolute IOR - dispersion not supported
    vec3 thin_film_interior_ior;   // absolute IOR - dispersion supported
    vec3 rgb_wavelengths_nm;
    vec3 thin_wall_constant_reflection_albedo;  // albedo of the thin-wall specular reflection component
                                                // "constant" because it does not dependent on idoth
};

// This function returns the degree to which the thin film exists.
// The returned value is used to blend between standard reflection and transmission and thin-film reflection and transmission.
float openpbr_compute_thin_film_presence(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient)
                                             refl_trans_coeff,
                                         const float idoth)
{
    // Only apply thin film when striking the front of a microfacet (surface normal was faceforwarded but microfacet normal was not).
    if (idoth >= 0.0f)
        return refl_trans_coeff.thin_film_weight * openpbr_thin_film_presence_multiplier(refl_trans_coeff.thin_film_thickness_nm);
    else
        return 0.0f;
}

vec3 openpbr_reflection_coefficient(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient)
                                        refl_trans_coeff,
                                    const float idoth)
{
    vec3 standard_refl_coeff = vec3(0.0f);
    vec3 thin_film_refl_coeff = vec3(0.0f);

    const float thin_film_presence = openpbr_compute_thin_film_presence(refl_trans_coeff, idoth);

    // Calculate the standard reflection coefficient.
    if (thin_film_presence < 1.0f)
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
            standard_refl_coeff += refl_trans_coeff.scale_for_reflection_for_transparent_part *
                                   openpbr_fresnel_rgb(refl_trans_coeff.eta_t_over_eta_i_for_transparent_part,
                                                       idoth,
                                                       OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion));

        standard_refl_coeff +=
            refl_trans_coeff.scale_for_reflection_for_opaque_part *
            openpbr_fresnel_rgb(refl_trans_coeff.eta_t_over_eta_i_for_opaque_part, idoth, OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion));

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
            standard_refl_coeff += refl_trans_coeff.metal_amount *
                                   openpbr_metal_schlick_with_f82_tint(refl_trans_coeff.f0_for_metal, refl_trans_coeff.f82_tint_for_metal, idoth);
    }

    // Calculate the thin-film reflection coefficient.
    if (thin_film_presence > 0.0f)
    {
        OpenPBR_ThinFilmResults thin_film_results = openpbr_thin_film_and_base_reflectance(
            idoth,
            refl_trans_coeff.thin_film_exterior_ior,
            refl_trans_coeff.thin_film_ior,
            refl_trans_coeff.thin_film_interior_ior,
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion),
            refl_trans_coeff.f0_for_metal,
            refl_trans_coeff.f82_tint_for_metal,
            (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) &&
             any(greaterThan(refl_trans_coeff.scale_for_reflection_for_transparent_part, vec3(0.0f)))) ||
                any(greaterThan(refl_trans_coeff.scale_for_reflection_for_opaque_part, vec3(0.0f))),      // dielectric part
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && refl_trans_coeff.metal_amount > 0.0f,  // metal part
            refl_trans_coeff.thin_film_thickness_nm,
            refl_trans_coeff.rgb_wavelengths_nm);

#ifdef ECLAIR_RASTERIZER
        thin_film_results = openpbr_desaturate_thin_film_reflectance(
            thin_film_results.reflectance_dielectric, thin_film_results.reflectance_metal, refl_trans_coeff.thin_film_thickness_nm, idoth);
#endif

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
            thin_film_refl_coeff += refl_trans_coeff.scale_for_reflection_for_transparent_part * thin_film_results.reflectance_dielectric;

        thin_film_refl_coeff += refl_trans_coeff.scale_for_reflection_for_opaque_part * thin_film_results.reflectance_dielectric;

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
            thin_film_refl_coeff += refl_trans_coeff.metal_amount * thin_film_results.reflectance_metal;
    }

    const vec3 refl_coeff = mix(standard_refl_coeff, thin_film_refl_coeff, thin_film_presence);

    // Currently, when thin wall is enabled, the thin wall reflection is calculated elsewhere and added here;
    // the tranmission is zeroed out in the thin-wall case using scale_for_reflection_for_transparent_part
    // because it is accounted for by an entirely different lobe.
    // TODO: Combine thin film and thin wall in a better way.
    const vec3 thin_wall_aware_refl_coeff = refl_coeff + refl_trans_coeff.thin_wall_constant_reflection_albedo;

    return thin_wall_aware_refl_coeff;
}

vec3 openpbr_transmission_coefficient(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient)
                                          refl_trans_coeff,
                                      const float idoth)
{
    vec3 standard_trans_coeff = vec3(0.0f);
    vec3 thin_film_trans_coeff = vec3(0.0f);

    const float thin_film_presence = openpbr_compute_thin_film_presence(refl_trans_coeff, idoth);

    // Calculate the standard transmission coefficient.
    if (thin_film_presence < 1.0f)
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
            standard_trans_coeff +=
                refl_trans_coeff.transmission * (vec3(1.0f) - openpbr_fresnel_rgb(refl_trans_coeff.eta_t_over_eta_i_for_transparent_part,
                                                                                  idoth,
                                                                                  OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion)));
    }

    // Calculate the thin-film transmission coefficient.
    if (thin_film_presence > 0.0f)
    {
        OpenPBR_ThinFilmResults thin_film_results = openpbr_thin_film_and_base_reflectance(
            idoth,
            refl_trans_coeff.thin_film_exterior_ior,
            refl_trans_coeff.thin_film_ior,
            refl_trans_coeff.thin_film_interior_ior,
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion),
            refl_trans_coeff.f0_for_metal,
            refl_trans_coeff.f82_tint_for_metal,
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) &&
                any(greaterThan(refl_trans_coeff.scale_for_reflection_for_transparent_part, vec3(0.0f))),  // dielectric part
            false,                                                                                         // metal part
            refl_trans_coeff.thin_film_thickness_nm,
            refl_trans_coeff.rgb_wavelengths_nm);

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
            thin_film_trans_coeff += refl_trans_coeff.transmission * (vec3(1.0f) - thin_film_results.reflectance_dielectric);
    }

    const vec3 trans_coeff = mix(standard_trans_coeff, thin_film_trans_coeff, thin_film_presence);

    return trans_coeff;
}

// The default implementations of the reflection and transmission probabilities use both coefficients.
// Specialized implementations could calculate both based on the same shared factors (such as fresnel and 1-fresnel).
OpenPBR_AllCoefficients
openpbr_all_coefficients(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient) refl_trans_coeff,
                         const float idoth)
{
    vec3 standard_refl_coeff = vec3(0.0f);
    vec3 standard_trans_coeff = vec3(0.0f);
    vec3 thin_film_refl_coeff = vec3(0.0f);
    vec3 thin_film_trans_coeff = vec3(0.0f);

    const float thin_film_presence = openpbr_compute_thin_film_presence(refl_trans_coeff, idoth);

    // Calculate the standard reflection coefficient.
    if (thin_film_presence < 1.0f)
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        {
            const vec3 fresnel_for_transparent_part = openpbr_fresnel_rgb(
                refl_trans_coeff.eta_t_over_eta_i_for_transparent_part, idoth, OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion));
            standard_refl_coeff += refl_trans_coeff.scale_for_reflection_for_transparent_part * fresnel_for_transparent_part;
            standard_trans_coeff += refl_trans_coeff.transmission * (vec3(1.0f) - fresnel_for_transparent_part);
        }

        standard_refl_coeff +=
            refl_trans_coeff.scale_for_reflection_for_opaque_part *
            openpbr_fresnel_rgb(refl_trans_coeff.eta_t_over_eta_i_for_opaque_part, idoth, OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion));

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
            standard_refl_coeff += refl_trans_coeff.metal_amount *
                                   openpbr_metal_schlick_with_f82_tint(refl_trans_coeff.f0_for_metal, refl_trans_coeff.f82_tint_for_metal, idoth);
    }

    // Calculate the thin-film reflection coefficient.
    if (thin_film_presence > 0.0f)
    {
        OpenPBR_ThinFilmResults thin_film_results = openpbr_thin_film_and_base_reflectance(
            idoth,
            refl_trans_coeff.thin_film_exterior_ior,
            refl_trans_coeff.thin_film_ior,
            refl_trans_coeff.thin_film_interior_ior,
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion),
            refl_trans_coeff.f0_for_metal,
            refl_trans_coeff.f82_tint_for_metal,
            (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) &&
             any(greaterThan(refl_trans_coeff.scale_for_reflection_for_transparent_part, vec3(0.0f)))) ||
                any(greaterThan(refl_trans_coeff.scale_for_reflection_for_opaque_part, vec3(0.0f))),      // dielectric part
            OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && refl_trans_coeff.metal_amount > 0.0f,  // metal part
            refl_trans_coeff.thin_film_thickness_nm,
            refl_trans_coeff.rgb_wavelengths_nm);

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        {
            thin_film_refl_coeff += refl_trans_coeff.scale_for_reflection_for_transparent_part * thin_film_results.reflectance_dielectric;
            thin_film_trans_coeff += refl_trans_coeff.transmission * (vec3(1.0f) - thin_film_results.reflectance_dielectric);
        }

        thin_film_refl_coeff += refl_trans_coeff.scale_for_reflection_for_opaque_part * thin_film_results.reflectance_dielectric;

        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
            thin_film_refl_coeff += refl_trans_coeff.metal_amount * thin_film_results.reflectance_metal;
    }

    const vec3 refl_coeff = mix(standard_refl_coeff, thin_film_refl_coeff, thin_film_presence);
    const vec3 trans_coeff = mix(standard_trans_coeff, thin_film_trans_coeff, thin_film_presence);

    // Currently, when thin wall is enabled, the thin wall reflection is calculated elsewhere and added here;
    // the tranmission is zeroed out in the thin-wall case using scale_for_reflection_for_transparent_part
    // because it is accounted for by an entirely different lobe.
    // TODO: Combine thin film and thin wall in a better way.
    const vec3 thin_wall_aware_refl_coeff = refl_coeff + refl_trans_coeff.thin_wall_constant_reflection_albedo;

    return OPENPBR_MAKE_STRUCT_2(OpenPBR_AllCoefficients, thin_wall_aware_refl_coeff, trans_coeff);
}

float openpbr_reflection_probability(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient)
                                         refl_trans_coeff,
                                     const vec3 path_throughput,
                                     const float idoth)
{
    if (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        return 1.0f;

    const OpenPBR_AllCoefficients coefficients = openpbr_all_coefficients(refl_trans_coeff, idoth);
    const float reflection_coefficient_max = openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.reflection_coefficient);
    const float transmission_coefficient_max =
        openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.transmission_coefficient);
    // Using division rather than subtracting the other probability from one is more numerically stable.
    // openpbr_safe_divide is used to handle the case where the reflection and transmission coefficients are both 0.
    return openpbr_safe_divide(reflection_coefficient_max, reflection_coefficient_max + transmission_coefficient_max, 0.0f);
}

float openpbr_transmission_probability(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient)
                                           refl_trans_coeff,
                                       const vec3 path_throughput,
                                       const float idoth)
{
    if (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        return 0.0f;

    const OpenPBR_AllCoefficients coefficients = openpbr_all_coefficients(refl_trans_coeff, idoth);
    const float reflection_coefficient_max = openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.reflection_coefficient);
    const float transmission_coefficient_max =
        openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.transmission_coefficient);
    // Using division rather than subtracting the other probability from one is more numerically stable.
    // openpbr_safe_divide is used to handle the case where the reflection and transmission coefficients are both 0.
    return openpbr_safe_divide(transmission_coefficient_max, reflection_coefficient_max + transmission_coefficient_max, 0.0f);
}

OpenPBR_AllCoefficientsAndProbabilities openpbr_all_coefficients_and_probabilities(
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient) refl_trans_coeff,
    const vec3 path_throughput,
    const float idoth)
{
    const OpenPBR_AllCoefficients coefficients = openpbr_all_coefficients(refl_trans_coeff, idoth);
    const float reflection_coefficient_max = openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.reflection_coefficient);
    const float transmission_coefficient_max =
        openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients.transmission_coefficient);
    const float coefficient_max_sum = reflection_coefficient_max + transmission_coefficient_max;
    // For the probabilities, using division rather than subtracting the other probability from one
    // or multiplying by the inverse is more numerically precise, and openpbr_safe_divide is used to handle
    // the case where the reflection and transmission coefficients are both 0.
    return OPENPBR_MAKE_STRUCT_4(
        OpenPBR_AllCoefficientsAndProbabilities,
        coefficients.reflection_coefficient,
        coefficients.transmission_coefficient,
        OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) ? openpbr_safe_divide(reflection_coefficient_max, coefficient_max_sum, 0.0f) : 1.0f,
        OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) ? openpbr_safe_divide(transmission_coefficient_max, coefficient_max_sum, 0.0f)
                                                                : 0.0f);
}

float openpbr_estimate_weight(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient) refl_trans_coeff,
                              const vec3 path_throughput,
                              const float idotn)
{
    const OpenPBR_AllCoefficients coefficients = openpbr_all_coefficients(refl_trans_coeff, idotn);
    const vec3 coefficients_sum = coefficients.reflection_coefficient + coefficients.transmission_coefficient;
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, coefficients_sum);
}

float openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveReflectionTransmissionCoefficient) refl_trans_coeff,
    const vec3 path_throughput,
    const float idotn,
    const float isotropic_alpha)
{
    // We don't have a way to calculate the average albedo of this complex reflection/transmission coefficient
    // applied to a GGX microfacet distribution, so we just estimate the weight assuming a smooth surface.
    return openpbr_estimate_weight(refl_trans_coeff, path_throughput, idotn);
}

#endif  // !OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENTS_H
