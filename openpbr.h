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

#ifndef OPENPBR_H
#define OPENPBR_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// OpenPBR BSDF implementation from Adobe's Eclair path tracer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// This is Adobe's implementation of the OpenPBR surface shading model, extracted
// from Eclair, Adobe's in-house production path tracer. OpenPBR is an open standard
// for physically-based material description; see https://academysoftwarefoundation.github.io/OpenPBR/
//
// The system is designed to be easy to integrate: this single header is the only
// include needed. It brings in everything - types, constants, settings, interop,
// and the full BSDF API - with no external dependencies (C++ users must pre-include GLM).
//
// The implementation is written in portable GLSL-like syntax and compiles unmodified
// for GLSL, C++, MSL, CUDA, and Slang via a thin cross-language interop layer.
// The correct backend is selected automatically based on compiler-defined macros,
// or can be overridden explicitly via compile-time settings.
//
// Sub-headers included by this file, in order:
//   - openpbr_settings.h              compile-time configuration and feature flags
//   - openpbr_data_constants.h        LUT dimensions and LUT ID constants
//   - interop/openpbr_interop.h       cross-language macro layer
//   - openpbr_resolved_inputs.h       OpenPBR_ResolvedInputs struct and default initializer
//   - openpbr_constants.h             physical and material constants
//   - openpbr_diffuse_specular.h      OpenPBR_DiffuseSpecular response type
//   - openpbr_bsdf_lobe_type.h        BSDF lobe type flags
//   - openpbr_volume_homogeneous.h    Volume type exposed by OpenPBR_PreparedBsdf
//   - openpbr_api.h                   public API: prepare, eval, sample, pdf

// Configuration and compile-time settings
#include "openpbr_settings.h"

// LUT dimensions and LUT ID constants (public API for texture mode)
#include "openpbr_data_constants.h"

// Cross-language interop macros
#include "interop/openpbr_interop.h"

// Material input structure and default-initialization function
#include "openpbr_resolved_inputs.h"

// Material constant definitions
#include "openpbr_constants.h"

// Diffuse/specular component types
#include "openpbr_diffuse_specular.h"

// BSDF lobe type flags
#include "openpbr_bsdf_lobe_type.h"

// Volume type (OpenPBR_HomogeneousVolume) exposed via OpenPBR_PreparedBsdf
#include "openpbr_volume_homogeneous.h"

// Main BSDF evaluation API (prepare, sample, eval, pdf)
#include "openpbr_api.h"

#endif  // OPENPBR_H
