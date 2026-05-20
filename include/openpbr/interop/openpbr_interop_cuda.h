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

// OpenPBR CUDA interop macros.
//
// This header provides CUDA-specific macro definitions so OpenPBR code can compile in CUDA environments.

#ifndef OPENPBR_INTEROP_CUDA_H
#define OPENPBR_INTEROP_CUDA_H

// This interop header is intended for CUDA compilation only.
#if !defined(__CUDACC__)
#error "openpbr_interop_cuda.h requires CUDA compilation (__CUDACC__)."
#endif

// CUDA vector type definitions.
#include <vector_types.h>

// CUDA assert support.
#include <cassert>

// cstdint provides std::uint32_t.
#include <cstdint>

// Memory qualifiers / address space specifiers.
// CUDA does not use function-parameter address space qualifiers.
#define OPENPBR_ADDRESS_SPACE_THREAD

// Parameter passing macros.
// CUDA uses references for output and input/output parameters.
#define OPENPBR_OUT(type) type&
#define OPENPBR_INOUT(type) type&
#define OPENPBR_CONST_REF(type) const type&

// Constant declaration macros.
// CUDA supports constexpr constants like C++.
#define OPENPBR_CONSTEXPR_LOCAL constexpr
#define OPENPBR_CONSTEXPR_GLOBAL static inline constexpr

// Constexpr function qualifiers.
// CUDA supports constexpr functions directly.
#define OPENPBR_GENERAL_CONSTEXPR_FUNCTION static constexpr
#define OPENPBR_LIMITED_CONSTEXPR_FUNCTION static constexpr

// Function inline specifier.
// CUDA GPU-callable helpers use __device__ inline.
#define OPENPBR_INLINE_FUNCTION __device__ inline

// Math type aliases to match GLSL naming conventions.
// CUDA uses float2/float3/float4, but OpenPBR code uses vec2/vec3/vec4.
// Set OPENPBR_USE_CUSTOM_VEC_TYPES = 1 to suppress these if your host already defines them.
#ifndef OPENPBR_USE_CUSTOM_VEC_TYPES
#define OPENPBR_USE_CUSTOM_VEC_TYPES 0
#endif
#if !OPENPBR_USE_CUSTOM_VEC_TYPES
using vec2 = float2;
using vec3 = float3;
using vec4 = float4;
#endif  // !OPENPBR_USE_CUSTOM_VEC_TYPES

// Fixed-width integer type aliases. CUDA is C++-based; std::uint32_t/uint16_t from <cstdint>.
#ifndef OPENPBR_UINT32
#define OPENPBR_UINT32 std::uint32_t
#endif

#ifndef OPENPBR_UINT16
#define OPENPBR_UINT16 std::uint16_t
#endif

// Energy table storage: CUDA supports unsigned short as exactly 16 bits.
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#define OPENPBR_ENERGY_TABLES_USE_UINT16 1
#endif

// GLSL-style mix() - linear interpolation: mix(a, b, t) = a + (b - a) * t.
// CUDA has no built-in mix(); HLSL and Slang use lerp() instead.
// Defined as a macro so it works with any type (float, vec3, etc.) without
// depending on which vec type aliases are active.
#ifndef mix
#define mix(a, b, t) ((a) + ((b) - (a)) * (t))
#endif

// GLSL-style vector comparison and boolean reduction functions.
// CUDA does not provide these; defined here as macros for OpenPBR compatibility.
// For scalar operands (e.g. uint lobe-type comparisons) these are exact equivalents.
// For float3/vec3 operands, == and != compare as structs (all-components), which
// matches the semantics of all(equal(...)) and any(notEqual(...)) in OpenPBR usage.
#ifndef equal
#define equal(a, b) ((a) == (b))
#endif
#ifndef notEqual
#define notEqual(a, b) ((a) != (b))
#endif
#ifndef greaterThan
#define greaterThan(a, b) ((a) > (b))
#endif
#ifndef greaterThanEqual
#define greaterThanEqual(a, b) ((a) >= (b))
#endif
// all()/any() reduce a bool result; identities here since the comparison macros above
// already return a single bool for the types OpenPBR passes to them.
#ifndef all
#define all(x) (x)
#endif
#ifndef any
#define any(x) (x)
#endif

// Swizzle helpers.
OPENPBR_INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec3 v)
{
    return vec2{ v.x, v.y };
}

OPENPBR_INLINE_FUNCTION vec2 openpbr_swizzle_xy(const vec4 v)
{
    return vec2{ v.x, v.y };
}

OPENPBR_INLINE_FUNCTION vec3 openpbr_swizzle_xyz(const vec4 v)
{
    return vec3{ v.x, v.y, v.z };
}

#define OPENPBR_SWIZZLE(v, suffix) openpbr_swizzle_##suffix(v)

// Struct-construction helpers.
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

// saturate() uses the native __saturatef() intrinsic for potentially better codegen.
// Note: NaN behavior may differ from the explicit ternary shim used in the C++ and GLSL backends.
// MSL and Slang also rely on their respective language built-ins with the same caveat.
// Set OPENPBR_USE_CUSTOM_SATURATE = 1 to suppress these if your host already defines saturate().
#ifndef OPENPBR_USE_CUSTOM_SATURATE
#define OPENPBR_USE_CUSTOM_SATURATE 0
#endif
#if !OPENPBR_USE_CUSTOM_SATURATE

OPENPBR_INLINE_FUNCTION float saturate(const float x)
{
    return __saturatef(x);
}

OPENPBR_INLINE_FUNCTION vec3 saturate(const vec3 v)
{
    return vec3(__saturatef(v.x), __saturatef(v.y), __saturatef(v.z));
}

#endif  // !OPENPBR_USE_CUSTOM_SATURATE

// Override any of these by pre-defining before including openpbr.h.
// assert() is used for runtime checks; the "&& (message)" trick embeds the message string.
// do-while(false) makes each macro a single statement, safe in if/else without braces.
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

#endif  // OPENPBR_INTEROP_CUDA_H
