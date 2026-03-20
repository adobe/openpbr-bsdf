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

// OpenPBR Slang interop macros.
//
// This header provides Slang-specific macro definitions so OpenPBR code can
// compile in Slang environments.

#ifndef OPENPBR_INTEROP_SLANG_H
#define OPENPBR_INTEROP_SLANG_H

// Memory qualifiers / address space specifiers.
// Slang does not use function-parameter address space qualifiers.
#define OPENPBR_ADDRESS_SPACE_THREAD

// Parameter passing macros.
// Slang uses HLSL-style out/inout parameter qualifiers.
#define OPENPBR_OUT(type) out type
#define OPENPBR_INOUT(type) inout type
#define OPENPBR_CONST_REF(type) const type

// Constant declaration macros.
// Slang uses const for local constants and static const for global constants.
#define OPENPBR_CONSTEXPR_LOCAL const
#define OPENPBR_CONSTEXPR_GLOBAL static const

// Constexpr function qualifiers.
// Slang uses HLSL-style syntax and does not have a constexpr concept for functions.
#define OPENPBR_GENERAL_CONSTEXPR_FUNCTION
#define OPENPBR_LIMITED_CONSTEXPR_FUNCTION

// Function inline specifier.
// Slang does not require an explicit inline keyword.
#define OPENPBR_INLINE_FUNCTION

// Math type aliases to match GLSL naming conventions.
// Slang uses float2/float3/float4 (HLSL-style), but OpenPBR code uses vec2/vec3/vec4.
// Set OPENPBR_USE_CUSTOM_VEC_TYPES = 1 to suppress these if your host already defines them.
#if !OPENPBR_USE_CUSTOM_VEC_TYPES
typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;
#endif  // !OPENPBR_USE_CUSTOM_VEC_TYPES

// Fixed-width integer type aliases. uint is a built-in 32-bit type in Slang.
#ifndef OPENPBR_UINT32
#define OPENPBR_UINT32 uint
#endif

#ifndef OPENPBR_UINT16
#define OPENPBR_UINT16 unsigned short
#endif

// Energy table storage: Slang supports unsigned short as exactly 16 bits.
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#define OPENPBR_ENERGY_TABLES_USE_UINT16 1
#endif

// GLSL-style naming compatibility for Slang (which uses HLSL-style syntax).
// OpenPBR code is written in GLSL-style syntax (mix/equal/notEqual/greaterThan/...)
// while Slang uses different names or relies on operators directly.
// These aliases let OpenPBR compile without forcing users to provide extra wrapper macros.
#ifndef mix
#define mix lerp
#endif
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

// Swizzle helper.
#define OPENPBR_SWIZZLE(v, suffix) (v).suffix

// Struct-construction helpers.
// clang-format off
#define OPENPBR_MAKE_STRUCT_1(type, arg1) type(arg1)
#define OPENPBR_MAKE_STRUCT_2(type, arg1, arg2) type(arg1, arg2)
#define OPENPBR_MAKE_STRUCT_3(type, arg1, arg2, arg3) type(arg1, arg2, arg3)
#define OPENPBR_MAKE_STRUCT_4(type, arg1, arg2, arg3, arg4) type(arg1, arg2, arg3, arg4)
#define OPENPBR_MAKE_STRUCT_5(type, arg1, arg2, arg3, arg4, arg5) type(arg1, arg2, arg3, arg4, arg5)
#define OPENPBR_MAKE_STRUCT_6(type, arg1, arg2, arg3, arg4, arg5, arg6) type(arg1, arg2, arg3, arg4, arg5, arg6)
#define OPENPBR_MAKE_STRUCT_7(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define OPENPBR_MAKE_STRUCT_8(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define OPENPBR_MAKE_STRUCT_9(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define OPENPBR_MAKE_STRUCT_10(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define OPENPBR_MAKE_STRUCT_11(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define OPENPBR_MAKE_STRUCT_12(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define OPENPBR_MAKE_STRUCT_13(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)
#define OPENPBR_MAKE_STRUCT_14(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14)
#define OPENPBR_MAKE_STRUCT_15(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15)
// clang-format on

// saturate() relies on the Slang language built-in; no shim is needed.
// Note: NaN behavior may differ from the explicit ternary shim used in the C++ and GLSL backends.
// CUDA and MSL also rely on their respective language built-ins with the same caveat.

// Override any of these by pre-defining before including openpbr.h.
// Slang has no portable assert mechanism; runtime asserts are silent no-ops.
// do-while(false) makes each macro a single statement, safe in if/else without braces.
#ifndef OPENPBR_ASSERT
#define OPENPBR_ASSERT(expr, message)                                                                                                                \
    do                                                                                                                                               \
    {                                                                                                                                                \
    } while (false)
#endif
#ifndef OPENPBR_ASSERT_UNREACHABLE
#define OPENPBR_ASSERT_UNREACHABLE(message)                                                                                                          \
    do                                                                                                                                               \
    {                                                                                                                                                \
    } while (false)
#endif
#ifndef OPENPBR_STATIC_ASSERT
#define OPENPBR_STATIC_ASSERT(expr, message) static_assert(expr, message)
#endif

// Default: specialization constants become compile-time booleans (all features enabled at
// default_value). Renderers with a real specialization constant pipeline - Vulkan
// layout(constant_id), Metal function_constant, runtime dispatch tables, etc. - can
// override both macros before including any OpenPBR header. See openpbr_settings.h.
#ifndef OPENPBR_DECLARE_SPECIALIZATION_CONSTANT
#define OPENPBR_DECLARE_SPECIALIZATION_CONSTANT(constant_id_number, name, default_value) OPENPBR_CONSTEXPR_GLOBAL bool name = default_value
#endif
#ifndef OPENPBR_GET_SPECIALIZATION_CONSTANT
#define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) name
#endif

#endif  // OPENPBR_INTEROP_SLANG_H
