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

#ifndef OPENPBR_THIN_WALL_SPECULAR_TRANSMISSION_LOBE_H
#define OPENPBR_THIN_WALL_SPECULAR_TRANSMISSION_LOBE_H

#include "../openpbr_diffuse_specular.h"
#include "openpbr_math.h"

// Specify the type of lobe that is used for the thin-wall specular reflection and transmission.
#include "openpbr_constant_reflection_coefficient.h"
#include "openpbr_microfacet_multiple_scattering_data.h"
#include "openpbr_vndf_microfacet_distributions.h"
#define OPENPBR_MICROFACET_DISTRIBUTION_TYPE OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution
#define OPENPBR_REFLECTION_COEFFICIENT_TYPE OpenPBR_ConstantReflectionCoefficient
#define OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE                                                                                                         \
    OpenPBR_MinimalMicrofacetReflectionLobe_AnisotropicGGXSmithVNDFMicrofacetDistribution_ConstantReflectionCoefficient
#include "openpbr_generic_minimal_microfacet_lobe.h"
#undef OPENPBR_REFLECTION_COEFFICIENT_TYPE
#undef OPENPBR_MICROFACET_DISTRIBUTION_TYPE

//////////////////////////////////////
// ThinWallSpecularTransmissionLobe //
//////////////////////////////////////

// A thin-wall specular lobe that only handles transmission via a flipped minimal microfacet lobe.
struct OpenPBR_ThinWallSpecularTransmissionLobe
{
    OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE flipped_lobe;
};

#undef OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE

// Initialize the inner specular lobe with flipped normal, flipped microfacet-distribution basis, and reflected view direction.
void openpbr_initialize_lobe(ADDRESS_SPACE_THREAD INOUT(OpenPBR_ThinWallSpecularTransmissionLobe) lobe,
                             const vec3 normal_ff,
                             const OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr,
                             const vec3 trans_color)
{
    const vec3 flipped_normal = -normal_ff;
    OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution flipped_microfacet_distr = microfacet_distr;
    flipped_microfacet_distr.basis_ff.n *= -1.0f;  // only necessary to flip the normal
    openpbr_initialize_lobe(
        lobe.flipped_lobe, flipped_normal, flipped_microfacet_distr, MAKE_STRUCT_1(OpenPBR_ConstantReflectionCoefficient, trans_color));
}

#define OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE OpenPBR_ThinWallSpecularTransmissionLobe
#include "openpbr_generic_flipped_lobe.h"
#undef OPENPBR_FLIPPED_WRAPPER_LOBE_TYPE

#endif  // OPENPBR_THIN_WALL_SPECULAR_TRANSMISSION_LOBE_H
