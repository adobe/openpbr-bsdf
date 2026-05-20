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

#ifndef OPENPBR_AGGREGATE_LOBE_H
#define OPENPBR_AGGREGATE_LOBE_H

#include "openpbr_comprehensive_microfacet_lobe.h"

#include "../openpbr_diffuse_specular.h"
#include "openpbr_diffuse_lobe.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_microfacet_multiple_scattering_lobes.h"
#include "openpbr_thin_wall_diffuse_transmission_lobe.h"
#include "openpbr_thin_wall_specular_transmission_lobe.h"

///////////////////
// AggregateLobe //
///////////////////

// This lobe stores all of the lobes that constitute the base surface.
// These lobes are assumed to be already weighted so that they can exist side by side without any nesting logic.
// In the names below, the acronym MMS stands for "microfacet multiple scattering".

OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_InvalidLobeIndex = -1;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_SpecularLobeIndex = 0;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_DielectricMMSLobeIndex = 1;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_MetalMMSLobeIndex = 2;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_DiffuseLobeIndex = 3;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_ThinWallSpecularTransLobeIndex = 4;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_ThinWallDiffuseTransLobeIndex = 5;
OPENPBR_CONSTEXPR_GLOBAL int OpenPBR_NumBaseLobes = 6;

struct OpenPBR_AggregateLobe
{
    OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe specular_lobe;
    OpenPBR_DielectricMicrofacetMultipleScatteringLobe dielectric_mms_lobe;
    OpenPBR_MetalMicrofacetMultipleScatteringLobe metal_mms_lobe;
    OpenPBR_EnergyConservingRoughDiffuseLobe diffuse_lobe;
    OpenPBR_ThinWallSpecularTransmissionLobe thin_wall_specular_trans_lobe;
    OpenPBR_ThinWallDiffuseTransmissionLobe thin_wall_diffuse_trans_lobe;

    float lobe_weights[OpenPBR_NumBaseLobes];
};

int openpbr_select_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe,
                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(float) rand,
                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) selected_lobe_weight,
                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) total_weight)
{
    total_weight = 0.0f;

    total_weight += lobe.lobe_weights[OpenPBR_SpecularLobeIndex];
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        total_weight += lobe.lobe_weights[OpenPBR_DielectricMMSLobeIndex];
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
        total_weight += lobe.lobe_weights[OpenPBR_MetalMMSLobeIndex];
    total_weight += lobe.lobe_weights[OpenPBR_DiffuseLobeIndex];
    total_weight += lobe.lobe_weights[OpenPBR_ThinWallSpecularTransLobeIndex];
    total_weight += lobe.lobe_weights[OpenPBR_ThinWallDiffuseTransLobeIndex];

    // Abort sampling if total_weight is zero, subnormal, or the minimum normal float.
    // If total_weight were subnormal or equal to OpenPBR_FloatMin, then multiplying rand by
    // total_weight -- which always makes rand less than total_weight when the original
    // rand is less than one and total_weight is a normal float -- would occasionally
    // make rand equal to total_weight, resulting in a corrupt sample of the last lobe.
    if (total_weight <= OpenPBR_FloatMin)
    {
        selected_lobe_weight = 0.0f;
        return OpenPBR_InvalidLobeIndex;
    }

    OPENPBR_ASSERT(rand < 1.0f, "Random number unexpectedly large");
    rand *= total_weight;
    OPENPBR_ASSERT(rand < total_weight, "Random number unexpectedly large");

    float weight_so_far = 0.0f;

    // Loop through all but the last lobe index.
    for (int lobe_index = 0; lobe_index < OpenPBR_NumBaseLobes - 1; ++lobe_index)
    {
        // Skip any lobes that are disabled by specialization constants.
        if ((!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && lobe_index == OpenPBR_DielectricMMSLobeIndex) ||
            (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && lobe_index == OpenPBR_MetalMMSLobeIndex))
            continue;

        selected_lobe_weight = lobe.lobe_weights[lobe_index];
        if (rand < weight_so_far + selected_lobe_weight)
        {
            rand = (rand - weight_so_far) / selected_lobe_weight;
            openpbr_clamp_remapped_random_number(rand);
            return lobe_index;
        }
        weight_so_far += selected_lobe_weight;
    }

    OPENPBR_CONSTEXPR_LOCAL int OpenPBR_LastLobeIndex = OpenPBR_NumBaseLobes - 1;
    // The very last lobe is currently the thin-wall diffuse-transmission lobe.
    OPENPBR_STATIC_ASSERT(OpenPBR_LastLobeIndex == OpenPBR_ThinWallDiffuseTransLobeIndex, "Unexpected lobe index");
    selected_lobe_weight = lobe.lobe_weights[OpenPBR_LastLobeIndex];
    // The logic is simplified on the last iteration.
    OPENPBR_ASSERT(weight_so_far + selected_lobe_weight == total_weight, "New sum should be identical to earlier sum");
    rand = (rand - weight_so_far) / selected_lobe_weight;
    openpbr_clamp_remapped_random_number(rand);
    return OpenPBR_LastLobeIndex;
}

