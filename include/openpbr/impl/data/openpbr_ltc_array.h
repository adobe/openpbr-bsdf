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

#ifndef OPENPBR_LTC_ARRAY_H
#define OPENPBR_LTC_ARRAY_H

// Only include the data array when not using texture-based lookups.
#if !OPENPBR_USE_TEXTURE_LUTS

#include "../../openpbr_data_constants.h"

// ================================================================================================================
// Linearly Transformed Cosines (LTC) Lookup Table
// ================================================================================================================
//
// This table contains precomputed LTC coefficients for the Disney sheen (fuzz) model. The data is reproduced from:
//     https://github.com/tizian/ltc-sheen/blob/master/pbrt-v3/src/materials/sheenltc.cpp
// by Tizian Zeltner, Brent Burley, and Matt Jen-Yuan Chiang, licensed under the Apache License, Version 2.0:
//     https://github.com/tizian/ltc-sheen/blob/master/LICENSE
//
// The table is parameterized by:
//   - Elevation angle (as cos(theta)) - horizontal axis
//   - Sheen roughness (alpha = sqrt(sigma)) - vertical axis
//
// Dimensions: 32 x 32 (width x height)
// Format: vec3 per entry (a_inv, b_inv, R)
//   - a_inv, b_inv: LTC transformation matrix coefficients
//   - R: overall reflectance
//
// Total size: 1024 vec3 entries = 3072 floats
//
// ================================================================================================================

OPENPBR_CONSTEXPR_GLOBAL vec3 OpenPBR_LTC_Array[OpenPBR_LTCTableSize * OpenPBR_LTCTableSize] = {
#include "openpbr_ltc_data.h"
};

#endif  // !OPENPBR_USE_TEXTURE_LUTS

#endif  // OPENPBR_LTC_ARRAY_H
