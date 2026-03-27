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

// Minimal standalone C++ example for Adobe's OpenPBR BSDF implementation.
// Demonstrates the core path-tracing flow: prepare the BSDF,
// then importance-sample a new scattering direction and its throughput weight.
//
// Build and run:
// g++ -std=c++17 -I/path/to/openpbr-bsdf -I/path/to/glm minimal_cpp_example.cpp -o minimal_cpp_example && ./minimal_cpp_example
//
// Expected output:
// Sampled direction : (-0.689796, 0.165627, 0.704805)
// Throughput weight : (0.834542, 0.495157, 0.359402)
// Sampling PDF      : 0.320592

// Include GLM before openpbr.h so the C++ interop layer can use GLM types and functions.
#include <glm/glm.hpp>

// Include OpenPBR main header after including GLM and tweaking any relevant OpenPBR settings.
#include "openpbr.h"

// Standard includes for this example.
#include <cstdint>
#include <iostream>

// Minimal 32-bit LCG for illustration.
// Returns a float in [0, 1).
// Replace with a higher-quality RNG in a real renderer.
static float lcg_float(std::uint32_t& state)
{
    state = state * 1664525u + 1013904223u;

    // Use the upper 24 bits and scale by 2^-24 to map into [0, 1).
    return static_cast<float>(state >> 8u) * 0x1.0p-24f;
}

int main()
{
    // 1. Set up a simple material.

    OpenPBR_ResolvedInputs inputs = openpbr_make_default_resolved_inputs();
    inputs.base_color = vec3(0.8f, 0.3f, 0.1f);  // terracotta
    inputs.specular_roughness = 0.4f;

    // The local frame at the surface hit point is stored in inputs.shading_basis,
    // which is not part of the official OpenPBR spec.
    // It defaults to a Z-up frame: tangent = X, bitangent = Y, normal = Z.
    // In a real renderer, replace it with world-space vectors from the geometry,
    // and keep view_direction and light_direction in that same space.

    // 2. Prepare the BSDF.

    // view_direction points away from the surface toward the camera
    // and must be in the same space as shading_basis. Here it is 45 degrees from the surface normal.
    const vec3 view_direction = normalize(vec3(1.0f, 0.0f, 1.0f));

    const OpenPBR_PreparedBsdf prepared = openpbr_prepare_bsdf_and_volume(inputs,
                                                                          vec3(1.0f),                     // path throughput
                                                                          OpenPBR_BaseRgbWavelengths_nm,  // fixed RGB wavelengths for this example
                                                                          OpenPBR_VacuumIor,              // IOR of the medium above the surface
                                                                          view_direction);

    // 3. Importance-sample the BSDF.

    // Each xi is computed on its own line here: C++ does not guarantee argument
    // evaluation order, so vec3(f(), f(), f()) could consume RNG values in an unspecified order.
    constexpr std::uint32_t RngSeed = 12345u;  // fixed seed for reproducibility
    std::uint32_t rng_state = RngSeed;
    const float xi0 = lcg_float(rng_state);
    const float xi1 = lcg_float(rng_state);
    const float xi2 = lcg_float(rng_state);
    vec3 light_direction;
    OpenPBR_DiffuseSpecular weight;
    float pdf;
    OpenPBR_BsdfLobeType lobe_type;

    openpbr_sample(prepared, vec3(xi0, xi1, xi2), light_direction, weight, pdf, lobe_type);

    // 4. Print results.
    if (pdf > 0.0f)
    {
        const vec3 weight_sum = openpbr_get_sum_of_diffuse_specular(weight);
        std::cout << "Sampled direction : (" << light_direction.x << ", " << light_direction.y << ", " << light_direction.z << ")\n";
        std::cout << "Throughput weight : (" << weight_sum.x << ", " << weight_sum.y << ", " << weight_sum.z << ")\n";
        std::cout << "Sampling PDF      : " << pdf << "\n";
    }
    else
    {
        std::cout << "No valid BSDF sample generated\n";
    }

    // In a real path tracer, multiply the running path throughput by weight
    // and trace the next ray along light_direction. The PDF can also be used
    // for MIS (multiple importance sampling) with direct-light sampling.
    //
    // lobe_type identifies which type of lobe generated the sample
    // (for example, reflection vs. transmission, diffuse vs. specular)
    // and can be useful for diagnostics.
    //
    // prepared.volume contains the homogeneous volume parameters derived from
    // the OpenPBR inputs. Pass them to your volume integrator to handle
    // subsurface scattering and transmission.

    return 0;
}
