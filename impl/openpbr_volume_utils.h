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

#ifndef OPENPBR_VOLUME_UTILS_H
#define OPENPBR_VOLUME_UTILS_H

#include "../openpbr_volume_homogeneous.h"
#include "openpbr_math.h"

// This file contains the math necessary to calculate
// volume properties based on OpenPBR volumes parameters.

// Helper functions used only in this file.

// Clamping inputs is necessary to handle edge cases that would result in overflow and NaNs,
// and to avoid extreme values that would result in numerical issues during path tracing.
CONSTEXPR_GLOBAL float OpenPBR_MinColor = 1e-3f;
CONSTEXPR_GLOBAL float OpenPBR_MinDistance = 1e-3f;
CONSTEXPR_GLOBAL float OpenPBR_MaxAbsAnisotropy = 0.999f;

vec3 openpbr_clamp_input_color(const vec3 color)
{
    return max(color, vec3(OpenPBR_MinColor));
}

vec3 openpbr_clamp_input_distance(const vec3 distance)
{
    return max(distance, vec3(OpenPBR_MinDistance));
}

float openpbr_clamp_input_distance(const float distance)
{
    return max(distance, OpenPBR_MinDistance);
}

float openpbr_clamp_input_anisotropy(const float anisotropy)
{
    return clamp(anisotropy, -OpenPBR_MaxAbsAnisotropy, OpenPBR_MaxAbsAnisotropy);
}

// Functions used by the OpenPBR material to calculate subsurface and interior volume properties.

// Convert artist-friendly subsurface parameters to low-level volume parameters.
// TODO(sss): Figure out whether unit conversion is needed!
// TODO(sss): Incorporate IOR, roughness, and translucent shadows into the color mapping!
// TODO(sss): Gradually apply similarity theory in later bounces to improve efficiency!
OpenPBR_HomogeneousVolume openpbr_create_subsurface_volume_from_openpbr_params(const float subsurface_radius,
                                                                               const vec3 subsurface_radius_scale,
                                                                               const vec3 subsurface_color,
                                                                               const float subsurface_scatter_anisotropy)
{
    // Calculate the extinction coefficient from the subsurface radius and radius scale.
    // Per the OpenPBR spec:
    //     "Unit: where relevant, otherwise assumed to be dimensionless.
    //      Note that lengths (other than thin_film_thickness) can be assumed to be in the world space units of the scene."

    const vec3 extinction_mean_free_path = openpbr_clamp_input_distance(subsurface_radius * subsurface_radius_scale);

    const vec3 extinction_coefficient = vec3(1.0f) / extinction_mean_free_path;

    // Calculate the single-scattering albedo from the multiple-scattering albedo.
    // This is using the van de Hulst formulas mentioned in the OpenPBR spec.
    // As per the OpenPBR spec, this albedo mapping compensates for the scattering anisotropy, but the extinction mapping does not.
    // TODO(sss): Is the van de Hulst mapping as good as the Hyperion one we were previously using?
    // TODO(sss): Derive an Eclair-specific mapping or at least a general mapping taking into account the IOR and roughness.

    const float clamped_subsurface_anisotropy = openpbr_clamp_input_anisotropy(subsurface_scatter_anisotropy);

    // This is the van de Hulst mapping:
    const vec3 s = vec3(4.09712f) + vec3(4.20863f) * subsurface_color -
                   sqrt(vec3(9.59217f) + vec3(41.6808f) * subsurface_color + vec3(17.7126f) * openpbr_square(subsurface_color));
    const vec3 single_scattering_albedo = (vec3(1.0f) - openpbr_square(s)) / (vec3(1.0f) - clamped_subsurface_anisotropy * openpbr_square(s));

    // This is the Hyperion mapping:
    //         const vec3 A = subsurface_color;
    //         const vec3 AA = A * A;
    //         const vec3 AAA = AA * A;
    //         const vec3 AAAA = AA * AA;
    //         const vec3 single_scattering_albedo = -expm1(-11.43f * A + 15.38f * AA - 13.91f * AAA);
    // TODO(sss): The anisotropy would need to be taken into account if this were to be used.

    // Rounding error occurs in the mapping above; clamp to ensure valid albedo.
    const vec3 clamped_single_scattering_albedo = clamp(single_scattering_albedo, vec3(0.0f), vec3(1.0f));

    // Initialize and return the final homogeneous volume.
    return openpbr_make_volume_from_extinction_coefficient_and_albedo_and_anisotropy(
        extinction_coefficient, clamped_single_scattering_albedo, clamped_subsurface_anisotropy);
}

// Calculate, combine, and return "absorption" and "scattering" volumes.
// TODO(sss): Figure out whether unit conversion is needed!
OpenPBR_HomogeneousVolume openpbr_create_volume_from_openpbr_params(const vec3 transmission_color,
                                                                    const float transmission_depth,
                                                                    const vec3 transmission_scatter,
                                                                    const float transmission_anisotropy)
{
    ASSERT(transmission_depth > 0.0f, "Zero-depth transmission should have been handled outside this function");

    // Early-out if extinction would be zero or invalid.
    if (openpbr_min3(transmission_color) >= 1.0f && openpbr_max3(transmission_scatter) <= 0.0f)
        return openpbr_make_empty_volume();

    const vec3 clamped_transmission_color = openpbr_clamp_input_color(transmission_color);
    const float clamped_transmission_depth = openpbr_clamp_input_distance(transmission_depth);

    const float reciprocal_clamped_transmission_depth = 1.0f / clamped_transmission_depth;

    const vec3 extinction_coefficient = -log(clamped_transmission_color) * reciprocal_clamped_transmission_depth;

    const vec3 scattering_coefficient = transmission_scatter * reciprocal_clamped_transmission_depth;

    vec3 absorption_coefficient = extinction_coefficient - scattering_coefficient;

    // As per the OpenPBR spec:
    //     "If any component of mu_a is negative, then mu_a is shifted by enough gray to make all the components positive"
    if (openpbr_min3(absorption_coefficient) < 0.0f)
        absorption_coefficient -= openpbr_min3(absorption_coefficient);

    return openpbr_make_volume_from_absorption_and_scattering_coefficients_and_anisotropy(
        absorption_coefficient, scattering_coefficient, openpbr_clamp_input_anisotropy(transmission_anisotropy));
}

#endif  // OPENPBR_VOLUME_UTILS_H
