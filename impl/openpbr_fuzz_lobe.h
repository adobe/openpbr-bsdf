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

// This file implements the OpenPBR fuzz lobe, which is based on the paper:
// "Practical Multiple-Scattering Sheen Using Linearly Transformed Cosines".

#ifndef OPENPBR_FUZZ_LOBE_H
#define OPENPBR_FUZZ_LOBE_H

#include "../openpbr_constants.h"
#include "../openpbr_data_constants.h"
#include "../openpbr_diffuse_specular.h"
#include "openpbr_coating_lobe.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_math.h"
#include "openpbr_microfacet_multiple_scattering_data.h"
#include "openpbr_sampling.h"

#if !OPENPBR_USE_TEXTURE_LUTS
#include "data/openpbr_ltc_array.h"
#endif

///////////////////////////////
// DISNEY SHEEN LOBE SUMMARY //
///////////////////////////////

// This is an implementation of this paper:
//
//     "Practical Multiple-Scattering Sheen Using Linearly Transformed Cosines"
//     by Tizian Zeltner, Brent Burley, and Matt Jen-Yuan Chiang (2022).
//
// For more details about linearly transformed cosine distributions (LTCs), see this paper:
//
//     "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines"
//     by Heitz et al. (2016).
//
// The LTCs used by this BRDF were fitted to a volumetric sheen layer of fiber-like
// particles with a SGGX microflake phase function. For details about SGGX, see this paper:
//
//     "The SGGX Microflake Distribution"
//     by Heitz et al. (2015).
//
// The reference volume was non-absorptive (with single-scattering albedo = 1)
// and had unit density and thickness. These choices were somewhat arbitrary but
// reproduced the overall sheen look from explicit fibers that the authors were
// going for. The ASM sheen lobe made similar choices.
//
// The reference implementation by Tizian Zeltner, Brent Burley, and Matt Jen-Yuan Chiang
// is available at https://github.com/tizian/ltc-sheen and is licensed under the
// Apache License, Version 2.0. The core Disney-sheen-specific functions below are
// derived from that reference: they are prefixed with "disney_sheen_" and use the
// structure and variable names from the reference implementation to make it easier
// to compare them. Direction vectors and calculations in those functions are in the
// z-up local space.

////////////////////////////
// DISNEY SHEEN LOBE DATA //
////////////////////////////

struct OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe
{
    OpenPBR_CoatingLobe_AggregateLobe coating_lobe;  // the lobe underneath the sheen lobe

    // Note that there is some confusing overloaded terminology in the literature related to the roughness.
    //
    // The "alpha" in GGX is sometimes called the "roughness" (in the Disney LTC sheen paper for example),
    // but that's different than the "roughness" in most artist-friendly material models.
    // For example, the Disney Principled BRDF paper defines the "roughness" as the
    // square root of "alpha" (see attached excerpt from the Disney Principled BRDF paper).
    // The SGGX paper calls "sigma" the "roughness". But the Disney LTC sheen paper's "roughness"
    // is denoted "alpha" and defined as the square root of "sigma".
    //
    // In other words, the common microfacet-model perceptual roughness is the square root
    // of the GGX roughness alpha, which matches the SGGX roughness sigma but is different
    // from the LTC-sheen roughness alpha, which is the square root of sigma.

    float alpha;  // artist-specified roughness parameter;
                  // the square root of the SGGX fiber cross section sigma

    vec3 tint;       // multiplied by the sheen result to make even black sheen possible
    float presence;  // the proportion of the microsurface covered by the sheen

    OpenPBR_Basis basis;   // a basis aligned with the normal and oriented according to the view direction
    vec3 view_dir_local;   // the view direction in the z-up local space of the basis
    float view_reflected;  // overall proportion of light reflected by the sheen lobe for the given view direction
};

//////////////////////////////////////////////////////
// LOW-LEVEL MATH HELPER FUNCTIONS FOR DISNEY SHEEN //
//////////////////////////////////////////////////////

