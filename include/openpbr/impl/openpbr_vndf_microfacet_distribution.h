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

#ifndef OPENPBR_VNDF_MICROFACET_DISTRIBUTION_H
#define OPENPBR_VNDF_MICROFACET_DISTRIBUTION_H

#include "../openpbr_basis.h"
#include "openpbr_lobe_utils.h"

// The classes here abstract away all of the data, function calls, and coordinate-system transformations
// necessary to sample and evaluate microfacet distributions with GGX+Smith visible-normal sampling.
// The client microfacet lobes need not know whether the distribution is isotropic or anisotropic.
// These classes have no virtual functions; duck typing can be used to avoid any abstraction cost.

///////////////////////////////////////////////////
// AnisotropicGGXSmithVNDFMicrofacetDistribution //
///////////////////////////////////////////////////

struct OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution
{
    vec2 alpha;
    OpenPBR_Basis basis_ff;

    // Can be used by the client lobes to get the original or standard isotropic alpha value,
    // which is needed for auxiliary operations like creating microfacet-multiple-scattering lobes
    // and estimating lobe contributions.
    float isotropic_alpha;
};

vec3 openpbr_sample_ggx_smith_vndf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution)
                                       microfacet_distr,
                                   const vec3 view_direction,
                                   const vec3 normal_ff,
                                   const vec2 rand)
{
    // Choose microfacet normal (in local space).
    const vec3 half_vector_local =
        openpbr_sample_aniso_ggx_smith_vndf(microfacet_distr.alpha, openpbr_world_to_local(microfacet_distr.basis_ff, view_direction), rand);
    OPENPBR_ASSERT(half_vector_local.z >= 0.0f, "Bad half vector");

    // Map half vector to world.
    return openpbr_local_to_world(microfacet_distr.basis_ff, half_vector_local);
}

float openpbr_eval_ggx(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution) microfacet_distr,
                       const vec3 half_vector,
                       const vec3 normal_ff)
{
    return openpbr_eval_aniso_ggx(openpbr_world_to_local(microfacet_distr.basis_ff, half_vector), microfacet_distr.alpha);
}

float openpbr_eval_smith_g1(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution) microfacet_distr,
                            const vec3 view_direction_or_light_direction,
                            float iodotn)
{
    return openpbr_eval_aniso_smith_g1(openpbr_world_to_local(microfacet_distr.basis_ff, view_direction_or_light_direction), microfacet_distr.alpha);
}

float openpbr_eval_smith_g2(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution) microfacet_distr,
                            const vec3 view_direction,
                            const vec3 light_direction,
                            float idotn,
                            float odotn)
{
    return openpbr_eval_aniso_smith_g2(openpbr_world_to_local(microfacet_distr.basis_ff, view_direction),
                                       openpbr_world_to_local(microfacet_distr.basis_ff, light_direction),
                                       microfacet_distr.alpha);
}

float openpbr_get_isotropic_alpha(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution)
                                      microfacet_distr)
{
    return microfacet_distr.isotropic_alpha;
}

#endif  // !OPENPBR_VNDF_MICROFACET_DISTRIBUTION_H
