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

#include "../../openpbr_settings.h"

// =======================
// Lookup Table Dimensions
// =======================

// Table dimensions for energy compensation tables
CONSTEXPR_GLOBAL int OpenPBR_EnergyTableSize = 32;

// Table dimensions for LTC (Linearly Transformed Cosines) sheen table from Disney
CONSTEXPR_GLOBAL int OpenPBR_LTCTableSize = 32;

// ======================
// Energy Table Data Type
// ======================

// Define the integer type used for energy table storage based on the selected configuration.
// Always stores values in [0, 65535] range, normalized by dividing by 65535.0.
#if OPENPBR_ENERGY_TABLES_USE_UINT16
#define OpenPBR_EnergyTableElement unsigned short
#else
#define OpenPBR_EnergyTableElement uint
#endif

// ===============================
// Energy Table Indexing Constants
// ===============================

// Energy table IDs for unified lookup - must match texture binding indices in texture mode
#define OpenPBR_EnergyTableId int
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_IdealDielectricEnergyComplement = 0;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_IdealDielectricAverageEnergyComplement = 1;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_IdealDielectricReflectionRatio = 2;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_OpaqueDielectricEnergyComplement = 3;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_OpaqueDielectricAverageEnergyComplement = 4;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_IdealMetalEnergyComplement = 5;
CONSTEXPR_GLOBAL OpenPBR_EnergyTableId OpenPBR_EnergyTableId_IdealMetalAverageEnergyComplement = 6;

#endif  // OPENPBR_DATA_CONSTANTS_H
