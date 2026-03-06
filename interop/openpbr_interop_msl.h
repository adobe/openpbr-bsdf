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

// OpenPBR Metal Shading Language (MSL) interop macros.
//
// This header provides MSL-specific macro definitions so OpenPBR code can compile in Metal environments.

#ifndef OPENPBR_INTEROP_MSL_H
#define OPENPBR_INTEROP_MSL_H

// This interop header is intended for MSL compilation only.
#if !defined(__METAL_VERSION__)
#error "openpbr_interop_msl.h requires Metal compilation (__METAL_VERSION__)."
#endif

// Memory qualifiers / address space specifiers.
// MSL requires explicit address space qualifiers for reference parameters.
#define ADDRESS_SPACE_THREAD thread

// Parameter passing macros.
// MSL uses references like C++.
#define OUT(type) type&
#define INOUT(type) type&
#define CONST_REF(type) const type&

// Constant declaration macros.
// MSL uses constexpr, and global constants live in the constant address space.
#define CONSTEXPR_LOCAL constexpr
#define CONSTEXPR_GLOBAL static constexpr constant inline

// Constexpr function qualifiers.
// GENERAL_CONSTEXPR_FUNCTION is for scalar-only functions.
// LIMITED_CONSTEXPR_FUNCTION is for functions that use vector types (float2/float3/float4).
// Metal SIMD vector types are not reliably constexpr-capable across toolchains,
// so LIMITED_CONSTEXPR_FUNCTION remains non-constexpr.
#define GENERAL_CONSTEXPR_FUNCTION static constexpr
#define LIMITED_CONSTEXPR_FUNCTION static

// Function inline specifier.
// MSL supports inline functions.
#define INLINE_FUNCTION inline

// Swizzle helper.
#define SWIZZLE(v, suffix) (v).suffix

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

// Specialization constants default to compile-time constants in MSL mode.
#define DECLARE_SPECIALIZATION_CONSTANT(constant_id_number, name, default_value) CONSTEXPR_GLOBAL bool name = default_value
#define GET_SPECIALIZATION_CONSTANT(name) name

// Assert macros.
// Runtime asserts are no-ops; compile-time checks map to static_assert.
#define ASSERT(expr, message)
#define ASSERT_UNREACHABLE(message)
#define STATIC_ASSERT(expr, message) static_assert(expr, message)

// Math type aliases to match GLSL naming conventions.
// MSL uses float2/float3/float4, but OpenPBR code uses vec2/vec3/vec4.
typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

// saturate() relies on the MSL language built-in; no shim is needed.
// Note: NaN behavior may differ from the explicit ternary shim used in the C++ and GLSL backends.
// CUDA and Slang also rely on their respective language built-ins with the same caveat.

#endif  // OPENPBR_INTEROP_MSL_H
