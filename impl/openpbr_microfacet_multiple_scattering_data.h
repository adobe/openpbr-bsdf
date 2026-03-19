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

#ifndef OPENPBR_MICROFACET_MULTIPLE_SCATTERING_DATA_H
#define OPENPBR_MICROFACET_MULTIPLE_SCATTERING_DATA_H

#include "../openpbr_data_constants.h"
#include "openpbr_lobe_utils.h"

#if !OPENPBR_USE_TEXTURE_LUTS
#include "data/openpbr_energy_array_access.h"
#include "data/openpbr_energy_arrays.h"
#endif

OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_EnergyTableSizeMinusOne = OpenPBR_EnergyTableSize - 1;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_IorMax = 2.5f;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_InverseIorMax = 1.0f / OpenPBR_IorMax;

float openpbr_ior_to_exact_index(const float ior)
{
    OPENPBR_CONSTEXPR_LOCAL float IorOne = 1.0f;
    OPENPBR_CONSTEXPR_LOCAL float IorMaxMinusIorOne = OpenPBR_IorMax - IorOne;
    OPENPBR_CONSTEXPR_LOCAL float InverseIorMaxMinusIorOne = 1.0f / IorMaxMinusIorOne;
    OPENPBR_CONSTEXPR_LOCAL int HalfTableSize = OpenPBR_EnergyTableSize / 2;
    OPENPBR_CONSTEXPR_LOCAL int HalfTableSizeMinusOne = HalfTableSize - 1;
    if (ior < IorOne)
    {
        const float inverse_ior = 1.0f / ior;
        const float fraction = (inverse_ior - IorOne) * InverseIorMaxMinusIorOne;
        return HalfTableSizeMinusOne - fraction * HalfTableSizeMinusOne;
    }
    else
    {
        const float fraction = (ior - IorOne) * InverseIorMaxMinusIorOne;
        return HalfTableSize + fraction * HalfTableSizeMinusOne;
    }
}

float openpbr_alpha_to_exact_index(const float alpha)
{
    // NOTE: openpbr_fast_sqrt() is not utilized here as it alters roughness visibly
    const float roughness = sqrt(alpha);
    const float exact_index = roughness * OpenPBR_EnergyTableSizeMinusOne;
    return exact_index;
}

float openpbr_cos_theta_to_exact_index(const float cos_theta)
{
    const float exact_index = cos_theta * OpenPBR_EnergyTableSizeMinusOne;
    return exact_index;
}

float openpbr_remap_exact_index(const float exact_index)
{
    OPENPBR_CONSTEXPR_LOCAL float InverseTableSize = 1.0f / OpenPBR_EnergyTableSize;
    OPENPBR_CONSTEXPR_LOCAL float MinCoordinate = 0.5f * InverseTableSize;
    OPENPBR_CONSTEXPR_LOCAL float MaxCoordinate = 1.0f - MinCoordinate;
    return clamp(MinCoordinate + exact_index * InverseTableSize, MinCoordinate, MaxCoordinate);
}

float openpbr_clamp_exact_index(const float exact_index)
{
    return clamp(exact_index, 0.0f, float(OpenPBR_EnergyTableSizeMinusOne));
}

float openpbr_extrapolate_table_value_beyond_ior_max_if_needed(const float table_value, const float ior)
{
    if (ior > OpenPBR_IorMax || ior < OpenPBR_InverseIorMax)
    {
        OPENPBR_CONSTEXPR_LOCAL float F0Max = openpbr_f0_from_ior(OpenPBR_IorMax);
        OPENPBR_CONSTEXPR_LOCAL float F0ExtrapolationRange = 1.0f - F0Max;
        OPENPBR_CONSTEXPR_LOCAL float InverseF0ExtrapolationRange = 1.0f / F0ExtrapolationRange;
        const float f0 = openpbr_f0_from_ior(ior);
        const float progress_toward_zero = (f0 - F0Max) * InverseF0ExtrapolationRange;
        return (1.0f - progress_toward_zero) * table_value;
    }
    else
    {
        return table_value;
    }
}

// Unified energy table lookup helpers
// Texture mode: OPENPBR_SAMPLE_2D_TEXTURE / OPENPBR_SAMPLE_3D_TEXTURE are called directly with the lut_id.
// Array mode: lut_id selects array via switch in openpbr_energy_array_access.h

float openpbr_look_up_linear(const OpenPBR_LutId lut_id, const float exact_index_x)
{
#if OPENPBR_USE_TEXTURE_LUTS
    const float remapped_exact_index_x = openpbr_remap_exact_index(exact_index_x);
    return OPENPBR_SAMPLE_2D_TEXTURE(lut_id, vec2(remapped_exact_index_x, 0.5f)).r;  // TODO: Use a 1D texture sampler when available.
#else
    const float clamped_exact_index_x = openpbr_clamp_exact_index(exact_index_x);
    return openpbr_energy_array_lookup_linear(lut_id, clamped_exact_index_x);
#endif
}

