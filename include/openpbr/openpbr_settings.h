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

#ifndef OPENPBR_SETTINGS_H
#define OPENPBR_SETTINGS_H

// ======================================
// OpenPBR Configuration Surface Overview
// ======================================
//
// Most users only need one setting:
//   - OPENPBR_USE_TEXTURE_LUTS        — 0 (default, self-contained arrays) or 1 (GPU textures)
//
// Auto-configured; override only if the default is wrong for your target:
//   - OPENPBR_LANGUAGE_TARGET_*        — auto-detected from compiler macros; set explicitly for Slang
//
// Optional performance hooks — define any of these to supply a faster platform-specific
// implementation (GPU hardware intrinsics, CPU approximations, etc.):
//   - OPENPBR_FAST_RCP_SQRT(x), OPENPBR_FAST_SQRT(x), OPENPBR_FAST_NORMALIZE(v)
//
// Conflict-suppression hooks — set to 1 if your host already defines these:
//   - OPENPBR_USE_CUSTOM_SATURATE      — suppress OpenPBR's saturate() shim
//   - OPENPBR_USE_CUSTOM_VEC_TYPES     — suppress vec2/vec3/vec4 aliases and GLM using-declarations
//
// GPU pipeline hook — override if your renderer has a real specialization constant pipeline:
//   - OPENPBR_GET_SPECIALIZATION_CONSTANT(name)
//
// Advanced escape hatches — rarely needed:
//   - OPENPBR_ASSERT(expr, message)        — redirect runtime assertions
//   - OPENPBR_ASSERT_UNREACHABLE(message)  — redirect unreachable-branch assertions
//     (pre-defining both also suppresses #include <cassert> in the C++ backend)
//   - OPENPBR_STATIC_ASSERT(expr, message) — redirect static assertions
//   - OPENPBR_USE_CUSTOM_INTEROP           — replace the entire interop layer

// ======================================
// Lookup Table Access Mode Configuration
// ======================================
//
// This OpenPBR BSDF implementation uses precomputed lookup tables (LUTs) for:
//   - Linearly Transformed Cosines (LTC) coefficients for the fuzz lobe
//   - Energy compensation tables for microfacet multiple scattering
//
// This setting controls how these tables are accessed:
//
// OPENPBR_USE_TEXTURE_LUTS = 0 (Array Mode, Default)
//   - LUTs are hardcoded as constant arrays in shader/CPU code
//   - No texture infrastructure or host-side binding required
//   - Fully self-contained and portable
//   - Useful for:
//       * Standalone OpenPBR implementations
//       * CPU-based path tracers
//       * Platforms without texture support
//       * Testing and validation
//   - Slightly slower due to manual interpolation
//   - Increases shader code size
//
// OPENPBR_USE_TEXTURE_LUTS = 1 (Texture Mode)
//   - LUTs are stored in textures
//   - Requires host code to create and bind textures
//   - Best performance on GPU (hardware interpolation, filtering, caching)
//   - Used by Eclair renderer and other texture-capable renderers
//   - Requires the following renderer-provided macros to be defined before openpbr.h:
//
//       OPENPBR_SAMPLE_2D_TEXTURE(lut_id, uv)
//           Given an OpenPBR LUT ID (one of the OpenPBR_LutId_* constants defined in
//           openpbr_data_constants.h) and a vec2 UV, samples the corresponding
//           2D texture and returns a vec4. OpenPBR reads the result via .r (scalar tables)
//           or .xyz (the LTC table); pack data in those components accordingly.
//
//       OPENPBR_SAMPLE_3D_TEXTURE(lut_id, uvw)
//           Same contract, but for the two 3D energy tables
//           (OpenPBR_LutId_IdealDielectricEnergyComplement = 0,
//            OpenPBR_LutId_OpaqueDielectricEnergyComplement = 3).
//           Returns a vec4; OpenPBR accesses the result via .r.
//
//   OpenPBR defines eight texture IDs (integer constants, 0–7) in
//   openpbr_data_constants.h. Renderers that store OpenPBR textures
//   consecutively in a bindless array can use these IDs directly as slot offsets.
//
//   The headers will emit a clear error if any of these macros are missing.
//
// Example for renderers with texture support:
//   #define OPENPBR_USE_TEXTURE_LUTS 1
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_TEXTURE_LUTS
#define OPENPBR_USE_TEXTURE_LUTS 0
#endif

