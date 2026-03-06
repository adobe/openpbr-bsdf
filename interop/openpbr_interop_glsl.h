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
#define ADDRESS_SPACE_THREAD

// Parameter passing macros.
// GLSL has native out/inout parameter qualifiers.
// CONST_REF maps to const because GLSL passes by value.
#define OUT(type) out type
#define INOUT(type) inout type
#define CONST_REF(type) const type

// Constant declaration macros.
// GLSL uses const for both local and global constants.
#define CONSTEXPR_LOCAL const
#define CONSTEXPR_GLOBAL const

// Constexpr function qualifiers.
// GLSL has no constexpr function concept, so these are empty.
#define GENERAL_CONSTEXPR_FUNCTION
#define LIMITED_CONSTEXPR_FUNCTION

// Function inline specifier.
// GLSL does not require an explicit inline keyword.
#define INLINE_FUNCTION

// Swizzle helper.
#define SWIZZLE(v, suffix) (v).suffix

// Struct-construction helpers.
// clang-format off
#define MAKE_STRUCT_1(type, arg1) type(arg1)
#define MAKE_STRUCT_2(type, arg1, arg2) type(arg1, arg2)
#define MAKE_STRUCT_3(type, arg1, arg2, arg3) type(arg1, arg2, arg3)
#define MAKE_STRUCT_4(type, arg1, arg2, arg3, arg4) type(arg1, arg2, arg3, arg4)
#define MAKE_STRUCT_5(type, arg1, arg2, arg3, arg4, arg5) type(arg1, arg2, arg3, arg4, arg5)
#define MAKE_STRUCT_6(type, arg1, arg2, arg3, arg4, arg5, arg6) type(arg1, arg2, arg3, arg4, arg5, arg6)
#define MAKE_STRUCT_7(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define MAKE_STRUCT_8(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define MAKE_STRUCT_9(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define MAKE_STRUCT_10(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define MAKE_STRUCT_11(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define MAKE_STRUCT_12(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define MAKE_STRUCT_13(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)
#define MAKE_STRUCT_14(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14)
#define MAKE_STRUCT_15(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15)
// clang-format on

// Specialization constants default to regular const values in GLSL mode.
#define DECLARE_SPECIALIZATION_CONSTANT(constant_id_number, name, default_value) CONSTEXPR_GLOBAL bool name = default_value
#define GET_SPECIALIZATION_CONSTANT(name) name

// Assert macros.
// GLSL has no portable assert mechanism, so these are no-ops.
#define ASSERT(expr, message)
#define ASSERT_UNREACHABLE(message)
#define STATIC_ASSERT(expr, message)

// Math type aliases.
// vec2/vec3/vec4 are built into GLSL; no type definitions needed.
// saturate() is defined here as an explicit shim - standard GLSL has no built-in saturate().
// CUDA, MSL, and Slang use their respective language built-ins; NaN behavior may differ from this ternary.
// The C++ backend also defines it as a shim for the same reason.

INLINE_FUNCTION float saturate(const float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

INLINE_FUNCTION vec3 saturate(const vec3 v)
{
    return vec3(saturate(v.x), saturate(v.y), saturate(v.z));
}

#endif  // OPENPBR_INTEROP_GLSL_H