float openpbr_look_up_bilinear(const OpenPBR_LutId lut_id, const float exact_index_x, const float exact_index_y)
{
#if OPENPBR_USE_TEXTURE_LUTS
    const float remapped_exact_index_x = openpbr_remap_exact_index(exact_index_x);
    const float remapped_exact_index_y = openpbr_remap_exact_index(exact_index_y);
    return OPENPBR_SAMPLE_2D_TEXTURE(lut_id, vec2(remapped_exact_index_y, remapped_exact_index_x)).r;
#else
    const float clamped_exact_index_x = openpbr_clamp_exact_index(exact_index_x);
    const float clamped_exact_index_y = openpbr_clamp_exact_index(exact_index_y);
    return openpbr_energy_array_lookup_bilinear(lut_id, clamped_exact_index_x, clamped_exact_index_y);
#endif
}

float openpbr_look_up_trilinear(const OpenPBR_LutId lut_id, const float exact_index_x, const float exact_index_y, const float exact_index_z)
{
#if OPENPBR_USE_TEXTURE_LUTS
    const float remapped_exact_index_x = openpbr_remap_exact_index(exact_index_x);
    const float remapped_exact_index_y = openpbr_remap_exact_index(exact_index_y);
    const float remapped_exact_index_z = openpbr_remap_exact_index(exact_index_z);
    return OPENPBR_SAMPLE_3D_TEXTURE(lut_id, vec3(remapped_exact_index_z, remapped_exact_index_y, remapped_exact_index_x)).r;
#else
    const float clamped_exact_index_x = openpbr_clamp_exact_index(exact_index_x);
    const float clamped_exact_index_y = openpbr_clamp_exact_index(exact_index_y);
    const float clamped_exact_index_z = openpbr_clamp_exact_index(exact_index_z);
    return openpbr_energy_array_lookup_trilinear(lut_id, clamped_exact_index_x, clamped_exact_index_y, clamped_exact_index_z);
#endif
}

float openpbr_look_up_ideal_dielectric_energy_complement(const float ior, const float alpha, const float cos_theta)
{
    const float exact_index_ior = openpbr_ior_to_exact_index(ior);
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    const float exact_index_cos_theta = openpbr_cos_theta_to_exact_index(cos_theta);
    return openpbr_look_up_trilinear(
        OpenPBR_LutId_IdealDielectricEnergyComplement, exact_index_ior, exact_index_alpha, exact_index_cos_theta);
}

float openpbr_look_up_ideal_dielectric_average_energy_complement(const float ior, const float alpha)
{
    const float exact_index_ior = openpbr_ior_to_exact_index(ior);
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    return openpbr_look_up_bilinear(OpenPBR_LutId_IdealDielectricAverageEnergyComplement, exact_index_ior, exact_index_alpha);
}

float openpbr_look_up_ideal_dielectric_reflection_ratio(const float ior, const float alpha)
{
    const float exact_index_ior = openpbr_ior_to_exact_index(ior);
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    return openpbr_look_up_bilinear(OpenPBR_LutId_IdealDielectricReflectionRatio, exact_index_ior, exact_index_alpha);
}

float openpbr_look_up_opaque_dielectric_energy_complement(const float ior, const float alpha, const float cos_theta)
{
    const float exact_index_ior = openpbr_ior_to_exact_index(ior);
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    const float exact_index_cos_theta = openpbr_cos_theta_to_exact_index(cos_theta);
    const float table_value =
        openpbr_look_up_trilinear(OpenPBR_LutId_OpaqueDielectricEnergyComplement, exact_index_ior, exact_index_alpha, exact_index_cos_theta);
    return openpbr_extrapolate_table_value_beyond_ior_max_if_needed(table_value, ior);
}

float openpbr_look_up_opaque_dielectric_average_energy_complement(const float ior, const float alpha)
{
    const float exact_index_ior = openpbr_ior_to_exact_index(ior);
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    const float table_value =
        openpbr_look_up_bilinear(OpenPBR_LutId_OpaqueDielectricAverageEnergyComplement, exact_index_ior, exact_index_alpha);
    return openpbr_extrapolate_table_value_beyond_ior_max_if_needed(table_value, ior);
}

float openpbr_look_up_ideal_metal_energy_complement(const float alpha, const float cos_theta)
{
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    const float exact_index_cos_theta = openpbr_cos_theta_to_exact_index(cos_theta);
    return openpbr_look_up_bilinear(OpenPBR_LutId_IdealMetalEnergyComplement, exact_index_alpha, exact_index_cos_theta);
}

float openpbr_look_up_ideal_metal_average_energy_complement(const float alpha)
{
    const float exact_index_alpha = openpbr_alpha_to_exact_index(alpha);
    return openpbr_look_up_linear(OpenPBR_LutId_IdealMetalAverageEnergyComplement, exact_index_alpha);
}

// This function can be used for the common case of finding the average energy reflected from an opaque rough dielectric.
// It also contains a special case for zero roughness to produce an exact result in that case.
// This function could be relocated somewhere else if it ever needs to be used outside of this file,
// such as in MinimalMicrofacetReflectionLobe's estimate_lobe_contribution function.
float openpbr_compute_dielectric_energy_reflected(const float ior, const float alpha, const float cos_theta)
{
    if (alpha == 0.0f)
        return openpbr_fresnel(ior, cos_theta);
    else
        return 1.0f - openpbr_look_up_opaque_dielectric_energy_complement(ior, alpha, cos_theta);
}

#endif  // !OPENPBR_MICROFACET_MULTIPLE_SCATTERING_DATA_H
