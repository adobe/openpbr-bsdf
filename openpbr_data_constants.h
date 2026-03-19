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

#ifndef OPENPBR_DATA_CONSTANTS_H
#define OPENPBR_DATA_CONSTANTS_H

// LUT dimensions and LUT ID constants.
//
// This file is part of the public API and is intentionally dependency-free.
// It can be included by host-side CPU code in isolation (e.g. texture allocation
// code that needs table dimensions or LUT IDs) without pulling in openpbr.h,
// the interop layer, or any shading-language-specific macros.
// All values are plain #defines for the same reason.
//
// The actual precomputed table data lives in impl/data/:
//   - openpbr_energy_arrays.h / *_data.h  - energy compensation tables (array mode)
//   - openpbr_ltc_array.h / openpbr_ltc_data.h  - LTC sheen table (array mode)
// Those files are included automatically by openpbr.h when OPENPBR_USE_TEXTURE_LUTS = 0.

// =======================
// Lookup Table Dimensions
// =======================

#define OpenPBR_EnergyTableSize 32  // energy compensation tables
#define OpenPBR_LTCTableSize    32  // LTC sheen table

// ================
// LUT ID Constants
// ================

// Integer IDs for all 8 OpenPBR precomputed lookup textures.
// Used as the 'id' argument to OPENPBR_SAMPLE_2D_TEXTURE / OPENPBR_SAMPLE_3D_TEXTURE
// (texture mode), or as the array selector in openpbr_energy_array_access.h (array mode).
// Renderers storing OpenPBR textures consecutively can use these IDs directly as slot offsets.
//
// IDs 0-6: energy compensation tables (used by openpbr_look_up_* functions)
// ID  7  : LTC sheen table (sampled directly by the fuzz lobe)
// clang-format off
#define OpenPBR_LutId                                          int
#define OpenPBR_LutId_IdealDielectricEnergyComplement          0  // 3D
#define OpenPBR_LutId_IdealDielectricAverageEnergyComplement   1  // 2D
#define OpenPBR_LutId_IdealDielectricReflectionRatio           2  // 2D
#define OpenPBR_LutId_OpaqueDielectricEnergyComplement         3  // 3D
#define OpenPBR_LutId_OpaqueDielectricAverageEnergyComplement  4  // 2D
#define OpenPBR_LutId_IdealMetalEnergyComplement               5  // 2D
#define OpenPBR_LutId_IdealMetalAverageEnergyComplement        6  // 2D (1D data stored as thin 2D)
#define OpenPBR_LutId_LTC                                      7  // 2D
// clang-format on

#endif  // OPENPBR_DATA_CONSTANTS_H
