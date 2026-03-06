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

#ifndef OPENPBR_ENERGY_ARRAY_ACCESS_H
#define OPENPBR_ENERGY_ARRAY_ACCESS_H

#if !OPENPBR_USE_TEXTURE_LUTS

#include "openpbr_data_constants.h"
#include "openpbr_energy_arrays.h"

// Energy table array access with ID-based selection
//
// Indexing matches C array layout where source arrays are declared as array[ior][alpha][costheta]
// and flattened in row-major order with costheta fastest (rightmost index contiguous in memory):
//   - 1D: array[x] -> flat index = x
//   - 2D: array[x][y] -> flat index = x * N + y
//   - 3D: array[x][y][z] -> flat index = x * N*N + y * N + z

// Convert normalized integer [0, 65535] to float [0.0, 1.0]
// Note: Works with both uint and unsigned short storage types
float openpbr_unorm_to_float(const OpenPBR_EnergyTableElement unorm_table_value)
{
    CONSTEXPR_LOCAL float UnormOne = 65535.0f;
    CONSTEXPR_LOCAL float InverseUnormOne = 1.0f / UnormOne;
    return float(unorm_table_value) * InverseUnormOne;
}

// Fetch raw normalized value from energy table by ID and flat index
OpenPBR_EnergyTableElement openpbr_fetch_lut_unorm(const OpenPBR_EnergyTableId lut_id, const int flat_index)
{
    switch (lut_id)
    {
        case OpenPBR_EnergyTableId_IdealDielectricEnergyComplement:
            return OpenPBR_IdealDielectricEnergyComplement_Array[flat_index];
        case OpenPBR_EnergyTableId_IdealDielectricAverageEnergyComplement:
            return OpenPBR_IdealDielectricAverageEnergyComplement_Array[flat_index];
        case OpenPBR_EnergyTableId_IdealDielectricReflectionRatio:
            return OpenPBR_IdealDielectricReflectionRatio_Array[flat_index];
        case OpenPBR_EnergyTableId_OpaqueDielectricEnergyComplement:
            return OpenPBR_OpaqueDielectricEnergyComplement_Array[flat_index];
        case OpenPBR_EnergyTableId_OpaqueDielectricAverageEnergyComplement:
            return OpenPBR_OpaqueDielectricAverageEnergyComplement_Array[flat_index];
        case OpenPBR_EnergyTableId_IdealMetalEnergyComplement:
            return OpenPBR_IdealMetalEnergyComplement_Array[flat_index];
        case OpenPBR_EnergyTableId_IdealMetalAverageEnergyComplement:
            return OpenPBR_IdealMetalAverageEnergyComplement_Array[flat_index];
        default:
            ASSERT_UNREACHABLE("Invalid energy table ID");
            // Defensive fallback - should never get here
            return 0;  // Should never happen
    }
}

// Fetch and decode energy table value to float [0.0, 1.0] in one step
float openpbr_fetch_lut_float(const OpenPBR_EnergyTableId lut_id, const int flat_index)
{
    return openpbr_unorm_to_float(openpbr_fetch_lut_unorm(lut_id, flat_index));
}

// Helper functions to compute flat index from 2D coordinates
int openpbr_flatten_index_2d(const int ix, const int iy)
{
    CONSTEXPR_LOCAL int N = OpenPBR_EnergyTableSize;
    return ix * N + iy;
}

// Helper functions to compute flat index from 3D coordinates
int openpbr_flatten_index_3d(const int ix, const int iy, const int iz)
{
    CONSTEXPR_LOCAL int N = OpenPBR_EnergyTableSize;
    CONSTEXPR_LOCAL int NN = N * N;
    return ix * NN + iy * N + iz;
}

// Linear interpolation for 1D energy tables
float openpbr_energy_array_lookup_linear(const OpenPBR_EnergyTableId lut_id, const float clamped_exact_index_x)
{
    ASSERT(clamped_exact_index_x >= 0.0f && clamped_exact_index_x <= float(OpenPBR_EnergyTableSize - 1), "Index must be clamped to valid range");

    const int i0 = int(floor(clamped_exact_index_x));
    const int i1 = min(i0 + 1, OpenPBR_EnergyTableSize - 1);
    const float t = clamped_exact_index_x - float(i0);

    const float v0 = openpbr_fetch_lut_float(lut_id, i0);
    const float v1 = openpbr_fetch_lut_float(lut_id, i1);

    return mix(v0, v1, t);
}

