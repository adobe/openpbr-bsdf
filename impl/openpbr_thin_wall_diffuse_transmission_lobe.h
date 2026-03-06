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

#ifndef OPENPBR_THIN_WALL_DIFFUSE_TRANSMISSION_LOBE_H
#define OPENPBR_THIN_WALL_DIFFUSE_TRANSMISSION_LOBE_H

#include "../openpbr_diffuse_specular.h"
#include "openpbr_diffuse_lobe.h"
#include "openpbr_math.h"

/////////////////////////////////////
// ThinWallDiffuseTransmissionLobe //
/////////////////////////////////////

// This lobe wraps an existing EnergyConservingRoughDiffuseLobe
// but flips it to the opposite hemisphere to represent diffuse transmission.
struct OpenPBR_ThinWallDiffuseTransmissionLobe
{
    OpenPBR_EnergyConservingRoughDiffuseLobe flipped_lobe;
};

// Initialize the inner diffuse lobe with flipped normal and reflected view direction.
void openpbr_initialize_lobe(ADDRESS_SPACE_THREAD INOUT(OpenPBR_ThinWallDiffuseTransmissionLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const vec3 diffuse_albedo,
                             const float diffuse_roughness,
                             const float specular_alpha,
                             const float specular_eta_t_over_eta_i)
{
    const vec3 flipped_normal = -normal_ff;
    vec3 reflected_view = reflect(view_direction, normal_ff);  // reflects view across surface (view points away from surface)

    const float dot_reflected_flipped = dot(reflected_view, flipped_normal);
    if (dot_reflected_flipped < 0.0f)
    {
        ASSERT(dot_reflected_flipped > -1e-5f, "Reflected view is significantly misaligned with flipped normal");

        // In the rare case where the reflected view is on the wrong side of the surface, we force it to be above the surface.
        // This can happen due to numerical precision issues when the view direction is nearly parallel to the surface
        // (rounding error is introduced by the reflect() function above, so the view can get out of sync with the normal).
        reflected_view *= -1.0f;
    }

    openpbr_initialize_lobe(
        lobe.flipped_lobe, flipped_normal, reflected_view, diffuse_albedo, diffuse_roughness, specular_alpha, specular_eta_t_over_eta_i);
}

#define OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE OpenPBR_ThinWallDiffuseTransmissionLobe
#include "openpbr_generic_flipped_lobe.h"
#undef OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE

#endif  // !OPENPBR_THIN_WALL_DIFFUSE_TRANSMISSION_LOBE_H
