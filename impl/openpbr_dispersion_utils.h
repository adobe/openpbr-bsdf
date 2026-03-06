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

#ifndef OPENPBR_DISPERSION_UTILS_H
#define OPENPBR_DISPERSION_UTILS_H

#include "openpbr_math.h"

CONSTEXPR_GLOBAL float OpenPBR_FraunhoferLine_C_nm = 656.3f;
CONSTEXPR_GLOBAL float OpenPBR_FraunhoferLine_D_nm = 589.3f;  // average of sodium lines D1 and D2
CONSTEXPR_GLOBAL float OpenPBR_FraunhoferLine_d_nm = 587.6f;  // helium line D3
CONSTEXPR_GLOBAL float OpenPBR_FraunhoferLine_F_nm = 486.1f;

CONSTEXPR_GLOBAL float OpenPBR_DispersionWavelengthLong_nm = OpenPBR_FraunhoferLine_C_nm;
CONSTEXPR_GLOBAL float OpenPBR_DispersionWavelengthMedium_nm = OpenPBR_FraunhoferLine_d_nm;  // Abbe originally used D, but today d is more common.
CONSTEXPR_GLOBAL float OpenPBR_DispersionWavelengthShort_nm = OpenPBR_FraunhoferLine_F_nm;

// Converts OpenPBR "disperion" parameter to Abbe number V_d, the reciprocal of the relative mean dispersion:
// V_d = (n_d - 1) / (n_F - n_C) (Abbe originally used D, but today d is more common). The mapping is
// V_d = 20 / "dispersion". This maps all of the realistic glass Abbe numbers, which range from about
// 20 to 95, into [0, 1], where 0 corresponds to no dispersion (infinite Abbe number) and 1 corresponds
// to the highest amount of realistic dispersion (Abbe number 20). By being based on the reciprocal of the
// Abbe number, the parameterization ensures that visible color spread increases approximately linearly
// with the parameter (the reciprocal of the Abbe number is proportional to the change in the index across
// the spectrum and the angle of refraction varies approximately linearly with the index at these scales),
// and the inverse mapping from Abbe number takes the same simple form (the parameter can easily be
// derived from a measured Abbe number V_d by simply taking 20 / V_d). The mapping from Abbe number to
// wavelength-dependent indexes is based on the basic form of Cauchy's equation (which is about the best
// we can do without additional information such as the partial dispersions).
float openpbr_abbe_number_V_d(const float dispersion_param)
{
    return 20.0f / dispersion_param;
}

// A and B are derived from n_d and V_d in such a way that the resultant
// indexes have an Abbe number of exactly V_d and n_lambda(A, B, d) = n_d.
float openpbr_cauchy_equation_B(const float n_d, const float V_d)
{
    return (n_d - 1.0f) /
           (V_d * (openpbr_square(1.0f / OpenPBR_DispersionWavelengthShort_nm) - openpbr_square(1.0f / OpenPBR_DispersionWavelengthLong_nm)));
}
float openpbr_cauchy_equation_A(const float n_d, const float B)
{
    return n_d - B * openpbr_square(1.0f / OpenPBR_DispersionWavelengthMedium_nm);
}

float openpbr_cauchy_equation(const float A, const float B, const float lambda_nm)
{
    const float n_lambda = A + B * openpbr_square(1.0f / lambda_nm);
    return n_lambda;
}

// Applies dispersion to an IOR based on the value of the OpenPBR "dispersion" parameter.
// Values of "dispersion" in [0, 1] map to plausible Abbe numbers in [95+, 20].
// Valid values of the "dispersion" parameter are those in [0, ~20+].
// Dispersion 0 corresponds to no dispersion at all and should be handled separately by the caller.
// IOR 1 cannot produce dispersion and should also be handled separately by the caller.
float openpbr_dispersion_adjusted_ior(const float original_ior, const float dispersion_param, const float wavelength_nm)
{
    ASSERT(dispersion_param > 0.0f && wavelength_nm > 0.0f, "This function assumes that there is non-zero dispersion");

    const bool original_ior_was_less_than_one = original_ior < 1.0f;
    const float ior_above_one = original_ior_was_less_than_one ? 1.0f / original_ior : original_ior;

    const float V_d = openpbr_abbe_number_V_d(dispersion_param);
    const float B = openpbr_cauchy_equation_B(ior_above_one, V_d);
    const float A = openpbr_cauchy_equation_A(ior_above_one, B);

    const float adjusted_ior = openpbr_cauchy_equation(A, B, wavelength_nm);

    return original_ior_was_less_than_one ? 1.0f / adjusted_ior : adjusted_ior;
}

#endif  // !OPENPBR_DISPERSION_UTILS_H