// Two helper functions for evaluating / sampling from LTCs that are defined
// based on a standard coordinate system aligned with "phi = 0".

float openpbr_disney_sheen_phi(const vec3 v)
{
    float p = atan2(v.y, v.x);
    if (p < 0.0f)
        p += OpenPBR_TwoPi;
    return p;
}

vec3 openpbr_disney_sheen_rotate_vector(const vec3 v, const vec3 axis, const float angle)
{
    const float s = sin(angle);
    const float c = cos(angle);
    return v * c + axis * dot(v, axis) * (1.0f - c) + s * cross(axis, v);
}

//////////////////////////////////////////////////////
// LOOKUP-TABLE-RELATED UTILITIES FOR DISNEY SHEEN  //
//////////////////////////////////////////////////////

// Evaluate the LTC distribution in its default coordinate system.
float openpbr_disney_sheen_eval_ltc(const vec3 wi_local, const vec3 ltc_coeffs)
{
    // The (inverse) transform matrix "M^{-1}" is given by:
    //
    //              [[a_inv 0    b_inv]
    //     M^{-1} =  [0    a_inv 0    ]
    //               [0    0    1     ]]
    //
    // with "a_inv = ltc_coeffs[0]", "b_inv = ltc_coeffs[1]" fetched from the
    // table. The transformed direction "wi_original_local" is therefore:
    //
    //                                            [[a_inv * wi_local.x + b_inv * wi_local.z]
    //     wi_original_local = M^{-1} * wi_local = [a_inv * wi_local.y                     ]
    //                                             [wi_local.z                             ]]
    //
    // which is subsequently normalized. The determinant of the matrix is:
    //
    //     |M^{-1}| = a_inv * a_inv
    //
    // which is used to compute the Jacobian determinant of the complete
    // mapping including the normalization.
    //
    // See the original paper [Heitz et al. 2016] for details about the LTC itself.

    const float a_inv = ltc_coeffs[0];
    const float b_inv = ltc_coeffs[1];
    vec3 wi_original_local = vec3(a_inv * wi_local.x + b_inv * wi_local.z, a_inv * wi_local.y, wi_local.z);
    const float len = length(wi_original_local);
    wi_original_local /= len;

    const float det = a_inv * a_inv;
    const float jacobian = det / (len * len * len);

    return openpbr_compute_pdf_for_sample_unit_hemisphere_cosine(wi_original_local) * jacobian;
}

// Sample from the LTC distribution in its default coordinate system.
vec3 openpbr_disney_sheen_sample_ltc(const vec3 ltc_coeffs, const vec2 rand)
{
    // The (inverse) transform matrix "M^{-1}" is given by:
    //
    //              [[a_inv 0    b_inv]
    //     M^{-1} =  [0    a_inv 0    ]
    //               [0    0    1     ]]
    //
    // with "a_inv = ltc_coeffs[0]", "b_inv = ltc_coeffs[1]" fetched from the
    // table. The non-inverted matrix "M" is therefore:
    //
    //         [[1/a_inv 0      -b_inv/a_inv]
    //     M =  [0      1/a_inv  0          ]
    //          [0      0       1           ]]
    //
    // and the transformed direction wi_local is:
    //
    //                                       [[wi_original_local.x/a_inv - wi_original_local.z*b_inv/a_inv]
    //     wi_local = M * wi_original_local = [wi_original_local.y/a_inv                                  ]
    //                                        [wi_original_local.z                                        ]]
    //
    // which is subsequently normalized.
    //
    // See the original paper [Heitz et al. 2016] for details about the LTC itself.

    const vec3 wi_original_local = openpbr_sample_unit_hemisphere_cosine(rand);

    const float a_inv = ltc_coeffs[0];
    const float b_inv = ltc_coeffs[1];
    const float a = 1.0f / a_inv;
    const vec3 wi_local = vec3(wi_original_local.x * a - wi_original_local.z * b_inv * a, wi_original_local.y * a, wi_original_local.z);
    return normalize(wi_local);
}