// Assumes that child lobes have already been initialized.
void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_AggregateLobe) lobe,
                             const vec3 view_direction,
                             const vec3 path_throughput)
{
    lobe.lobe_weights[OpenPBR_SpecularLobeIndex] = openpbr_estimate_lobe_contribution(lobe.specular_lobe, view_direction, path_throughput);
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        lobe.lobe_weights[OpenPBR_DielectricMMSLobeIndex] =
            openpbr_estimate_lobe_contribution(lobe.dielectric_mms_lobe, view_direction, path_throughput);
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
        lobe.lobe_weights[OpenPBR_MetalMMSLobeIndex] = openpbr_estimate_lobe_contribution(lobe.metal_mms_lobe, view_direction, path_throughput);
    lobe.lobe_weights[OpenPBR_DiffuseLobeIndex] = openpbr_estimate_lobe_contribution(lobe.diffuse_lobe, view_direction, path_throughput);
    lobe.lobe_weights[OpenPBR_ThinWallSpecularTransLobeIndex] =
        openpbr_estimate_lobe_contribution(lobe.thin_wall_specular_trans_lobe, view_direction, path_throughput);
    lobe.lobe_weights[OpenPBR_ThinWallDiffuseTransLobeIndex] =
        openpbr_estimate_lobe_contribution(lobe.thin_wall_diffuse_trans_lobe, view_direction, path_throughput);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe)
{
    OpenPBR_BsdfLobeType result = OpenPBR_BsdfLobeTypeNone;

    result |= openpbr_get_lobe_type(lobe.specular_lobe);
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        result |= openpbr_get_lobe_type(lobe.dielectric_mms_lobe);
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
        result |= openpbr_get_lobe_type(lobe.metal_mms_lobe);
    result |= openpbr_get_lobe_type(lobe.diffuse_lobe);
    result |= openpbr_get_lobe_type(lobe.thin_wall_specular_trans_lobe);
    result |= openpbr_get_lobe_type(lobe.thin_wall_diffuse_trans_lobe);

    return result;
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    OpenPBR_DiffuseSpecular result = openpbr_make_zero_diffuse_specular();

    result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.specular_lobe, view_direction, light_direction));
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
        result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.dielectric_mms_lobe, view_direction, light_direction));
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
        result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.metal_mms_lobe, view_direction, light_direction));
    result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.diffuse_lobe, view_direction, light_direction));
    result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.thin_wall_specular_trans_lobe, view_direction, light_direction));
    result = openpbr_add_diffuse_specular(result, openpbr_calculate_lobe_value(lobe.thin_wall_diffuse_trans_lobe, view_direction, light_direction));

    return result;
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    float sum = 0.0f;
    float total_weight = 0.0f;

    {
        const float weight = lobe.lobe_weights[OpenPBR_SpecularLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.specular_lobe, view_direction, light_direction);
        total_weight += weight;
    }
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
    {
        const float weight = lobe.lobe_weights[OpenPBR_DielectricMMSLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.dielectric_mms_lobe, view_direction, light_direction);
        total_weight += weight;
    }
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
    {
        const float weight = lobe.lobe_weights[OpenPBR_MetalMMSLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.metal_mms_lobe, view_direction, light_direction);
        total_weight += weight;
    }
    {
        const float weight = lobe.lobe_weights[OpenPBR_DiffuseLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.diffuse_lobe, view_direction, light_direction);
        total_weight += weight;
    }
    {
        const float weight = lobe.lobe_weights[OpenPBR_ThinWallSpecularTransLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.thin_wall_specular_trans_lobe, view_direction, light_direction);
        total_weight += weight;
    }
    {
        const float weight = lobe.lobe_weights[OpenPBR_ThinWallDiffuseTransLobeIndex];
        sum += weight * openpbr_calculate_lobe_pdf(lobe.thin_wall_diffuse_trans_lobe, view_direction, light_direction);
        total_weight += weight;
    }

    return total_weight > 0.0f ? sum / total_weight : 0.0f;
}

