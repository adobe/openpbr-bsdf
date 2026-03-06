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

// Only include the data arrays when not using texture-based lookups
#if !OPENPBR_USE_TEXTURE_LUTS

#include "openpbr_data_constants.h"

// ===============================================================================================
// Energy Compensation Lookup Tables
// ===============================================================================================
//
// These tables contain precomputed energy compensation values for microfacet multiple scattering,
// generated using a custom Monte Carlo integration tool.
//
// All tables use OpenPBR_EnergyTableElement (uint16 or uint32) normalized to [0, 65535] range.
//
// ===============================================================================================

// Ideal Dielectric Energy Complement (3D: 32x32x32)
// Parameterized by: IOR, roughness (alpha), cos(theta)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_IdealDielectricEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_energy_complement_data.h"
    };

// Ideal Dielectric Average Energy Complement (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealDielectricAverageEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_avg_energy_complement_data.h"
};

// Ideal Dielectric Reflection Ratio (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealDielectricReflectionRatio_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_dielectric_reflection_ratio_data.h"
};

// Opaque Dielectric Energy Complement (3D: 32x32x32)
// Parameterized by: IOR, roughness (alpha), cos(theta)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_OpaqueDielectricEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_opaque_dielectric_energy_complement_data.h"
    };

// Opaque Dielectric Average Energy Complement (2D: 32x32)
// Parameterized by: IOR, roughness (alpha)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement
    OpenPBR_OpaqueDielectricAverageEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_opaque_dielectric_avg_energy_complement_data.h"
    };

// Ideal Metal Energy Complement (2D: 32x32)
// Parameterized by: roughness (alpha), cos(theta)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealMetalEnergyComplement_Array[OpenPBR_EnergyTableSize * OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_metal_energy_complement_data.h"
};

// Ideal Metal Average Energy Complement (1D: 32)
// Parameterized by: roughness (alpha)
CONSTEXPR_GLOBAL OpenPBR_EnergyTableElement OpenPBR_IdealMetalAverageEnergyComplement_Array[OpenPBR_EnergyTableSize] = {
#include "openpbr_ideal_metal_avg_energy_complement_data.h"
};

#endif  // !OPENPBR_USE_TEXTURE_LUTS

#endif  // OPENPBR_ENERGY_ARRAYS_H