// Fetch the LTC coefficients by bilinearly interpolating entries in a 32x32 lookup table.
vec3 openpbr_disney_sheen_fetch_coeffs(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                       const vec3 direction_local)
{
    // LTC coefficients are stored in a table parameterized by these variables:
    //
    //     - Elevation angle, parameterized as "cos(theta)".
    //     - Sheen roughness "alpha", which is related to the SGGX fiber cross
    //           section "sigma" as "alpha = sqrt(sigma)".
    //
    // Two versions of the LTC fit are available. We use the "Volume" version, which
    // is a direct fit of the volumetric sheen model (not the "Approx" version).

#if OPENPBR_USE_TEXTURE_LUTS
    // Since the original data is contained in a table (see https://github.com/tizian/ltc-sheen/blob/master/pbrt-v3/src/materials/sheenltc.cpp),
    // we compensate the difference between bilinear interpolation in that table and in a regular texture by:
    //     - keeping the original sampling indices.
    //     - offsetting the UVs by 0.5 (in pixel space).
    OPENPBR_CONSTEXPR_LOCAL vec2 uv_max =
        vec2(float(OpenPBR_LTCTableSize - 1) / float(OpenPBR_LTCTableSize), float(OpenPBR_LTCTableSize - 1) / float(OpenPBR_LTCTableSize));
    OPENPBR_CONSTEXPR_LOCAL vec2 uv_step = vec2(0.5f / float(OpenPBR_LTCTableSize), 0.5f / float(OpenPBR_LTCTableSize));
    const vec2 uv = vec2(openpbr_get_cos_theta_local(direction_local), lobe.alpha) * uv_max + uv_step;
    return OPENPBR_SWIZZLE(OPENPBR_SAMPLE_2D_TEXTURE(OpenPBR_LutId_LTC, uv), xyz);
#else
    // Fetch the LTC coefficients by bilinearly interpolating entries in a 32x32 lookup table.
    // This exactly matches Disney's SheenLTC::fetchCoeffs implementation.

    // Compute table indices and interpolation factors.
    const float row = max(0.0f, min(lobe.alpha, OpenPBR_LargestFloatBelowOne)) * float(OpenPBR_LTCTableSize - 1);
    const float col = max(0.0f, min(openpbr_get_cos_theta_local(direction_local), OpenPBR_LargestFloatBelowOne)) * float(OpenPBR_LTCTableSize - 1);
    const float r = floor(row);
    const float c = floor(col);
    const float rf = row - r;
    const float cf = col - c;
    const int ri = int(r);
    const int ci = int(c);

// Local helper for LTC table indexing matching Disney's array layout:
// Disney's _ltcParamTableVolume[row][column] maps to: row * OpenPBR_LTCTableSize + column
#define ltcFlatIndex(row, column) ((row) * OpenPBR_LTCTableSize + (column))

    // Bilinear interpolation - matches Disney's exact formula:
    // return (_ltcParamTableVolume[ri][ci]     * (1.f - cf) + _ltcParamTableVolume[ri][ci + 1]     * cf) * (1.f - rf) +
    //        (_ltcParamTableVolume[ri + 1][ci] * (1.f - cf) + _ltcParamTableVolume[ri + 1][ci + 1] * cf) * rf;
    return mix(mix(OpenPBR_LTC_Array[ltcFlatIndex(ri, ci)], OpenPBR_LTC_Array[ltcFlatIndex(ri, ci + 1)], cf),
               mix(OpenPBR_LTC_Array[ltcFlatIndex(ri + 1, ci)], OpenPBR_LTC_Array[ltcFlatIndex(ri + 1, ci + 1)], cf),
               rf);

#undef ltcFlatIndex

#endif
}

/////////////////////////////////////
// CORE BSDF MATH FOR DISNEY SHEEN //
/////////////////////////////////////