// Given a view direction, sample a light direction,
// evaluate the throughput and pdf for this direction,
// and return whether a sample was successfully generated.
// Output arguments are only set when the function returns true.
bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    float rand_x = rand.x;
    float selected_lobe_weight;
    float total_weight;
    const int selected_lobe_index = openpbr_select_lobe(lobe, rand_x, selected_lobe_weight, total_weight);

    if (selected_lobe_index == OpenPBR_InvalidLobeIndex)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    const vec3 remapped_rand = vec3(rand_x, rand.y, rand.z);

    bool valid_sample = false;
    if (selected_lobe_index == OpenPBR_SpecularLobeIndex)
        valid_sample = openpbr_sample_lobe(lobe.specular_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && selected_lobe_index == OpenPBR_DielectricMMSLobeIndex)
        valid_sample = openpbr_sample_lobe(lobe.dielectric_mms_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && selected_lobe_index == OpenPBR_MetalMMSLobeIndex)
        valid_sample = openpbr_sample_lobe(lobe.metal_mms_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else if (selected_lobe_index == OpenPBR_DiffuseLobeIndex)
        valid_sample = openpbr_sample_lobe(lobe.diffuse_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else if (selected_lobe_index == OpenPBR_ThinWallSpecularTransLobeIndex)
        valid_sample =
            openpbr_sample_lobe(lobe.thin_wall_specular_trans_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else if (selected_lobe_index == OpenPBR_ThinWallDiffuseTransLobeIndex)
        valid_sample =
            openpbr_sample_lobe(lobe.thin_wall_diffuse_trans_lobe, remapped_rand, view_direction, light_direction, weight, pdf, sampled_type);
    else
    {
        OPENPBR_ASSERT_UNREACHABLE("Invalid lobe index");
    }

    if (!valid_sample)
        return false;

    // Compute overall weight and pdf with all matched lobes.
    if (!bool(sampled_type & OpenPBR_BsdfLobeTypeSpecular))
    {
        OPENPBR_ASSERT(pdf > 0.0f, "Non-delta lobe samples must either set a positive PDF or return false");
        OpenPBR_DiffuseSpecular bsdf_cos = openpbr_scale_diffuse_specular(weight, pdf);
        pdf *= selected_lobe_weight;

        if (selected_lobe_index != OpenPBR_SpecularLobeIndex)
        {
            bsdf_cos = openpbr_add_diffuse_specular(bsdf_cos, openpbr_calculate_lobe_value(lobe.specular_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_SpecularLobeIndex] * openpbr_calculate_lobe_pdf(lobe.specular_lobe, view_direction, light_direction);
        }
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && selected_lobe_index != OpenPBR_DielectricMMSLobeIndex)
        {
            bsdf_cos =
                openpbr_add_diffuse_specular(bsdf_cos, openpbr_calculate_lobe_value(lobe.dielectric_mms_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_DielectricMMSLobeIndex] *
                   openpbr_calculate_lobe_pdf(lobe.dielectric_mms_lobe, view_direction, light_direction);
        }
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && selected_lobe_index != OpenPBR_MetalMMSLobeIndex)
        {
            bsdf_cos = openpbr_add_diffuse_specular(bsdf_cos, openpbr_calculate_lobe_value(lobe.metal_mms_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_MetalMMSLobeIndex] * openpbr_calculate_lobe_pdf(lobe.metal_mms_lobe, view_direction, light_direction);
        }
        if (selected_lobe_index != OpenPBR_DiffuseLobeIndex)
        {
            bsdf_cos = openpbr_add_diffuse_specular(bsdf_cos, openpbr_calculate_lobe_value(lobe.diffuse_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_DiffuseLobeIndex] * openpbr_calculate_lobe_pdf(lobe.diffuse_lobe, view_direction, light_direction);
        }
        if (selected_lobe_index != OpenPBR_ThinWallSpecularTransLobeIndex)
        {
            bsdf_cos = openpbr_add_diffuse_specular(
                bsdf_cos, openpbr_calculate_lobe_value(lobe.thin_wall_specular_trans_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_ThinWallSpecularTransLobeIndex] *
                   openpbr_calculate_lobe_pdf(lobe.thin_wall_specular_trans_lobe, view_direction, light_direction);
        }
        if (selected_lobe_index != OpenPBR_ThinWallDiffuseTransLobeIndex)
        {
            bsdf_cos = openpbr_add_diffuse_specular(bsdf_cos,
                                                    openpbr_calculate_lobe_value(lobe.thin_wall_diffuse_trans_lobe, view_direction, light_direction));
            pdf += lobe.lobe_weights[OpenPBR_ThinWallDiffuseTransLobeIndex] *
                   openpbr_calculate_lobe_pdf(lobe.thin_wall_diffuse_trans_lobe, view_direction, light_direction);
        }

        pdf /= total_weight;                                            // PDF takes all lobes into account.
        weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);  // Weight takes all lobes into account.

        // NOTE: The sampled_type is intentionally not updated here -- it represents
        // the type of lobe that directly generated the sample.
    }
    else
    {
        // If a delta lobe is sampled, don't attempt to combine its weight with the other lobes
        // (theoretically multiple delta lobes could be combined but we're not doing that).
        weight = openpbr_scale_diffuse_specular(weight, total_weight / selected_lobe_weight);
    }

    return true;
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_AggregateLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    float result = 0.0f;

    for (int lobe_index = 0; lobe_index < OpenPBR_NumBaseLobes; ++lobe_index)
    {
        if ((!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && lobe_index == OpenPBR_DielectricMMSLobeIndex) ||
            (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic) && lobe_index == OpenPBR_MetalMMSLobeIndex))
            continue;

        result += lobe.lobe_weights[lobe_index];
    }

    return result;
}

#endif  // !OPENPBR_AGGREGATE_LOBE_H
