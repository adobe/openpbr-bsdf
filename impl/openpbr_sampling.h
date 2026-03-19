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

// Sample the upper z hemisphere uniformly and return a unit vector
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_unit_hemisphere_uniform(const vec2 s)
{
    const float phi = OpenPBR_TwoPi * s[0];
    const float z = s[1];                            // cos(theta)
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
OPENPBR_INLINE_FUNCTION float openpbr_compute_pdf_for_sample_unit_hemisphere_cosine(const vec3 direction_lsn)
{
    return max(0.0f, openpbr_get_cos_theta_ls(direction_lsn) * OpenPBR_RcpPi);
}

// Sample a triangle uniformly using Heitz's low-distortion map between triangle and square. (paper V2 optimized version)
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_unit_triangle_uniform(const vec2 s)
{
    vec3 v;
    if (s.y > s.x)
    {
        v.x = 0.5f * s.x;
        v.y = s.y - v.x;
    }
    else
    {
        v.y = 0.5f * s.y;
        v.x = s.x - v.y;
    }
    v.z = 1.0f - v.x - v.y;
    return v;
}

// Sample a unit disk uniformly and return a unit 2D vector
OPENPBR_INLINE_FUNCTION vec2 openpbr_sample_unit_disk_uniform(const vec2 s)
{
    const float a = 2.0f * s.x - 1.0f;
    const float b = 2.0f * s.y - 1.0f;
    float r;
    float phi;
    if (openpbr_square(a) > openpbr_square(b))
    {
        r = a;
        phi = OpenPBR_PiOverFour * (b / a);
    }
    else if (b != 0.0f)
    {
        r = b;
        phi = OpenPBR_HalfPi - OpenPBR_PiOverFour * (a / b);
    }
    else
    {
        r = 0.0f;
        phi = 0.0f;
    }

    return vec2(r * cos(phi), r * sin(phi));
}

// Sample a unit square centered at 0 from [-.5,.5]
OPENPBR_INLINE_FUNCTION vec2 openpbr_sample_unit_rectangle_uniform(const vec2 s)
{
    return vec2(s[0] - 0.5f, s[1] - 0.5f);
}

// Sample a cone with angle theta oriented toward (0, 0, 1).
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_cone_uniform(const vec2 u, const float cos_theta_max)
{
    const float cos_theta = (1.0f - u.x) + u.x * cos_theta_max;
    const float sin_theta = sqrt(1.0f - openpbr_square(cos_theta));
    const float phi = 2.0f * OpenPBR_Pi * u.y;

    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

OPENPBR_INLINE_FUNCTION float openpbr_compute_pdf_for_sample_cone_uniform(const float cos_theta_max)
{
    return 1.0f / (2.0f * OpenPBR_Pi * (1.0f - cos_theta_max));
}

#endif
