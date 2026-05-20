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

// Memory qualifiers / address space specifiers.
// C++ does not use function-parameter address space qualifiers.
#define OPENPBR_ADDRESS_SPACE_THREAD

// Parameter passing macros.
// C++ uses references for output and input/output parameters.
#define OPENPBR_OUT(type) type&
#define OPENPBR_INOUT(type) type&
#define OPENPBR_CONST_REF(type) const type&

// Constant declaration macros.
// C++ uses constexpr for compile-time constants.
// Global constants use static inline constexpr for ODR safety.
#define OPENPBR_CONSTEXPR_LOCAL constexpr
#define OPENPBR_CONSTEXPR_GLOBAL static inline constexpr

// Constexpr function qualifiers.
// C++ supports constexpr functions directly.
#define OPENPBR_GENERAL_CONSTEXPR_FUNCTION static constexpr
#define OPENPBR_LIMITED_CONSTEXPR_FUNCTION static constexpr

// Function inline specifier.
// Use inline to avoid multiple-definition issues across translation units.
#define OPENPBR_INLINE_FUNCTION inline

// Math type aliases to match GLSL naming conventions.
// When OPENPBR_USE_CUSTOM_VEC_TYPES = 0 (default), these declarations import GLM vec types
// and math functions into the global namespace under GLSL-compatible names so that OpenPBR
// code can use vec2/vec3/vec4, abs(), cross(), dot(), smoothstep(), etc. without
// explicit glm:: qualification.
//
// When OPENPBR_USE_CUSTOM_VEC_TYPES = 1, this block is skipped. Use this when the host
// environment already provides these GLSL-compatible names - for example, when compiling
// shader code in a renderer that supplies GLSL-style names via preprocessor macros.
// GLM functions that take GLM-typed arguments remain accessible through argument-dependent
// lookup (ADL) even without these declarations.
#if !OPENPBR_USE_CUSTOM_VEC_TYPES

// Types
using glm::vec2;
using glm::vec3;
using glm::vec4;

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

#endif  // !OPENPBR_USE_CUSTOM_VEC_TYPES

// Fixed-width integer type aliases matching shader-language conventions.
// C++ has no "uint" keyword; std::uint32_t / std::uint16_t from <cstdint> are exact.
// Unlike cassert, cstdint is safe to include at class scope (it emits only typedefs,
// not extern "C" linkage specifications), so no #ifndef guard is needed here.
#include <cstdint>
#ifndef OPENPBR_UINT32
#define OPENPBR_UINT32 std::uint32_t
#endif

#ifndef OPENPBR_UINT16
#define OPENPBR_UINT16 std::uint16_t
#endif

// Energy table storage: C++ supports unsigned short as exactly 16 bits.
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#define OPENPBR_ENERGY_TABLES_USE_UINT16 1
#endif

// Swizzle helpers.
// Self-contained implementations - no experimental GLM extension required.
OPENPBR_INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec3 v)
{
    return vec2(v.x, v.y);
}
OPENPBR_INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec4 v)
{
    return vec2(v.x, v.y);
}
OPENPBR_INLINE_FUNCTION vec3 openpbr_swizzle_xyz(const vec4 v)
{
    return vec3(v.x, v.y, v.z);
}
#define OPENPBR_SWIZZLE(v, suffix) openpbr_swizzle_##suffix(v)

// Struct-construction helpers for aggregate types.
// clang-format off
#define OPENPBR_MAKE_STRUCT_1(type, arg1) type{arg1}
#define OPENPBR_MAKE_STRUCT_2(type, arg1, arg2) type{arg1, arg2}
#define OPENPBR_MAKE_STRUCT_3(type, arg1, arg2, arg3) type{arg1, arg2, arg3}
#define OPENPBR_MAKE_STRUCT_4(type, arg1, arg2, arg3, arg4) type{arg1, arg2, arg3, arg4}
#define OPENPBR_MAKE_STRUCT_5(type, arg1, arg2, arg3, arg4, arg5) type{arg1, arg2, arg3, arg4, arg5}
#define OPENPBR_MAKE_STRUCT_6(type, arg1, arg2, arg3, arg4, arg5, arg6) type{arg1, arg2, arg3, arg4, arg5, arg6}
#define OPENPBR_MAKE_STRUCT_7(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7}
#define OPENPBR_MAKE_STRUCT_8(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8}
#define OPENPBR_MAKE_STRUCT_9(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9}
#define OPENPBR_MAKE_STRUCT_10(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10}
#define OPENPBR_MAKE_STRUCT_11(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11}
#define OPENPBR_MAKE_STRUCT_12(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12}
#define OPENPBR_MAKE_STRUCT_13(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13}
#define OPENPBR_MAKE_STRUCT_14(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14}
#define OPENPBR_MAKE_STRUCT_15(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) type{arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15}
// clang-format on

// saturate() is defined here as an explicit shim - neither C++ nor GLM provides it.
// CUDA, MSL, and Slang use their respective language built-ins; NaN behavior may differ from this ternary.
// The GLSL backend also defines it as a shim for the same reason.
// Set OPENPBR_USE_CUSTOM_SATURATE = 1 to suppress these if your host already defines saturate().
#ifndef OPENPBR_USE_CUSTOM_SATURATE
#define OPENPBR_USE_CUSTOM_SATURATE 0
#endif
#if !OPENPBR_USE_CUSTOM_SATURATE

OPENPBR_INLINE_FUNCTION float saturate(const float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

OPENPBR_INLINE_FUNCTION vec3 saturate(const vec3 v)
{
    return vec3(saturate(v.x), saturate(v.y), saturate(v.z));
}

#endif  // !OPENPBR_USE_CUSTOM_SATURATE

// Override any of these by pre-defining before including openpbr.h.
// <cassert> is suppressed only when both OPENPBR_ASSERT and OPENPBR_ASSERT_UNREACHABLE are
// pre-defined (both call assert() in their defaults). This avoids assert.h's extern "C"
// linkage specification being emitted at C++ class scope.
// The "&& (message)" trick embeds the message string in the assert output.
// do-while(false) makes each macro a single statement, safe in if/else without braces.
#if !defined(OPENPBR_ASSERT) || !defined(OPENPBR_ASSERT_UNREACHABLE)
#include <cassert>
#endif
#ifndef OPENPBR_ASSERT
#define OPENPBR_ASSERT(expr, message)                                                                                                                \
    do                                                                                                                                               \
    {                                                                                                                                                \
        assert((expr) && (message));                                                                                                                 \
    } while (false)
#endif
#ifndef OPENPBR_ASSERT_UNREACHABLE
#define OPENPBR_ASSERT_UNREACHABLE(message)                                                                                                          \
    do                                                                                                                                               \
    {                                                                                                                                                \
        assert(false && (message));                                                                                                                  \
    } while (false)
#endif
#ifndef OPENPBR_STATIC_ASSERT
#define OPENPBR_STATIC_ASSERT(expr, message) static_assert(expr, message)
#endif

// Default specialization-constant hook. Override before including any OpenPBR
// header if your renderer provides its own specialization constant pipeline.
#ifndef OPENPBR_GET_SPECIALIZATION_CONSTANT
#define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) true
#endif

#endif  // OPENPBR_INTEROP_CPP_H
