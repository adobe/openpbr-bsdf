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

#ifndef OPENPBR_ENERGY_ARRAYS_H
#define OPENPBR_ENERGY_ARRAYS_H

#include "../../openpbr_settings.h"

// Only include the data arrays when not using texture-based lookups
#if !OPENPBR_USE_TEXTURE_LUTS

#include "../../openpbr_data_constants.h"

// ======================
// Energy Table Data Type
// ======================
//
// Storage type for the hardcoded energy table arrays.
// Values are in [0, 65535], normalized to [0, 1] by dividing by 65535.0.
//
// OPENPBR_ENERGY_TABLES_USE_UINT16 is set by each per-language interop file:
//   - 1 uses OPENPBR_UINT16 (16-bit)
//   - 0 uses OPENPBR_UINT   (32-bit)
//
// OPENPBR_UINT and OPENPBR_UINT16 are defined by the active interop backend.
// Examples:
//   - C++/CUDA: OPENPBR_UINT = std::uint32_t, OPENPBR_UINT16 = std::uint16_t
//   - MSL:      OPENPBR_UINT = uint,          OPENPBR_UINT16 = ushort
//   - Slang:    OPENPBR_UINT = uint,          OPENPBR_UINT16 = unsigned short
//   - GLSL:     OPENPBR_UINT = uint (OPENPBR_ENERGY_TABLES_USE_UINT16 is 0)
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#error "OPENPBR_ENERGY_TABLES_USE_UINT16 is not defined. An OpenPBR interop header must be included before this file (normally via openpbr.h)."
#endif
#if OPENPBR_ENERGY_TABLES_USE_UINT16
#ifndef OPENPBR_UINT16
#error "OPENPBR_UINT16 must be defined by the interop layer when OPENPBR_ENERGY_TABLES_USE_UINT16 = 1."
#endif
#define OpenPBR_EnergyTableElement OPENPBR_UINT16
#else
#ifndef OPENPBR_UINT32
#error "OPENPBR_UINT32 must be defined by the interop layer when OPENPBR_ENERGY_TABLES_USE_UINT16 = 0."
#endif
#define OpenPBR_EnergyTableElement OPENPBR_UINT32
#endif

// =================================
// Energy Compensation Lookup Tables
// =================================
//
// Precomputed energy compensation values for microfacet multiple scattering,
// generated using a custom Monte Carlo integration tool.
// Stored as OpenPBR_EnergyTableElement values in [0, 65535], normalized by dividing by 65535.0.

// Ideal Dielectric Energy Complement (3D: 32x32x32)
// Parameterized by: IOR, roughness (alpha), cos(theta)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_IdealDielectricEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_energy_complement_data.h"
    };

// Ideal Dielectric Average Energy Complement (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_IdealDielectricAverageEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_avg_energy_complement_data.h"
    };

// Ideal Dielectric Reflection Ratio (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealDielectricReflectionRatio_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_reflection_ratio_data.h"
};

// Opaque Dielectric Energy Complement (3D: 32x32x32)
// Parameterized by: IOR, roughness (alpha), cos(theta)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_OpaqueDielectricEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_opaque_dielectric_energy_complement_data.h"
    };

// Opaque Dielectric Average Energy Complement (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_OpaqueDielectricAverageEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_opaque_dielectric_avg_energy_complement_data.h"
    };

// Ideal Metal Energy Complement (2D: 32x32)
// Parameterized by: roughness (alpha), cos(theta)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealMetalEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_metal_energy_complement_data.h"
};

// Ideal Metal Average Energy Complement (1D: 32)
// Parameterized by: roughness (alpha)
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealMetalAverageEnergyComplement_Array[OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_metal_avg_energy_complement_data.h"
};

#endif  // !OPENPBR_USE_TEXTURE_LUTS

#endif  // OPENPBR_ENERGY_ARRAYS_H
