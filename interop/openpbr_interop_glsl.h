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

// OpenPBR GLSL interop macros.
//
// This header provides GLSL-specific macro definitions so OpenPBR code can compile in GLSL environments.
//
// GLSL is the simplest target because the OpenPBR implementation's syntax is already GLSL-like.
// Most macros map directly to GLSL syntax or remain empty.

#ifndef OPENPBR_INTEROP_GLSL_H
#define OPENPBR_INTEROP_GLSL_H

// Memory qualifiers / address space specifiers.
// GLSL does not use function-parameter address space qualifiers.
#define OPENPBR_ADDRESS_SPACE_THREAD

// Parameter passing macros.
// GLSL has native out/inout parameter qualifiers.
// OPENPBR_CONST_REF maps to const because GLSL passes by value.
#define OPENPBR_OUT(type) out type
#define OPENPBR_INOUT(type) inout type
#define OPENPBR_CONST_REF(type) const type

// Constant declaration macros.
// GLSL uses const for both local and global constants.
#define OPENPBR_CONSTEXPR_LOCAL const
#define OPENPBR_CONSTEXPR_GLOBAL const

// Constexpr function qualifiers.
// GLSL has no constexpr function concept, so these are empty.
#define OPENPBR_GENERAL_CONSTEXPR_FUNCTION
#define OPENPBR_LIMITED_CONSTEXPR_FUNCTION

// Function inline specifier.
// GLSL does not require an explicit inline keyword.
#define OPENPBR_INLINE_FUNCTION

// Math type aliases.
// vec2/vec3/vec4 are built-in keywords in GLSL; no type definitions needed.

// Fixed-width integer type aliases. uint is a built-in 32-bit unsigned keyword in GLSL.
// GLSL has no 16-bit integer type; OPENPBR_UINT16 is therefore not defined here
// (OPENPBR_ENERGY_TABLES_USE_UINT16 = 0 for GLSL, so it is never used).
#ifndef OPENPBR_UINT32
#define OPENPBR_UINT32 uint
#endif

// Energy table storage: GLSL core has no 16-bit integer type; fall back to uint.
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#define OPENPBR_ENERGY_TABLES_USE_UINT16 0
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

// saturate() is defined here as an explicit shim - standard GLSL has no built-in saturate().
// CUDA, MSL, and Slang use their respective language built-ins; NaN behavior may differ from this ternary.
// The C++ backend also defines it as a shim for the same reason.
// Set OPENPBR_USE_CUSTOM_SATURATE = 1 to suppress these if your host already defines saturate().
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
// GLSL has no assert mechanism; runtime asserts are silent no-ops. OPENPBR_STATIC_ASSERT
// is also a no-op since GLSL has no static_assert.
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
#define OPENPBR_STATIC_ASSERT(expr, message)
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

#endif  // OPENPBR_INTEROP_GLSL_H
