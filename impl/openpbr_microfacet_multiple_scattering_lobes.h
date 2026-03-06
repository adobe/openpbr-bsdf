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

#ifndef OPENPBR_MICROFACET_MULTIPLE_SCATTERING_LOBES_H
#define OPENPBR_MICROFACET_MULTIPLE_SCATTERING_LOBES_H

#include "../openpbr_diffuse_specular.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_microfacet_multiple_scattering_data.h"

CONSTEXPR_GLOBAL float OpenPBR_MinRoughnessWithVisibleEnergyLoss = 0.04f;  // empirically derived
CONSTEXPR_GLOBAL float OpenPBR_MinAlphaWithVisibleEnergyLoss = OpenPBR_MinRoughnessWithVisibleEnergyLoss * OpenPBR_MinRoughnessWithVisibleEnergyLoss;

////////////////////////////////////////////////
// DielectricMicrofacetMultipleScatteringLobe //
////////////////////////////////////////////////

struct OpenPBR_DielectricMicrofacetMultipleScatteringLobe
{
    vec3 normal_ff;
    float alpha;
    float eta_t_over_eta_i;
    vec3 scale_refl;
    vec3 scale_trans;
    float reflection_ratio_ti;
    float energy_complement_ti_idotn;
};

float openpbr_clamped_tabulated_factors(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe, float odotn)
{
    float ior_to_use;
    if (odotn > 0.0f)
    {
        ior_to_use = lobe.eta_t_over_eta_i;
    }
    else
    {
        odotn *= -1.0f;
        const float eta_i_over_eta_t = 1.0f / lobe.eta_t_over_eta_i;
        ior_to_use = eta_i_over_eta_t;
    }

    const float tabulated_factors =
        lobe.energy_complement_ti_idotn * openpbr_look_up_ideal_dielectric_energy_complement(ior_to_use, lobe.alpha, odotn) /
        openpbr_clamp_average_energy_complement_above_zero(openpbr_look_up_ideal_dielectric_average_energy_complement(ior_to_use, lobe.alpha));

    // This clamping can prevent weights from getting out of control near grazing angles at low roughnesses.
    return min(tabulated_factors, openpbr_safe_divide(1.0f, odotn, 0.0f));
}

float openpbr_clamped_tabulated_factors_including_ratio(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                                                        float odotn)
{
    float ior_to_use;
    float ratio_to_use;
    if (odotn > 0.0f)
    {
        ior_to_use = lobe.eta_t_over_eta_i;
        ratio_to_use = lobe.reflection_ratio_ti;
    }
    else
    {
        odotn *= -1.0f;
        const float eta_i_over_eta_t = 1.0f / lobe.eta_t_over_eta_i;
        ior_to_use = eta_i_over_eta_t;
        ratio_to_use = 1.0f - lobe.reflection_ratio_ti;
    }

    float tabulated_factors =
        lobe.energy_complement_ti_idotn * openpbr_look_up_ideal_dielectric_energy_complement(ior_to_use, lobe.alpha, odotn) /
        openpbr_clamp_average_energy_complement_above_zero(openpbr_look_up_ideal_dielectric_average_energy_complement(ior_to_use, lobe.alpha));
    tabulated_factors *= ratio_to_use;

    // This clamping is necessary to avoid weights from getting out of control near grazing angles at low roughnesses.
    return min(tabulated_factors, openpbr_safe_divide(1.0f, odotn, 0.0f));
}

void openpbr_initialize_lobe(ADDRESS_SPACE_THREAD INOUT(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const float alpha,
                             const float eta_t_over_eta_i,
                             const vec3 scale_refl,
                             const vec3 scale_trans)
{
    // Copied properties.
    lobe.normal_ff = normal_ff;
    lobe.alpha = alpha;
    lobe.eta_t_over_eta_i = eta_t_over_eta_i;
    lobe.scale_refl = scale_refl;
    lobe.scale_trans = scale_trans;

    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return;

    // Derived properties.
    lobe.reflection_ratio_ti = openpbr_look_up_ideal_dielectric_reflection_ratio(lobe.eta_t_over_eta_i, lobe.alpha);
    const float idotn = dot(view_direction, lobe.normal_ff);
    lobe.energy_complement_ti_idotn = openpbr_look_up_ideal_dielectric_energy_complement(lobe.eta_t_over_eta_i, lobe.alpha, idotn);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe)
{
    return OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection | OpenPBR_BsdfLobeTypeTransmission;
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return openpbr_make_zero_diffuse_specular();

    const float odotn = dot(light_direction, lobe.normal_ff);
    const float unscaled = openpbr_clamped_tabulated_factors_including_ratio(lobe, odotn) * OpenPBR_RcpPi;
    const float unscaled_cos = unscaled * abs(odotn);
    if (odotn > 0.0f)
    {
        return openpbr_make_diffuse_specular_from_specular(lobe.scale_refl * unscaled_cos);
    }
    else
    {
        return openpbr_make_diffuse_specular_from_specular(lobe.scale_trans * unscaled_cos);
    }
}

float openpbr_calculate_lobe_pdf(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return 0.0f;

    const float odotn = dot(light_direction, lobe.normal_ff);
    if (odotn > 0.0f)
    {
        return OpenPBR_RcpTwoPi * lobe.reflection_ratio_ti;
    }
    else
    {
        return OpenPBR_RcpTwoPi * (1.0f - lobe.reflection_ratio_ti);
    }
}

bool openpbr_sample_lobe(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         ADDRESS_SPACE_THREAD OUT(vec3) light_direction,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_DiffuseSpecular) weight,
                         ADDRESS_SPACE_THREAD OUT(float) pdf,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    // Choose reflection or transmission first.
    float rand_x = rand.x;
    float z;
    if (rand_x < lobe.reflection_ratio_ti)  // reflection
    {
        rand_x /= lobe.reflection_ratio_ti;  // remap used random number
        openpbr_clamp_remapped_random_number(rand_x);
        z = rand_x;
    }
    else  // transmission
    {
        rand_x = (rand_x - lobe.reflection_ratio_ti) / (1.0f - lobe.reflection_ratio_ti);  // remap used random number
        openpbr_clamp_remapped_random_number(rand_x);
        z = -rand_x;
    }

    const float horizontal = sqrt(1.0f - openpbr_square(z));
    const float azimuth = OpenPBR_TwoPi * rand.y;
    const float y = sin(azimuth) * horizontal;
    const float x = cos(azimuth) * horizontal;

    light_direction = openpbr_local_to_world(openpbr_make_basis(lobe.normal_ff), vec3(x, y, z));

    const float odotn = dot(light_direction, lobe.normal_ff);
    const float unscaled_weight = 2.0f * abs(odotn) * openpbr_clamped_tabulated_factors(lobe, odotn);

    if (odotn > 0.0f)  // reflection
    {
        weight = openpbr_make_diffuse_specular_from_specular(lobe.scale_refl * unscaled_weight);
        pdf = OpenPBR_RcpTwoPi * lobe.reflection_ratio_ti;
        sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
    }
    else  // transmission
    {
        weight = openpbr_make_diffuse_specular_from_specular(lobe.scale_trans * unscaled_weight);
        pdf = OpenPBR_RcpTwoPi * (1.0f - lobe.reflection_ratio_ti);
        sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeTransmission;
    }

    return true;
}

