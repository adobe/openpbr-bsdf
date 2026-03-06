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

// Generic "flipped" wrapper lobe: maps any single-child lobe to its flipped-hemisphere counterpart.
// Requires before include:
//     #define OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE <wrapper struct name>
// Requires after include:
//     #undef OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE
// And that OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE has one member:
//     <inner lobe type> flipped_lobe;
// With at least one data member:
//     vec3 normal_ff;
//
// TODO: Note that when the reflected view direction is nearly parallel to the surface, it can
//       sometimes end up in the opposite hemisphere as the flipped normal due to rounding error
//       in the reflection. This is extremely rare, but if it causes problems in practice, we
//       could extract the view-direction-reflecting logic into a separate function and add
//       a fallback mechanism to handle this case, similar to the one in the
//       OpenPBR_ThinWallDiffuseTransmissionLobe's openpbr_initialize_lobe function.

#include "../openpbr_bsdf_lobe_type.h"  // for OpenPBR_BsdfLobeType flags
#include "../openpbr_diffuse_specular.h"
#include "openpbr_math.h"

// Get lobe type: query inner lobe type, then swap flags.
OpenPBR_BsdfLobeType openpbr_get_lobe_type(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE) lobe)
{
    const OpenPBR_BsdfLobeType inner_type = openpbr_get_lobe_type(lobe.flipped_lobe);
    return openpbr_swap_reflect_trans_flags(inner_type);
}

// Evaluate lobe value (BSDF*cosine): reflect view across original normal, then delegate to inner lobe.
OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    const vec3 reflected_view = reflect(view_direction, lobe.flipped_lobe.normal_ff);  // reflects view across surface (view points away from surface)
    return openpbr_calculate_lobe_value(lobe.flipped_lobe, reflected_view, light_direction);
}

// Evaluate lobe-sampling PDF: same reflect-then-delegate.
float openpbr_calculate_lobe_pdf(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    const vec3 reflected_view = reflect(view_direction, lobe.flipped_lobe.normal_ff);  // reflects view across surface (view points away from surface)
    return openpbr_calculate_lobe_pdf(lobe.flipped_lobe, reflected_view, light_direction);
}

// Sample lobe: reflect view, delegate, then swap reflection/transmission bits.
bool openpbr_sample_lobe(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         ADDRESS_SPACE_THREAD OUT(vec3) light_direction,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_DiffuseSpecular) weight,
                         ADDRESS_SPACE_THREAD OUT(float) pdf,
                         ADDRESS_SPACE_THREAD OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const vec3 reflected_view = reflect(view_direction, lobe.flipped_lobe.normal_ff);  // reflects view across surface (view points away from surface)
    if (!openpbr_sample_lobe(lobe.flipped_lobe, rand, reflected_view, light_direction, weight, pdf, sampled_type))
        return false;
    sampled_type = openpbr_swap_reflect_trans_flags(sampled_type);
    return true;
}

// Estimate lobe contribution: reflect view, delegate.
float openpbr_estimate_lobe_contribution(ADDRESS_SPACE_THREAD CONST_REF(OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    const vec3 reflected_view = reflect(view_direction, lobe.flipped_lobe.normal_ff);  // reflects view across surface (view points away from surface)
    return openpbr_estimate_lobe_contribution(lobe.flipped_lobe, reflected_view, path_throughput);
}