// When texture LUT mode is enabled, validate that all required renderer macros are present.
#if OPENPBR_USE_TEXTURE_LUTS
#ifndef OPENPBR_SAMPLE_2D_TEXTURE
#error "OPENPBR_USE_TEXTURE_LUTS = 1 requires OPENPBR_SAMPLE_2D_TEXTURE to be defined before including openpbr.h."
#endif
#ifndef OPENPBR_SAMPLE_3D_TEXTURE
#error "OPENPBR_USE_TEXTURE_LUTS = 1 requires OPENPBR_SAMPLE_3D_TEXTURE to be defined before including openpbr.h."
#endif
#endif

// ===============
// Fast Math Hooks
// ===============
//
// A few hot-path functions — reciprocal square root, square root, and normalize — can be
// overridden with faster platform-specific implementations (GPU hardware intrinsics, CPU
// approximations, etc.). Define any of the macros below before including openpbr.h;
// any undefined macro falls back to the standard library default.
//
//   OPENPBR_FAST_RCP_SQRT(x)     — fast 1/sqrt(x) for float x
//   OPENPBR_FAST_SQRT(x)         — fast sqrt(x) for float x  (also applied component-wise to vec3)
//   OPENPBR_FAST_NORMALIZE(v)    — fast normalize(v) for vec2 and vec3 v
//
// Example:
//   #define OPENPBR_FAST_RCP_SQRT(x)  fast_rcp_sqrt(x)
//   #define OPENPBR_FAST_SQRT(x)      fast_sqrt(x)
//   #define OPENPBR_FAST_NORMALIZE(v) fast_normalize(v)
//   #include "openpbr.h"
//
// The defaults are declared in impl/openpbr_math.h using #ifndef guards.
//

// ==========================
// Custom saturate() Override
// ==========================
// This library defines saturate(float) and saturate(vec3) shims in the C++, GLSL, and CUDA
// interop backends (MSL and Slang have language built-ins and emit no shim).
//
// OPENPBR_USE_CUSTOM_SATURATE = 0 (Default)
//   - This library provides its own saturate() implementations.
//
// OPENPBR_USE_CUSTOM_SATURATE = 1
//   - This library skips its saturate() definitions entirely.
//   - Use this when your host codebase already defines saturate() to avoid
//     a redefinition error.
//
// Example:
//   #define OPENPBR_USE_CUSTOM_SATURATE 1
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_CUSTOM_SATURATE
#define OPENPBR_USE_CUSTOM_SATURATE 0
#endif

// ==============================
// Custom vec2/vec3/vec4 Override
// ==============================
//
// Several interop backends define GLSL-compatible names to bridge their native types
// and functions to the names used by OpenPBR code:
//
//   MSL, CUDA, Slang  — define vec2/vec3/vec4 as aliases for float2/float3/float4.
//   C++ (GLM)         — also imports abs(), cross(), dot(), mix(), smoothstep(), etc.
//                       from the glm:: namespace via using-declarations.
//   GLSL              — vec2/vec3/vec4 are language built-ins; this setting has no
//                       effect for the GLSL backend.
//
// OPENPBR_USE_CUSTOM_VEC_TYPES = 0 (Default)
//   - The above aliases and using-declarations are emitted by the interop header.
//
// OPENPBR_USE_CUSTOM_VEC_TYPES = 1
//   - All of the above are suppressed.
//   - Use this when the host environment already provides these GLSL-compatible names
//     to avoid redefinition or ambiguous-overload errors.
//   - In C++/GLM mode, GLM functions that take GLM-typed arguments remain accessible
//     through argument-dependent lookup (ADL) even without the using-declarations.
//
// Example:
//   #define OPENPBR_USE_CUSTOM_VEC_TYPES 1
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_CUSTOM_VEC_TYPES
#define OPENPBR_USE_CUSTOM_VEC_TYPES 0
#endif

// ==========================================
// Custom Interop Override (Advanced)
// ==========================================
//
// This OpenPBR BSDF implementation uses abstraction macros for language-specific features
// (OPENPBR_ADDRESS_SPACE_THREAD, OPENPBR_OUT(), OPENPBR_CONSTEXPR_LOCAL, OPENPBR_UINT32,
// OPENPBR_UINT16, vec2/vec3/vec4, etc.). By default it provides its own interop
// definitions in interop/openpbr_interop.h.
//
// Most renderers should leave this at 0 and use OPENPBR_USE_CUSTOM_SATURATE /
// OPENPBR_USE_CUSTOM_VEC_TYPES for targeted suppressions. Set this to 1 only if you need
// to replace the entire interop layer — e.g., your renderer already provides every macro
// OpenPBR requires. No validation is performed when this is 1.
//
// Example:
//   #define OPENPBR_USE_CUSTOM_INTEROP 1
//   #include "your_custom_interop.h"
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_CUSTOM_INTEROP
#define OPENPBR_USE_CUSTOM_INTEROP 0
#endif

