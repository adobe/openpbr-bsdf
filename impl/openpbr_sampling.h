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

#ifndef OPENPBR_SAMPLING_H
#define OPENPBR_SAMPLING_H

#include "../openpbr_constants.h"
#include "openpbr_math.h"

#define OPENPBR_BSDF_PDF_THRESHOLD 1.0e-6f      // solid angle measure
#define OPENPBR_CONTRIBUTION_THRESHOLD 1.0e-4f  // Light radiance times BSDF times cosine divided by pdf times MIS weight times throughput
#define OPENPBR_DIRAC_DELTA_PDF (-1.0f)

// Sample a sphere uniformly and return a unit vector
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_unit_sphere_uniform(const vec2 s)
{
    const float phi = OpenPBR_TwoPi * s[0];
    const float z = 2.0f * s[1] - 1.0f;              // cos(theta)
    const float r = sqrt(1.0f - openpbr_square(z));  // sin(theta)
    return vec3(cos(phi) * r, sin(phi) * r, z);
}

// Sample the upper z hemisphere with a cosine distribution
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_unit_hemisphere_cosine(const vec2 s)
{
    const float phi = OpenPBR_TwoPi * s[0];
    const float z = sqrt(s[1]);         // cos(theta)
    const float r = sqrt(1.0f - s[1]);  // sin(theta)
    return vec3(cos(phi) * r, sin(phi) * r, z);
}

// Returns the PDF corresponding to the cosine-weighted hemisphere sampling function above.
OPENPBR_INLINE_FUNCTION float openpbr_compute_pdf_for_sample_unit_hemisphere_cosine(const vec3 direction_local)
{
    return max(0.0f, openpbr_get_cos_theta_local(direction_local) * OpenPBR_RcpPi);
}

#endif