// Evaluate the BRDF.
vec3 openpbr_disney_sheen_f(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                            const vec3 wo_local,
                            const vec3 wi_local)
{
    const float cos_theta_o = openpbr_get_cos_theta_local(wo_local);
    const float cos_theta_i = openpbr_get_cos_theta_local(wi_local);
    if (cos_theta_o < 0.0f || cos_theta_i < 0.0f)
        return vec3(0.0f);

    // Rotate coordinate frame to align with incident direction wo_local.
    const float phi_std = openpbr_disney_sheen_phi(wo_local);
    const vec3 wi_std_local = openpbr_disney_sheen_rotate_vector(wi_local, vec3(0.0f, 0.0f, 1.0f), -phi_std);

    // Evaluate LTC distribution in aligned coordinates.
    const vec3 ltc_coeffs = openpbr_disney_sheen_fetch_coeffs(lobe, wo_local);
    vec3 value = vec3(openpbr_disney_sheen_eval_ltc(wi_std_local, ltc_coeffs));

    // Also consider the presence (surface coverage), overall reflectance ("R" in the paper),
    // and artist-specified sheen tint ("Csheen" in the paper).
    //
    // Ideally color would be introduced by adding absorption to the microflake volume itself
    // during the simulation, however that would complicate the LTC fitting --
    // the simpler solution is just to multiply the final result by the given color.
    const float r = ltc_coeffs[2];
    value *= lobe.presence * r * lobe.tint;

    // The cosine foreshortening factor included in the LTC and Eclair's convention
    // is to include this factor in value returned from the BSDF.
    // If the cosine foreshortening factor were handled in the integrator instead of the BSDF,
    // we would need to cancel it out here like this:
    //
    // return cos_theta_i > 0.0f ? value / cos_theta_i : vec3(0.0f);

    return value;
}

// Sample proportionally to the BRDF.
// The LTC representation allows perfect importance sampling.
// Returns whether a valid sample was successfully generated.
bool openpbr_disney_sheen_sample_f(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                   const vec2 rand,
                                   const vec3 wo_local,
                                   OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) wi_local)
{
    const float cos_theta_o = openpbr_get_cos_theta_local(wo_local);
    if (cos_theta_o < 0.0f)
    {
        wi_local = vec3(0.0f);
        return false;
    }

    // Sample from the LTC distribution in aligned coordinates.
    const vec3 wi_std_local = openpbr_disney_sheen_sample_ltc(openpbr_disney_sheen_fetch_coeffs(lobe, wo_local), rand);

    // Rotate coordinate frame based on incident direction wo_local.
    const float phi_std = openpbr_disney_sheen_phi(wo_local);
    wi_local = openpbr_disney_sheen_rotate_vector(wi_std_local, vec3(0.0f, 0.0f, 1.0f), +phi_std);

    if (!openpbr_are_in_same_hemisphere_local(wo_local, wi_local))
        return false;

    return true;
}

// Sampling density associated with the "disney_sheen_sample_f" function above. Used for MIS.
float openpbr_disney_sheen_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                               const vec3 wo_local,
                               const vec3 wi_local)
{
    const float cos_theta_o = openpbr_get_cos_theta_local(wo_local), cos_theta_i = openpbr_get_cos_theta_local(wi_local);
    if (cos_theta_o < 0.0f || cos_theta_i < 0.0f)
        return 0.0f;

    // Rotate coordinate frame to align with incident direction wo_local.
    const float phi_std = openpbr_disney_sheen_phi(wo_local);
    const vec3 wi_std_local = openpbr_disney_sheen_rotate_vector(wi_local, vec3(0.0f, 0.0f, 1.0f), -phi_std);

    // Evaluate LTC distribution in aligned coordinates.
    return openpbr_disney_sheen_eval_ltc(wi_std_local, openpbr_disney_sheen_fetch_coeffs(lobe, wo_local));
}

