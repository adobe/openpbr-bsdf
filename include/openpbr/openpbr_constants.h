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

#ifndef OPENPBR_CONSTANTS_H
#define OPENPBR_CONSTANTS_H

OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_FloatMin = 1.175494351e-38f;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_LargestFloatBelowOne = 0.999999940f;  // 1 - 2^-24

OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_Pi = 3.14159265f;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_TwoPi = 6.28318531f;        // 2 * Pi
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_HalfPi = 1.57079633f;       // Pi / 2
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_PiOverFour = 0.785398163f;  // Pi / 4
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_RcpPi = 0.318309886f;       // 1 / Pi
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_RcpTwoPi = 0.159154943f;    // 1 / (2 * Pi) = 0.5 / Pi
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_RcpFourPi = 0.0795774715f;  // 1 / (4 * Pi) = 0.25 / Pi

OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_NumRgbChannels = 3;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_VacuumIor = 1.0f;
OPENPBR_CONSTEXPR_GLOBAL vec3 OpenPBR_BaseRgbWavelengths_nm = vec3(620.0f, 540.0f, 450.0f);

#endif