// =============================
// Specialization Constant Hooks
// =============================
//
// OpenPBR uses feature toggles to eliminate dead code paths for disabled features at
// compile or specialization time:
//   EnableSheenAndCoat, EnableDispersion, EnableTranslucency, EnableMetallic
//
// A single macro controls access:
//
//   OPENPBR_GET_SPECIALIZATION_CONSTANT(name)
//       Default: true
//       "name" is one of the four toggle names listed above, passed as a token.
//       Override to return the corresponding bool through your renderer's
//       specialization constant pipeline (Vulkan layout(constant_id),
//       Metal function_constant, CPU runtime lookup, etc.).
//
// Example override (before including openpbr.h):
//   #define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) my_get_feature_toggle(name)
//   #include "openpbr.h"
//

// ======================
// Custom Assertion Hooks
// ======================
//
// Pre-define any of these before including openpbr.h to redirect assertions to a custom system:
//
//   OPENPBR_ASSERT(expr, message)        — runtime assertion (independent guard).
//   OPENPBR_ASSERT_UNREACHABLE(message)  — unreachable-branch assertion (independent guard).
//   OPENPBR_STATIC_ASSERT(expr, message) — compile-time assertion (independent guard).
//
// Example:
//   #define OPENPBR_ASSERT(expr, message)        MY_ASSERT(expr, message)
//   #define OPENPBR_ASSERT_UNREACHABLE(message)  MY_ASSERT(false, message)
//   #define OPENPBR_STATIC_ASSERT(expr, message) MY_STATIC_ASSERT(expr, message)
//   #include "openpbr.h"
//
// The defaults are defined with #ifndef guards in all interop backends so they can be
// overridden. The C++ backend additionally suppresses #include <cassert> when both
// OPENPBR_ASSERT and OPENPBR_ASSERT_UNREACHABLE are pre-defined, since assert.h's
// extern "C" linkage specification is illegal at C++ class scope.
//

// =========================
// Language Target Selection
// =========================
//
// When OPENPBR_USE_CUSTOM_INTEROP = 0, this library needs to know which shading language to target.
// You can explicitly set the target language using one of these defines:
//
//   #define OPENPBR_LANGUAGE_TARGET_CPP 1      // C++ with GLM
//   #define OPENPBR_LANGUAGE_TARGET_CUDA 1     // CUDA
//   #define OPENPBR_LANGUAGE_TARGET_GLSL 1     // GLSL (OpenGL/Vulkan)
//   #define OPENPBR_LANGUAGE_TARGET_MSL 1      // Metal Shading Language
//   #define OPENPBR_LANGUAGE_TARGET_SLANG 1    // Slang
//
// Note: There is no dedicated OPENPBR_LANGUAGE_TARGET_HLSL.
// For HLSL-style pipelines, use Slang and set OPENPBR_LANGUAGE_TARGET_SLANG = 1.
//
// If you don't set any explicit target, this library will auto-detect based on compiler defines:
//   - __CUDACC__ -> CUDA
//   - __METAL_VERSION__ -> MSL
//   - __cplusplus -> C++
//   - Otherwise -> GLSL (fallback default)
//
// GLSL is the default fallback because it's the most generic shader language and works
// for most shader compilers that don't define special preprocessor macros.
// Note: Slang does not have a standard preprocessor macro, so it requires explicit setting.
//
// Example explicit target:
//   #define OPENPBR_LANGUAGE_TARGET_GLSL 1
//   #include "openpbr.h"
//
#ifndef OPENPBR_LANGUAGE_TARGET_CPP
#define OPENPBR_LANGUAGE_TARGET_CPP 0
#endif

#ifndef OPENPBR_LANGUAGE_TARGET_CUDA
#define OPENPBR_LANGUAGE_TARGET_CUDA 0
#endif

#ifndef OPENPBR_LANGUAGE_TARGET_GLSL
#define OPENPBR_LANGUAGE_TARGET_GLSL 0
#endif

#ifndef OPENPBR_LANGUAGE_TARGET_MSL
#define OPENPBR_LANGUAGE_TARGET_MSL 0
#endif

#ifndef OPENPBR_LANGUAGE_TARGET_SLANG
#define OPENPBR_LANGUAGE_TARGET_SLANG 0
#endif

#endif  // OPENPBR_SETTINGS_H
