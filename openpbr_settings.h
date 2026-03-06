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
// Lookup Table Access Mode Configuration
// ======================================
//
// The OpenPBR BSDF uses precomputed lookup tables (LUTs) for:
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
//   - Requires the following macros to be defined before including openpbr.h:
//       GET_2D_TEXTURE(handle)  — resolves a texture handle to a 2D sampler
//       GET_3D_TEXTURE(handle)  — resolves a texture handle to a 3D sampler
//       GET_BINDING(name)       — accesses a shader binding by name
//     These are renderer-specific. Eclair defines them in texture_func.h and
//     glsl_interop_begin.h. The headers will emit a clear error if they are missing.
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
#ifndef GET_2D_TEXTURE
#error "OPENPBR_USE_TEXTURE_LUTS = 1 requires GET_2D_TEXTURE to be defined before including openpbr.h."
#endif
#ifndef GET_3D_TEXTURE
#error "OPENPBR_USE_TEXTURE_LUTS = 1 requires GET_3D_TEXTURE to be defined before including openpbr.h."
#endif
#ifndef GET_BINDING
#error "OPENPBR_USE_TEXTURE_LUTS = 1 requires GET_BINDING to be defined before including openpbr.h."
#endif
#endif

// ====================================
// Energy Table Data Type Configuration
// ====================================
//
// Energy compensation tables store normalized values in the [0, 65535] range.
// This setting controls the integer type used for storage:
//
// OPENPBR_ENERGY_TABLES_USE_UINT16 = 0 (uint, Default)
//   - Uses 32-bit unsigned integers (uint)
//   - Compatible with all shading languages (GLSL, MSL, C++, CUDA, Slang)
//   - Larger memory footprint (4 bytes per element)
//   - Default for maximum portability
//
// OPENPBR_ENERGY_TABLES_USE_UINT16 = 1 (unsigned short)
//   - Uses 16-bit unsigned short integers
//   - Only supported in MSL, C++, CUDA, and Slang (NOT GLSL core)
//   - More memory efficient (2 bytes per element)
//   - Reduces shader binary size and memory bandwidth
//
// Note: The actual data values remain in [0, 65535] range regardless of storage type.
//       The normalization constant is always 65535.0f.
//
// TODO: Automatically set this based on selected shading language in the future.
//
// Example for Metal/C++/CUDA:
//   #define OPENPBR_ENERGY_TABLES_USE_UINT16 1
//   #include "openpbr.h"
//
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#define OPENPBR_ENERGY_TABLES_USE_UINT16 0
#endif

// ============================================
// Fast Math Approximations Configuration
// ============================================
//
// The OpenPBR BSDF uses a few hot-path math operations that can safely be accelerated
// with approximations in some cases: reciprocal square root, square root, and normalize.
//
// OPENPBR_USE_FAST_MATH_APPROXIMATIONS = 0 (Standard Math, Default)
//   - Uses precise standard library implementations: 1/sqrt(x), sqrt(x), normalize(v)
//   - Self-contained: no additional includes or user-defined functions required
//
// OPENPBR_USE_FAST_MATH_APPROXIMATIONS = 1 (Fast Approximations)
//   - openpbr_math.h skips its standard fallback definitions
//   - The caller must define these functions before including openpbr.h:
//       INLINE_FUNCTION float openpbr_fast_rcp_sqrt(const float x)  // fast 1/sqrt
//       INLINE_FUNCTION float openpbr_fast_sqrt(const float x)      // fast sqrt
//       INLINE_FUNCTION vec3  openpbr_fast_sqrt(const vec3 v)       // fast componentwise sqrt
//       INLINE_FUNCTION vec3  openpbr_fast_normalize(const vec3 v)  // fast normalize
//
// Example (define your implementations, then include openpbr.h):
//   #define OPENPBR_USE_FAST_MATH_APPROXIMATIONS 1
//   INLINE_FUNCTION float openpbr_fast_rcp_sqrt(const float x) { ... }
//   ...
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_FAST_MATH_APPROXIMATIONS
#define OPENPBR_USE_FAST_MATH_APPROXIMATIONS 0
#endif

// ==========================================
// Cross-Language Interop Macro Configuration
// ==========================================
//
// OpenPBR BSDF is designed to work across multiple shading languages:
//   - GLSL (OpenGL/Vulkan shaders)
//   - MSL (Metal Shading Language)
//   - C++ (CPU path tracers with GLM)
//   - CUDA (NVIDIA GPU compute)
//   - Slang (cross-platform shading language)
//
// The BSDF uses abstraction macros for language-specific features:
//   - ADDRESS_SPACE_THREAD (memory qualifiers)
//   - OUT(), INOUT(), CONST_REF() (parameter passing)
//   - CONSTEXPR_LOCAL, CONSTEXPR_GLOBAL (constants)
//   - vec2, vec3, vec4 (math types)
//   - And more...
//
// By default, OpenPBR provides its own interop macro definitions in:
//   interop/openpbr_interop.h
//
// To use your own custom interop system (e.g., from an existing renderer):
//   1. Set OPENPBR_USE_CUSTOM_INTEROP = 1
//   2. Define all required macros before including openpbr.h
//   3. OpenPBR will validate that macros are defined but won't define them itself
//
// Example with custom macros:
//   #define OPENPBR_USE_CUSTOM_INTEROP 1
//   #include "your_custom_interop.h"
//   #include "openpbr.h"
//
#ifndef OPENPBR_USE_CUSTOM_INTEROP
#define OPENPBR_USE_CUSTOM_INTEROP 0
#endif

// =========================
// Language Target Selection
// =========================
//
// When OPENPBR_USE_CUSTOM_INTEROP = 0, OpenPBR needs to know which shading language to target.
// You can explicitly set the target language using one of these defines:
//
//   #define OPENPBR_LANGUAGE_TARGET_CPP 1      // C++ with GLM
//   #define OPENPBR_LANGUAGE_TARGET_CUDA 1     // CUDA
//   #define OPENPBR_LANGUAGE_TARGET_GLSL 1     // GLSL (OpenGL/Vulkan)
//   #define OPENPBR_LANGUAGE_TARGET_MSL 1      // Metal Shading Language
//   #define OPENPBR_LANGUAGE_TARGET_SLANG 1    // Slang
//
// If you don't set any explicit target, OpenPBR will auto-detect based on compiler defines:
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
