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

#ifndef OPENPBR_BSDF_LOBE_TYPE_H
#define OPENPBR_BSDF_LOBE_TYPE_H

#ifndef OPENPBR_UINT32
#error "OPENPBR_UINT32 must be defined by the interop layer before including openpbr_bsdf_lobe_type.h."
#endif

#define OpenPBR_BsdfLobeType OPENPBR_UINT32
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeNone = 0;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeReflection = 1 << 0;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeTransmission = 1 << 1;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeDiffuse = 1 << 2;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeGlossy = 1 << 3;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeSpecular = 1 << 4;
OPENPBR_CONSTEXPR_GLOBAL OpenPBR_BsdfLobeType OpenPBR_BsdfLobeTypeVolume = 1 << 5;

// Swap reflection and transmission in the OpenPBR_BsdfLobeType bitmask.
OpenPBR_BsdfLobeType openpbr_swap_reflect_trans_flags(OpenPBR_BsdfLobeType bsdf_lobe_type)
{
    const bool has_r = bool(bsdf_lobe_type & OpenPBR_BsdfLobeTypeReflection);
    const bool has_t = bool(bsdf_lobe_type & OpenPBR_BsdfLobeTypeTransmission);
    bsdf_lobe_type &= ~(OpenPBR_BsdfLobeTypeReflection | OpenPBR_BsdfLobeTypeTransmission);
    if (has_r)
        bsdf_lobe_type |= OpenPBR_BsdfLobeTypeTransmission;
    if (has_t)
        bsdf_lobe_type |= OpenPBR_BsdfLobeTypeReflection;
    return bsdf_lobe_type;
}

#endif
