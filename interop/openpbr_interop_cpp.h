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

// OpenPBR C++ interop macros.
//
// This header provides C++-specific macro definitions so OpenPBR code can
// compile in C++ environments (for example, CPU path tracers and offline renderers).
// It uses GLM (OpenGL Mathematics) for GLSL-compatible math types and functions.
//
// Requirements:
//   - GLM (OpenGL Mathematics library: https://github.com/g-truc/glm)
//   - C++17 or later (required for inline constexpr variables)

#ifndef OPENPBR_INTEROP_CPP_H
#define OPENPBR_INTEROP_CPP_H

// cassert provides assert().
#include <cassert>

// GLM (OpenGL Mathematics) must be included by the user before including any
// OpenPBR header. This allows the user to control GLM's location and configuration
// (e.g. vendor/, third_party/, or a system install) without forcing a particular
// include path on the OpenPBR headers.
//
// Typical usage:
//   #include <glm/glm.hpp>
//   #include "openpbr.h"
#ifndef GLM_VERSION
#error "GLM must be included before openpbr_interop_cpp.h. Add #include <glm/glm.hpp> before including OpenPBR headers."
#endif

// Import specific GLM types and functions into the global namespace so OpenPBR
// code can use GLSL-style unqualified names without leaking all of GLM.

// Types
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uint;  // used by OpenPBR_BsdfLobeType and OpenPBR_EnergyTableElement

// Math functions
using glm::abs;
using glm::acos;
using glm::atan;
using glm::clamp;
using glm::cos;
using glm::exp;
using glm::floor;
using glm::log;
using glm::max;
using glm::min;
using glm::mix;
using glm::pow;
using glm::sin;
using glm::smoothstep;
using glm::sqrt;

// Geometric functions
using glm::cross;
using glm::dot;
using glm::length;
using glm::normalize;
using glm::reflect;

// Vector comparison functions (return bvec, but bvec is never explicitly named)
using glm::equal;
using glm::greaterThan;
using glm::greaterThanEqual;
using glm::notEqual;

// Boolean reduction functions
using glm::all;
using glm::any;

// Memory qualifiers / address space specifiers.
// C++ does not use function-parameter address space qualifiers.
#define ADDRESS_SPACE_THREAD

// Parameter passing macros.
// C++ uses references for output and input/output parameters.
#define OUT(type) type&
#define INOUT(type) type&
#define CONST_REF(type) const type&

// Constant declaration macros.
// C++ uses constexpr for compile-time constants.
// Global constants use static inline constexpr for ODR safety.
#define CONSTEXPR_LOCAL constexpr
#define CONSTEXPR_GLOBAL static inline constexpr

// Constexpr function qualifiers.
// C++ supports constexpr functions directly.
#define GENERAL_CONSTEXPR_FUNCTION static constexpr
#define LIMITED_CONSTEXPR_FUNCTION static constexpr

// Function inline specifier.
// Use inline to avoid multiple-definition issues across translation units.
#define INLINE_FUNCTION inline

// Swizzle helpers.
// Self-contained implementations - no experimental GLM extension required.
INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec3 v)
{
    return vec2(v.x, v.y);
}
INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec4 v)
{
    return vec2(v.x, v.y);
}
INLINE_FUNCTION vec3 openpbr_swizzle_xyz(const vec4 v)
{
    return vec3(v.x, v.y, v.z);
}
#define SWIZZLE(v, suffix) openpbr_swizzle_##suffix(v)

// Struct-construction helpers for aggregate types.
// clang-format off
#define MAKE_STRUCT_1(type, arg1) type{arg1}
#define MAKE_STRUCT_2(type, arg1, arg2) type{arg1, arg2}
#define MAKE_STRUCT_3(type, arg1, arg2, arg3) type{arg1, arg2, arg3}
#define MAKE_STRUCT_4(type, arg1, arg2, arg3, arg4) type{arg1, arg2, arg3, arg4}
#define MAKE_STRUCT_5(type, arg1, arg2, arg3, arg4, arg5) type{arg1, arg2, arg3, arg4, arg5}
#define MAKE_STRUCT_6(type, arg1, arg2, arg3, arg4, arg5, arg6) type{arg1, arg2, arg3, arg4, arg5, arg6}
#define MAKE_STRUCT_7(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7}
#define MAKE_STRUCT_8(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8}
#define MAKE_STRUCT_9(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9}
#define MAKE_STRUCT_10(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10}
#define MAKE_STRUCT_11(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11}
#define MAKE_STRUCT_12(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12}
#define MAKE_STRUCT_13(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13}
#define MAKE_STRUCT_14(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14}
#define MAKE_STRUCT_15(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15}
// clang-format on

// Specialization constants default to compile-time constants in C++ mode.
#define DECLARE_SPECIALIZATION_CONSTANT(constant_id_number, name, default_value) CONSTEXPR_GLOBAL bool name = default_value
#define GET_SPECIALIZATION_CONSTANT(name) name

// Assert macros.
// Use assert() for runtime checks and static_assert for compile-time checks.
// The '&& (message)' trick embeds the message string in the assert output.
#define ASSERT(expr, message) assert((expr) && (message))
#define ASSERT_UNREACHABLE(message) assert(false && (message))
#define STATIC_ASSERT(expr, message) static_assert(expr, message)

// Math type aliases to match GLSL naming conventions.
// GLM provides vec2/vec3/vec4 via the "using" declarations above; no type definitions needed.
// saturate() is defined here as an explicit shim - neither C++ nor GLM provides it.
// CUDA, MSL, and Slang use their respective language built-ins; NaN behavior may differ from this ternary.
// The GLSL backend also defines it as a shim for the same reason.

INLINE_FUNCTION float saturate(const float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

INLINE_FUNCTION vec3 saturate(const vec3 v)
{
    return vec3(saturate(v.x), saturate(v.y), saturate(v.z));
}

#endif  // OPENPBR_INTEROP_CPP_H