// Bilinear interpolation for 2D energy tables
float openpbr_energy_array_lookup_bilinear(const OpenPBR_EnergyTableId lut_id, const float clamped_exact_index_x, const float clamped_exact_index_y)
{
    ASSERT(clamped_exact_index_x >= 0.0f && clamped_exact_index_x <= float(OpenPBR_EnergyTableSize - 1), "Index X must be clamped to valid range");
    ASSERT(clamped_exact_index_y >= 0.0f && clamped_exact_index_y <= float(OpenPBR_EnergyTableSize - 1), "Index Y must be clamped to valid range");

    const int ix0 = int(floor(clamped_exact_index_x));
    const int iy0 = int(floor(clamped_exact_index_y));
    const int ix1 = min(ix0 + 1, OpenPBR_EnergyTableSize - 1);
    const int iy1 = min(iy0 + 1, OpenPBR_EnergyTableSize - 1);
    const float tx = clamped_exact_index_x - float(ix0);
    const float ty = clamped_exact_index_y - float(iy0);

    const float v00 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_2d(ix0, iy0));
    const float v01 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_2d(ix0, iy1));
    const float v10 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_2d(ix1, iy0));
    const float v11 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_2d(ix1, iy1));

    const float v0 = mix(v00, v10, tx);
    const float v1 = mix(v01, v11, tx);

    return mix(v0, v1, ty);
}

// Trilinear interpolation for 3D energy tables
float openpbr_energy_array_lookup_trilinear(const OpenPBR_EnergyTableId lut_id,
                                            const float clamped_exact_index_x,
                                            const float clamped_exact_index_y,
                                            const float clamped_exact_index_z)
{
    ASSERT(clamped_exact_index_x >= 0.0f && clamped_exact_index_x <= float(OpenPBR_EnergyTableSize - 1), "Index X must be clamped to valid range");
    ASSERT(clamped_exact_index_y >= 0.0f && clamped_exact_index_y <= float(OpenPBR_EnergyTableSize - 1), "Index Y must be clamped to valid range");
    ASSERT(clamped_exact_index_z >= 0.0f && clamped_exact_index_z <= float(OpenPBR_EnergyTableSize - 1), "Index Z must be clamped to valid range");

    const int ix0 = int(floor(clamped_exact_index_x));
    const int iy0 = int(floor(clamped_exact_index_y));
    const int iz0 = int(floor(clamped_exact_index_z));
    const int ix1 = min(ix0 + 1, OpenPBR_EnergyTableSize - 1);
    const int iy1 = min(iy0 + 1, OpenPBR_EnergyTableSize - 1);
    const int iz1 = min(iz0 + 1, OpenPBR_EnergyTableSize - 1);
    const float tx = clamped_exact_index_x - float(ix0);
    const float ty = clamped_exact_index_y - float(iy0);
    const float tz = clamped_exact_index_z - float(iz0);

    const float v000 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix0, iy0, iz0));
    const float v001 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix0, iy0, iz1));
    const float v010 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix0, iy1, iz0));
    const float v011 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix0, iy1, iz1));
    const float v100 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix1, iy0, iz0));
    const float v101 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix1, iy0, iz1));
    const float v110 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix1, iy1, iz0));
    const float v111 = openpbr_fetch_lut_float(lut_id, openpbr_flatten_index_3d(ix1, iy1, iz1));

    const float v00 = mix(v000, v100, tx);
    const float v01 = mix(v001, v101, tx);
    const float v10 = mix(v010, v110, tx);
    const float v11 = mix(v011, v111, tx);

    const float v0 = mix(v00, v10, ty);
    const float v1 = mix(v01, v11, ty);

    return mix(v0, v1, tz);
}

#endif  // !OPENPBR_USE_TEXTURE_LUTS

#endif  // OPENPBR_ENERGY_ARRAY_ACCESS_H
