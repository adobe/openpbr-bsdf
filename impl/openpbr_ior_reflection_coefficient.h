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

#ifndef OPENPBR_IOR_REFLECTION_COEFFICIENT_H
#define OPENPBR_IOR_REFLECTION_COEFFICIENT_H

#include "openpbr_reflection_transmission_coefficient_structs.h"

// Below is a simple variation of the reflection/transmission coefficient that only does dielectric reflection based on an IOR.
// It is useful for things like dielectric coating lobes.
// It follows the same pattern as the other reflection/transmission coefficients so it is easily interchangeable with them.

//////////////////////////////
// IorReflectionCoefficient //
//////////////////////////////

struct OpenPBR_IorReflectionCoefficient
{
    float eta_t_over_eta_i;  // the IOR used for reflection
};

vec3 openpbr_reflection_coefficient(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff, const float idoth)
{
    return vec3(openpbr_fresnel(refl_trans_coeff.eta_t_over_eta_i, idoth));
}

vec3 openpbr_transmission_coefficient(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff, const float idoth)
{
    return vec3(0.0f);
}

// The default implementations of the reflection and transmission probabilities use both coefficients.
// Specialized implementations could calculate both based on the same shared factors (such as fresnel and 1-fresnel).
OpenPBR_AllCoefficients openpbr_all_coefficients(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff, const float idoth)
{
    return MAKE_STRUCT_2(OpenPBR_AllCoefficients, openpbr_reflection_coefficient(refl_trans_coeff, idoth), vec3(0.0f));
}

float openpbr_reflection_probability(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff,
                                     const vec3 path_throughput,
                                     const float idoth)
{
    return 1.0f;
}

float openpbr_transmission_probability(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff,
                                       const vec3 path_throughput,
                                       const float idoth)
{
    return 0.0f;
}

OpenPBR_AllCoefficientsAndProbabilities openpbr_all_coefficients_and_probabilities(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient)
                                                                                       refl_trans_coeff,
                                                                                   const vec3 path_throughput,
                                                                                   const float idoth)
{
    return MAKE_STRUCT_4(OpenPBR_AllCoefficientsAndProbabilities, openpbr_reflection_coefficient(refl_trans_coeff, idoth), vec3(0.0f), 1.0f, 0.0f);
}

float openpbr_estimate_weight(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient) refl_trans_coeff,
                              const vec3 path_throughput,
                              const float idotn)
{
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, openpbr_reflection_coefficient(refl_trans_coeff, idotn));
}

float openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_IorReflectionCoefficient)
                                                                              refl_trans_coeff,
                                                                          const vec3 path_throughput,
                                                                          const float idotn,
                                                                          const float isotropic_alpha)
{
    // In this case we are given the alpha of the GGX microfacet distribution being used with our dielectric Fresnel reflection coefficient,
    // so we can look up the energy reflected (the complement of the energy not reflected) directly.
    // If we ever support delta (zero roughness) reflection, we could update this to
    // use the CoatingLobe's compute_rough_dielectric_energy_reflected function.
    const float average_reflectivity =
        1.0f - openpbr_look_up_opaque_dielectric_energy_complement(refl_trans_coeff.eta_t_over_eta_i, isotropic_alpha, idotn);
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, vec3(average_reflectivity));
}

#endif  // !OPENPBR_IOR_REFLECTION_COEFFICIENT_H