///////////////////////////////////////////////////////////
// GENERAL SHEEN LOBE FRAMEWORK ADAPTED FOR DISNEY SHEEN //
///////////////////////////////////////////////////////////

float openpbr_proportion_reflected(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                   const vec3 direction_local)
{
    // The implementation of this function uses the conventions of the core Disney sheen functions.
    //
    // Note that the proportion reflected is always less than one, which also means that
    // the base layer is always visible to some extent. This makes sense because the sheen
    // is based on a volume of finite density and thickness.

    const float cos_theta = openpbr_get_cos_theta_local(direction_local);
    if (cos_theta < 0.0f)
        return 0.0f;

    // The third component of each of the items in the LTC LUT is the overall reflectance ("R" in the paper).
    const vec3 ltc_coeffs = openpbr_disney_sheen_fetch_coeffs(lobe, direction_local);
    const float r = ltc_coeffs[2];
    return lobe.presence * r;
}

float openpbr_base_layer_scale_incoming(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe)
{
    return 1.0f - lobe.view_reflected;
}

float openpbr_base_layer_scale_outgoing(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                        const vec3 light_direction_local)
{
    const float light_reflected = openpbr_proportion_reflected(lobe, light_direction_local);
    return 1.0f - light_reflected;
}

float openpbr_base_layer_scale_complete(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                        const vec3 light_direction_local)
{
    const float incoming = openpbr_base_layer_scale_incoming(lobe);
    const float outgoing = openpbr_base_layer_scale_outgoing(lobe, light_direction_local);
    return incoming * outgoing;
}

float openpbr_sheen_probability(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                const vec3 view_direction)
{
    // Here we calculate the expected contribution of the sheen lobe relative to the overall lobe
    // and we use that to set the probability of sampling the sheen lobe.

    // TODO: Take the real path throughput into account (for both standalone lobes),
    //       either by saving it in the lobe struct or by passing it in.
    OPENPBR_CONSTEXPR_LOCAL vec3 PlaceholderPathThroughput = vec3(1.0f);

    // lobe.view_reflected is already weighted by lobe.presence; see openpbr_proportion_reflected().
    const float sheen_contribution = lobe.view_reflected * openpbr_max3(lobe.tint);

    const float unscaled_base_lobe_contribution = openpbr_estimate_lobe_contribution(lobe.coating_lobe, view_direction, PlaceholderPathThroughput);
    const float scaled_base_lobe_contribution = unscaled_base_lobe_contribution * openpbr_base_layer_scale_incoming(lobe);

    return openpbr_safe_divide(sheen_contribution, sheen_contribution + scaled_base_lobe_contribution, 0.5f);
}

// Assumes that child lobe has already been initialized.
void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const float roughness,
                             const vec3 tint,
                             const float presence)
{
    lobe.alpha = roughness;

    lobe.tint = tint;
    lobe.presence = presence;

    lobe.basis = openpbr_make_basis(normal_ff);
    lobe.view_dir_local = openpbr_world_to_local(lobe.basis, view_direction);
    lobe.view_reflected = openpbr_proportion_reflected(lobe, lobe.view_dir_local);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe)
{
    return OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection | openpbr_get_lobe_type(lobe.coating_lobe);
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    const OpenPBR_DiffuseSpecular base_value = openpbr_calculate_lobe_value(lobe.coating_lobe, view_direction, light_direction);

    const vec3 light_dir_local = openpbr_world_to_local(lobe.basis, light_direction);
    const vec3 sheen_value = openpbr_disney_sheen_f(lobe, lobe.view_dir_local, light_dir_local);

    return openpbr_add_diffuse_specular(openpbr_scale_diffuse_specular(base_value, openpbr_base_layer_scale_complete(lobe, light_dir_local)),
                                        openpbr_make_diffuse_specular_from_specular(sheen_value));
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    const float base_pdf = openpbr_calculate_lobe_pdf(lobe.coating_lobe, view_direction, light_direction);

    const vec3 light_dir_local = openpbr_world_to_local(lobe.basis, light_direction);
    const float sheen_pdf = openpbr_disney_sheen_pdf(lobe, lobe.view_dir_local, light_dir_local);

    const float sheen_prob = openpbr_sheen_probability(lobe, view_direction);

    return mix(base_pdf, sheen_pdf, sheen_prob);
}