float openpbr_estimate_lobe_contribution(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DielectricMicrofacetMultipleScatteringLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return 0.0f;

    const vec3 weighted_average_scale = lobe.scale_refl * lobe.reflection_ratio_ti + lobe.scale_trans * (1.0f - lobe.reflection_ratio_ti);
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, weighted_average_scale) * lobe.energy_complement_ti_idotn;
}

///////////////////////////////////////////
// MetalMicrofacetMultipleScatteringLobe //
///////////////////////////////////////////

struct OpenPBR_MetalMicrofacetMultipleScatteringLobe
{
    vec3 normal_ff;
    float alpha;
    vec3 scale;
    float energy_complement_idotn;
};

float openpbr_clamped_tabulated_factors(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe, const float odotn)
{
    const float tabulated_factors =
        lobe.energy_complement_idotn * openpbr_look_up_ideal_metal_energy_complement(lobe.alpha, odotn) /
        openpbr_clamp_average_energy_complement_above_zero(openpbr_look_up_ideal_metal_average_energy_complement(lobe.alpha));
    // This clamping can prevent weights from getting out of control near grazing angles at low roughnesses.
    return min(tabulated_factors, openpbr_safe_divide(1.0f, odotn, 0.0f));
}

void openpbr_initialize_lobe(ADDRESS_SPACE_THREAD INOUT(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const float alpha,
                             const vec3 scale)
{
    // Copied properties.
    lobe.normal_ff = normal_ff;
    lobe.alpha = alpha;
    lobe.scale = scale;

    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return;

    // Derived properties.
    const float idotn = dot(view_direction, lobe.normal_ff);
    lobe.energy_complement_idotn = openpbr_look_up_ideal_metal_energy_complement(lobe.alpha, idotn);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe)
{
    return OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return openpbr_make_zero_diffuse_specular();

    const float odotn = dot(light_direction, lobe.normal_ff);
    if (odotn > 0.0f)
    {
        const float unscaled = openpbr_clamped_tabulated_factors(lobe, odotn) * OpenPBR_RcpPi;
        const float unscaled_cos = unscaled * odotn;
        return openpbr_make_diffuse_specular_from_specular(lobe.scale * unscaled_cos);
    }
    else
    {
        return openpbr_make_zero_diffuse_specular();
    }
}

float openpbr_calculate_lobe_pdf(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return 0.0f;

    const float odotn = dot(light_direction, lobe.normal_ff);
    if (odotn > 0.0f)
    {
        return OpenPBR_RcpTwoPi;
    }
    else
    {
        return 0.0f;
    }
}

bool openpbr_sample_lobe(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         ADDRESS_SPACE_THREAD OUT(vec3) light_direction,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_DiffuseSpecular) weight,
                         ADDRESS_SPACE_THREAD OUT(float) pdf,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    const float z = rand.x;
    const float horizontal = sqrt(1.0f - openpbr_square(z));
    const float azimuth = OpenPBR_TwoPi * rand.y;
    const float y = sin(azimuth) * horizontal;
    const float x = cos(azimuth) * horizontal;

    light_direction = openpbr_local_to_world(openpbr_make_basis(lobe.normal_ff), vec3(x, y, z));

    const float odotn = dot(light_direction, lobe.normal_ff);
    const float unscaled_weight = 2.0f * odotn * openpbr_clamped_tabulated_factors(lobe, odotn);

    weight = openpbr_make_diffuse_specular_from_specular(lobe.scale * unscaled_weight);
    pdf = OpenPBR_RcpTwoPi;
    sampled_type = openpbr_get_lobe_type(lobe);

    return true;
}

float openpbr_estimate_lobe_contribution(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_MetalMicrofacetMultipleScatteringLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    if (lobe.alpha < OpenPBR_MinAlphaWithVisibleEnergyLoss)
        return 0.0f;

    return openpbr_max_component_of_throughput_weighted_color(path_throughput, lobe.scale) * lobe.energy_complement_idotn;
}

#endif  // !OPENPBR_MICROFACET_MULTIPLE_SCATTERING_LOBES_H
