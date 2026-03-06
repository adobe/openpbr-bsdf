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

// Memory qualifiers / address space specifiers.
// CUDA does not use function-parameter address space qualifiers.
#define ADDRESS_SPACE_THREAD

// Parameter passing macros.
// CUDA uses references for output and input/output parameters.
#define OUT(type) type&
#define INOUT(type) type&
#define CONST_REF(type) const type&

// Constant declaration macros.
// CUDA supports constexpr constants like C++.
#define CONSTEXPR_LOCAL constexpr
#define CONSTEXPR_GLOBAL static inline constexpr

// Constexpr function qualifiers.
// CUDA supports constexpr functions directly.
#define GENERAL_CONSTEXPR_FUNCTION static constexpr
#define LIMITED_CONSTEXPR_FUNCTION static constexpr

// Function inline specifier.
// CUDA GPU-callable helpers use __device__ inline.
#define INLINE_FUNCTION __device__ inline

// Swizzle helpers.
INLINE_FUNCTION float2 openpbr_swizzle_xy(const float3 v)
{
    return float2{ v.x, v.y };
}

INLINE_FUNCTION float2 openpbr_swizzle_xy(const float4 v)
{
    return float2{ v.x, v.y };
}

INLINE_FUNCTION float3 openpbr_swizzle_xyz(const float4 v)
{
    return float3{ v.x, v.y, v.z };
}

#define SWIZZLE(v, suffix) openpbr_swizzle_##suffix(v)

// Struct-construction helpers.
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

// Specialization constants default to compile-time constants in CUDA mode.
#define DECLARE_SPECIALIZATION_CONSTANT(constant_id_number, name, default_value) CONSTEXPR_GLOBAL bool name = default_value
#define GET_SPECIALIZATION_CONSTANT(name) name

// Assert macros.
// assert() is used for runtime checks and static_assert for compile-time checks.
// The '&& (message)' trick embeds the message string in the assert output.
#define ASSERT(expr, message) assert((expr) && (message))
#define ASSERT_UNREACHABLE(message) assert(false && (message))
#define STATIC_ASSERT(expr, message) static_assert(expr, message)

// Math type aliases to match GLSL naming conventions.
// CUDA uses float2/float3/float4, but OpenPBR code uses vec2/vec3/vec4.
using vec2 = float2;
using vec3 = float3;
using vec4 = float4;

// saturate() uses the native __saturatef() intrinsic for potentially better codegen.
// Note: NaN behavior may differ from the explicit ternary shim used in the C++ and GLSL backends.
// MSL and Slang also rely on their respective language built-ins with the same caveat.

INLINE_FUNCTION float saturate(const float x)
{
    return __saturatef(x);
}

INLINE_FUNCTION vec3 saturate(const vec3 v)
{
    return vec3(__saturatef(v.x), __saturatef(v.y), __saturatef(v.z));
}

#endif  // OPENPBR_INTEROP_CUDA_H
