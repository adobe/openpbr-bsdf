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

#ifndef OPENPBR_MATH_H
#define OPENPBR_MATH_H

// Fast math hooks. Define any of these macros before including openpbr.h to supply
// GPU-accelerated implementations. Undefined macros fall back to the standard library.
//
//   #define OPENPBR_FAST_RCP_SQRT(x)  fast_rcp_sqrt(x)   // fast 1/sqrt(x) for float x
//   #define OPENPBR_FAST_SQRT(x)      fast_sqrt(x)       // fast sqrt(x) for float x
//   #define OPENPBR_FAST_NORMALIZE(v) fast_normalize(v)  // fast normalize(v) for vec3 v
//
// OPENPBR_FAST_SQRT is also applied component-wise to vec3 arguments.

#ifndef OPENPBR_FAST_RCP_SQRT
#define OPENPBR_FAST_RCP_SQRT(x) (1.0f / sqrt(x))
#endif
#ifndef OPENPBR_FAST_SQRT
#define OPENPBR_FAST_SQRT(x) sqrt(x)
#endif
#ifndef OPENPBR_FAST_NORMALIZE
#define OPENPBR_FAST_NORMALIZE(v) normalize(v)
#endif

// clang-format off
OPENPBR_INLINE_FUNCTION float openpbr_fast_rcp_sqrt(const float x) { return OPENPBR_FAST_RCP_SQRT(x); }
OPENPBR_INLINE_FUNCTION float openpbr_fast_sqrt(const float x)     { return OPENPBR_FAST_SQRT(x); }
OPENPBR_INLINE_FUNCTION vec3  openpbr_fast_sqrt(const vec3 v)      { return vec3(OPENPBR_FAST_SQRT(v.x), OPENPBR_FAST_SQRT(v.y), OPENPBR_FAST_SQRT(v.z)); }
OPENPBR_INLINE_FUNCTION vec3  openpbr_fast_normalize(const vec3 v) { return OPENPBR_FAST_NORMALIZE(v); }
// clang-format on

OPENPBR_GENERAL_CONSTEXPR_FUNCTION float openpbr_square(const float x)
{
    return x * x;
}

OPENPBR_LIMITED_CONSTEXPR_FUNCTION vec3 openpbr_square(const vec3 v)
{
    return vec3(v.x * v.x, v.y * v.y, v.z * v.z);
}

OPENPBR_GENERAL_CONSTEXPR_FUNCTION float openpbr_fourth_power(const float x)
{
    const float xx = x * x;
    return xx * xx;
}

OPENPBR_GENERAL_CONSTEXPR_FUNCTION float openpbr_fifth_power(const float x)
{
    const float xx = x * x;
    return xx * xx * x;
}

OPENPBR_GENERAL_CONSTEXPR_FUNCTION float openpbr_sixth_power(const float x)
{
    const float xx = x * x;
    return xx * xx * xx;
}

OPENPBR_LIMITED_CONSTEXPR_FUNCTION float openpbr_min3(const vec3 v)
{
    return min(min(v.x, v.y), v.z);
}

OPENPBR_LIMITED_CONSTEXPR_FUNCTION float openpbr_max3(const vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

OPENPBR_INLINE_FUNCTION float openpbr_get_cos_theta_local(const vec3 direction_local)
{
    return direction_local.z;
}

OPENPBR_INLINE_FUNCTION float openpbr_sign_nonzero(const float x)
{
    return (x >= 0.0f) ? 1.0f : -1.0f;
}

OPENPBR_INLINE_FUNCTION vec2 openpbr_sign_nonzero(const vec2 v)
{
    return vec2(openpbr_sign_nonzero(v.x), openpbr_sign_nonzero(v.y));
}

OPENPBR_INLINE_FUNCTION float openpbr_safe_divide(const float numerator, const float denominator, const float fallback)
{
    return (denominator != 0.0f) ? (numerator / denominator) : fallback;
}

OPENPBR_INLINE_FUNCTION vec3 openpbr_safe_divide(const vec3 numerator, const vec3 denominator, const vec3 fallback)
{
    return vec3(openpbr_safe_divide(numerator[0], denominator[0], fallback[0]),
                openpbr_safe_divide(numerator[1], denominator[1], fallback[1]),
                openpbr_safe_divide(numerator[2], denominator[2], fallback[2]));
}

OPENPBR_INLINE_FUNCTION bool openpbr_is_normalized(const vec3 normal, const float tolerance)
{
    // We use <= to ensure that the test could still be passed even if the tolerance were set to zero.
    return abs(length(normal) - 1.0f) <= tolerance;
}

OPENPBR_INLINE_FUNCTION bool openpbr_is_normalized(const vec3 normal)
{
    return openpbr_is_normalized(normal, 1.0e-6f);
}

OPENPBR_INLINE_FUNCTION float openpbr_sum(const vec3 v)
{
    return v.x + v.y + v.z;
}

OPENPBR_INLINE_FUNCTION float openpbr_safe_pow(const float a, const float b)
{
    return a > 0 ? pow(a, b) : 0;
}

OPENPBR_INLINE_FUNCTION vec3 openpbr_safe_pow(const vec3 a, const vec3 b)
{
    return vec3(openpbr_safe_pow(a.x, b.x), openpbr_safe_pow(a.y, b.y), openpbr_safe_pow(a.z, b.z));
}

// Assuming a z-up local space, returns whether two directions are in the same hemisphere.
OPENPBR_INLINE_FUNCTION bool openpbr_are_in_same_hemisphere_local(const vec3 direction_local, const vec3 other_direction_local)
{
    return direction_local.z * other_direction_local.z > 0.0f;
}

OPENPBR_INLINE_FUNCTION float openpbr_fourth_root(const float x)
{
    return sqrt(sqrt(x));
}

OPENPBR_INLINE_FUNCTION float openpbr_average(const vec3 v)
{
    return openpbr_sum(v) * (1.0f / 3);
}

OPENPBR_INLINE_FUNCTION float openpbr_cube(const float x)
{
    return x * x * x;
}

OPENPBR_INLINE_FUNCTION float openpbr_three_halves_power(const float x)
{
    return sqrt(openpbr_cube(x));
}

#endif
