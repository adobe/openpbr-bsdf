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

#ifndef OPENPBR_API_H
#define OPENPBR_API_H

#include "impl/openpbr_bsdf.h"

// Public API for OpenPBR BSDF
// These wrapper functions provide the stable public interface, calling internal functions.
// More advanced workflows can call additional internal functions directly if needed - see README.md and openpbr_bsdf.h for details.

// Main initialization entrypoint: prepares both volume and BSDF lobes from resolved inputs.
OpenPBR_PreparedBsdf openpbr_prepare_bsdf_and_volume(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                                                     const vec3 path_throughput,
                                                     const vec3 rgb_wavelengths_nm,
                                                     const float exterior_ior,
                                                     const vec3 view_direction)
{
    return openpbr_prepare_bsdf_and_volume_impl(resolved_inputs, path_throughput, rgb_wavelengths_nm, exterior_ior, view_direction);
}

// Importance-samples a light direction and returns weight, PDF, and sampled lobe type.
void openpbr_sample(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_PreparedBsdf) prepared,
                    const vec3 rand,
                    ADDRESS_SPACE_THREAD OUT(vec3) light_direction,
                    ADDRESS_SPACE_THREAD OUT(OpenPBR_DiffuseSpecular) weight,
                    ADDRESS_SPACE_THREAD OUT(float) pdf,
                    ADDRESS_SPACE_THREAD OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    openpbr_sample_impl(prepared, rand, light_direction, weight, pdf, sampled_type);
}

// Evaluates the BSDF for a given light direction (includes cosine term).
OpenPBR_DiffuseSpecular openpbr_eval(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_PreparedBsdf) prepared, const vec3 light_direction)
{
    return openpbr_eval_impl(prepared, light_direction);
}

// Returns the PDF for sampling the given light direction.
float openpbr_pdf(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_PreparedBsdf) prepared, const vec3 light_direction)
{
    return openpbr_pdf_impl(prepared, light_direction);
}

#endif  // OPENPBR_API_H
