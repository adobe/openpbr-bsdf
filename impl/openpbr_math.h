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

#if !OPENPBR_USE_FAST_MATH_APPROXIMATIONS
INLINE_FUNCTION float openpbr_fast_rcp_sqrt(const float x)
{
    return 1.0f / sqrt(x);
}

INLINE_FUNCTION float openpbr_fast_sqrt(const float x)
{
    return sqrt(x);
}

INLINE_FUNCTION vec3 openpbr_fast_sqrt(const vec3 v)
{
    return vec3(sqrt(v.x), sqrt(v.y), sqrt(v.z));
}

INLINE_FUNCTION vec3 openpbr_fast_normalize(const vec3 v)
{
    return normalize(v);
}
#endif

GENERAL_CONSTEXPR_FUNCTION float openpbr_square(const float x)
{
    return x * x;
}

LIMITED_CONSTEXPR_FUNCTION vec3 openpbr_square(const vec3 v)
{
    return vec3(v.x * v.x, v.y * v.y, v.z * v.z);
}

GENERAL_CONSTEXPR_FUNCTION float openpbr_fourth_power(const float x)
{
    return x * x * x * x;
}

GENERAL_CONSTEXPR_FUNCTION float openpbr_fifth_power(const float x)
{
    return x * x * x * x * x;
}

GENERAL_CONSTEXPR_FUNCTION float openpbr_sixth_power(const float x)
{
    return x * x * x * x * x * x;
}

LIMITED_CONSTEXPR_FUNCTION float openpbr_min3(const vec3 v)
{
    return min(min(v.x, v.y), v.z);
}

LIMITED_CONSTEXPR_FUNCTION float openpbr_max3(const vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

INLINE_FUNCTION float openpbr_get_cos_theta_ls(const vec3 direction_lsn)
{
    return direction_lsn.z;
}

INLINE_FUNCTION float openpbr_sign_nonzero(const float x)
{
    return (x >= 0.0f) ? 1.0f : -1.0f;
}

INLINE_FUNCTION vec2 openpbr_sign_nonzero(const vec2 v)
{
    return vec2(openpbr_sign_nonzero(v.x), openpbr_sign_nonzero(v.y));
}

INLINE_FUNCTION float openpbr_safe_divide(const float numerator, const float denominator, const float fallback)
{
    return (denominator != 0.0f) ? (numerator / denominator) : fallback;
}

INLINE_FUNCTION vec3 openpbr_safe_divide(const vec3 numerator, const vec3 denominator, const vec3 fallback)
{
    return vec3(openpbr_safe_divide(numerator[0], denominator[0], fallback[0]),
                openpbr_safe_divide(numerator[1], denominator[1], fallback[1]),
                openpbr_safe_divide(numerator[2], denominator[2], fallback[2]));
}

INLINE_FUNCTION bool openpbr_is_normalized(const vec3 normal, const float tolerance)
{
    // We use <= to ensure that the test could still be passed even if the tolerance were set to zero.
    return abs(length(normal) - 1.0f) <= tolerance;
}

INLINE_FUNCTION bool openpbr_is_normalized(const vec3 normal)
{
    return openpbr_is_normalized(normal, 1.0e-6f);
}

INLINE_FUNCTION float openpbr_sum(const vec3 v)
{
    return v.x + v.y + v.z;
}

INLINE_FUNCTION float openpbr_safe_pow(const float a, const float b)
{
    return a > 0 ? pow(a, b) : 0;
}

INLINE_FUNCTION vec3 openpbr_safe_pow(const vec3 a, const vec3 b)
{
    return vec3(openpbr_safe_pow(a.x, b.x), openpbr_safe_pow(a.y, b.y), openpbr_safe_pow(a.z, b.z));
}

// Assuming a z-up local space, returns whether two directions are in the same hemisphere.
INLINE_FUNCTION bool openpbr_are_in_same_hemisphere_ls(const vec3 direction_lsn, const vec3 other_direction_lsn)
{
    return direction_lsn.z * other_direction_lsn.z > 0.0f;
}

INLINE_FUNCTION float openpbr_fourth_root(const float x)
{
    return sqrt(sqrt(x));
}

INLINE_FUNCTION float openpbr_average(const vec3 v)
{
    return openpbr_sum(v) * (1.0f / 3);
}

INLINE_FUNCTION float openpbr_cube(const float x)
{
    return x * x * x;
}

INLINE_FUNCTION float openpbr_three_halves_power(const float x)
{
    return sqrt(openpbr_cube(x));
}

#endif