// Given a view direction, sample a light direction,
// evaluate the throughput and pdf for this direction,
// and return whether a sample was successfully generated.
// Output arguments are only set when the function returns true.
bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const float sheen_prob = openpbr_sheen_probability(lobe, view_direction);
    const float base_prob = 1.0f - sheen_prob;

    float rand_x = rand.x;
    if (rand_x < sheen_prob)
    {
        // Sample the sheen lobe itself.

        // If we were to need 3 random numbers instead of 2, we could remap the first one like this:
        //     rand_x /= sheen_prob;
        //     clamp_remapped_random_number(rand_x);

        vec3 light_dir_local;
        const bool success = openpbr_disney_sheen_sample_f(lobe, vec2(rand.y, rand.z), lobe.view_dir_local, light_dir_local);
        if (!success)
        {
            openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
            return false;
        }

        light_direction = openpbr_local_to_world(lobe.basis, light_dir_local);

        pdf = openpbr_disney_sheen_pdf(lobe, lobe.view_dir_local, light_dir_local) * sheen_prob;
        pdf += openpbr_calculate_lobe_pdf(lobe.coating_lobe, view_direction, light_direction) *
               base_prob;  // The returned pdf takes both lobes into account.

        weight = openpbr_scale_diffuse_specular(openpbr_calculate_lobe_value(lobe, view_direction, light_direction),
                                                1.0f / pdf);  // The returned weight takes both lobes into account.

        sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
    }
    else
    {
        // Sample the base lobe below the sheen lobe.

        const float reciprocal_base_prob = 1.0f / base_prob;

        rand_x = (rand_x - sheen_prob) * reciprocal_base_prob;
        openpbr_clamp_remapped_random_number(rand_x);

        const bool success =
            openpbr_sample_lobe(lobe.coating_lobe, vec3(rand_x, rand.y, rand.z), view_direction, light_direction, weight, pdf, sampled_type);
        if (!success)
            return false;

        const vec3 light_dir_local = openpbr_world_to_local(lobe.basis, light_direction);

        if (!bool(sampled_type & OpenPBR_BsdfLobeTypeSpecular))  // non-delta
        {
            OpenPBR_DiffuseSpecular bsdf_cos = openpbr_scale_diffuse_specular(weight, pdf);
            bsdf_cos = openpbr_scale_diffuse_specular(bsdf_cos, openpbr_base_layer_scale_complete(lobe, light_dir_local));
            const vec3 sheen = openpbr_disney_sheen_f(lobe, lobe.view_dir_local, light_dir_local);
            bsdf_cos = openpbr_add_diffuse_specular(bsdf_cos, openpbr_make_diffuse_specular_from_specular(sheen));

            pdf *= base_prob;
            pdf +=
                openpbr_disney_sheen_pdf(lobe, lobe.view_dir_local, light_dir_local) * sheen_prob;  // The returned pdf takes both lobes into account.

            weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);  // The returned weight takes both lobes into account.
        }
        else  // delta
        {
            weight = openpbr_scale_diffuse_specular(weight, openpbr_base_layer_scale_complete(lobe, light_dir_local) * reciprocal_base_prob);
        }
    }

    return true;
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    // Note that presence isn't explicitly included below because lobe.view_reflected is already
    // weighted by lobe.presence; see openpbr_proportion_reflected().
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, lobe.view_reflected * lobe.tint) +
           openpbr_estimate_lobe_contribution(lobe.coating_lobe, view_direction, path_throughput) * openpbr_base_layer_scale_incoming(lobe);
}

#endif  // OPENPBR_FUZZ_LOBE_H
