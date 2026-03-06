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

#ifndef CONSTANT_REFLECTION_COEFFICIENT_H
#define CONSTANT_REFLECTION_COEFFICIENT_H

#include "openpbr_reflection_transmission_coefficient_structs.h"

// Below is a simple variation of the reflection/transmission coefficient that only does dielectric reflection based on a constant color.
// It is useful for things like the thin-film specular lobes that are uniformly scaled based on the net reflection and transmission from a window.
// It follows the same pattern as the other reflection/transmission coefficients so it is easily interchangeable with them.

///////////////////////////////////
// ConstantReflectionCoefficient //
///////////////////////////////////

struct OpenPBR_ConstantReflectionCoefficient
{
    vec3 color;  // the constant color used for reflection
};

vec3 openpbr_reflection_coefficient(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff, const float idoth)
{
    return refl_trans_coeff.color;
}

vec3 openpbr_transmission_coefficient(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff, const float idoth)
{
    return vec3(0.0f);
}

// The default implementations of the reflection and transmission probabilities use both coefficients.
// Specialized implementations could calculate both based on the same shared factors (such as fresnel and 1-fresnel).
OpenPBR_AllCoefficients openpbr_all_coefficients(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff,
                                                 const float idoth)
{
    return MAKE_STRUCT_2(OpenPBR_AllCoefficients, refl_trans_coeff.color, vec3(0.0f));
}

float openpbr_reflection_probability(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff,
                                     const vec3 path_throughput,
                                     const float idoth)
{
    return 1.0f;
}

float openpbr_transmission_probability(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff,
                                       const vec3 path_throughput,
                                       const float idoth)
{
    return 0.0f;
}

OpenPBR_AllCoefficientsAndProbabilities
openpbr_all_coefficients_and_probabilities(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff,
                                           const vec3 path_throughput,
                                           const float idoth)
{
    return MAKE_STRUCT_4(OpenPBR_AllCoefficientsAndProbabilities, refl_trans_coeff.color, vec3(0.0f), 1.0f, 0.0f);
}

float openpbr_estimate_weight(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient) refl_trans_coeff,
                              const vec3 path_throughput,
                              const float idotn)
{
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, refl_trans_coeff.color);
}

float openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ConstantReflectionCoefficient)
                                                                              refl_trans_coeff,
                                                                          const vec3 path_throughput,
                                                                          const float idotn,
                                                                          const float isotropic_alpha)
{
    // We don't currently have a way to calculate the average albedo of this constant color reflection coefficient
    // applied to a GGX microfacet distribution, so we just estimate the weight assuming a smooth surface.
    return openpbr_estimate_weight(refl_trans_coeff, path_throughput, idotn);
}

#endif  // !CONSTANT_REFLECTION_COEFFICIENT_H
