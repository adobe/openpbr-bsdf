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

#ifndef OPENPBR_DIFFUSE_LOBE_H
#define OPENPBR_DIFFUSE_LOBE_H

#include "../openpbr_basis.h"
#include "../openpbr_diffuse_specular.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_microfacet_multiple_scattering_data.h"

//////////////////////////////////////
// EnergyConservingRoughDiffuseLobe //
//////////////////////////////////////

struct OpenPBR_EnergyConservingRoughDiffuseLobe
{
    vec3 normal_ff;
    vec3 diffuse_albedo;
    float diffuse_roughness;
    float specular_alpha;
    float specular_eta_t_over_eta_i;
    float cached_specular_energy_compensation;
};

// -------------------------------
// Low-level BRDF implementation.
// -------------------------------

OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_FONConstantA = 0.5f - 2.0f / (3.0f * OpenPBR_Pi);
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_FONConstantB = 2.0f / 3.0f - 28.0f / (15.0f * OpenPBR_Pi);

// Fujii Oren-Nayar (FON) directional albedo (exact).
float openpbr_E_FON_exact(const float mu, const float roughness)
{
    const float AF = 1.0f / (1.0f + OpenPBR_FONConstantA * roughness);  // FON A coeff.
    const float BF = roughness * AF;                                    // FON B coeff.
    const float Si = sqrt(1.0f - (mu * mu));
    const float G = Si * (acos(clamp(mu, -1.0f, 1.0f)) - Si * mu) + (2.0f / 3.0f) * ((Si / mu) * (1.0f - (Si * Si * Si)) - Si);
    return AF + (BF * OpenPBR_RcpPi) * G;
}

// Fujii Oren-Nayar (FON) directional albedo (approximate).
float openpbr_E_FON_approx(const float mu, const float roughness)
{
    const float mucomp = 1.0f - mu;
    OPENPBR_CONSTEXPR_LOCAL float g1 = 0.0571085289f;
    OPENPBR_CONSTEXPR_LOCAL float g2 = 0.491881867f;
    OPENPBR_CONSTEXPR_LOCAL float g3 = -0.332181442f;
    OPENPBR_CONSTEXPR_LOCAL float g4 = 0.0714429953f;
    const float GoverPi = mucomp * (g1 + mucomp * (g2 + mucomp * (g3 + mucomp * g4)));
    return (1.0f + roughness * GoverPi) / (1.0f + OpenPBR_FONConstantA * roughness);
}

// Evaluates the Energy-preserving Fujii Oren-Nayar (EON) BRDF value, given inputs:
//          rho = single-scattering albedo parameter
//    roughness = roughness in [0, 1]
//     wi_local = direction of incident ray (directed away from vertex)
//     wo_local = direction of outgoing ray (directed away from vertex)
//        exact = flag to select exact or fast approx. version
vec3 openpbr_f_EON(const vec3 rho, const float roughness, const vec3 wi_local, const vec3 wo_local, const bool exact)
{
    const float mu_i = wi_local.z;                                                // input angle cos
    const float mu_o = wo_local.z;                                                // output angle cos
    const float s = dot(wi_local, wo_local) - mu_i * mu_o;                        // QON s term
    const float sovertF = s > 0.0f ? s / max(mu_i, mu_o) : s;                     // FON s/t
    const float AF = 1.0f / (1.0f + OpenPBR_FONConstantA * roughness);            // FON A coeff.
    const vec3 f_ss = (rho * OpenPBR_RcpPi) * AF * (1.0f + roughness * sovertF);  // single-scatter lobe
    const float EFo = exact ? openpbr_E_FON_exact(mu_o, roughness) :              // FON w_o albedo (exact)
                          openpbr_E_FON_approx(mu_o, roughness);                  // FON w_o albedo (approx)
    const float EFi = exact ? openpbr_E_FON_exact(mu_i, roughness) :              // FON w_i albedo (exact)
                          openpbr_E_FON_approx(mu_i, roughness);                  // FON w_i albedo (approx)
    const float avgEF = AF * (1.0f + OpenPBR_FONConstantB * roughness);           // avg. albedo
    const vec3 rho_ms = (rho * rho) * avgEF / (vec3(1.0f) - rho * (1.0f - avgEF));
    OPENPBR_CONSTEXPR_LOCAL float Eps = 1.0e-7f;
    const vec3 f_ms = (rho_ms * OpenPBR_RcpPi) * max(Eps, 1.0f - EFo)  // multi-scatter lobe
                      * max(Eps, 1.0f - EFi) / max(Eps, 1.0f - avgEF);
    return f_ss + f_ms;
}

// ---------------------------
// Top-level wrapper function.
// ---------------------------

// This function calculates the diffuse BRDF value adjusted for the specular energy compensation.
// It handles selection of the appropriate BRDF model and conversion of the view and light directions to local space.
vec3 openpbr_diffuse_brdf_value_adjusted_for_specular(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                                                      const vec3 view_direction,
                                                      const vec3 light_direction)
{
    // Calculate a local basis:
    const OpenPBR_Basis basis = openpbr_make_basis(lobe.normal_ff);
    const vec3 wi_local = openpbr_world_to_local(basis, view_direction);
    const vec3 wo_local = openpbr_world_to_local(basis, light_direction);

    OPENPBR_CONSTEXPR_LOCAL bool Exact = false;
    const vec3 brdf_value = openpbr_f_EON(lobe.diffuse_albedo, lobe.diffuse_roughness, wi_local, wo_local, Exact);

    const float cos_out = wo_local.z;

    const float specular_energy_compensation =
        lobe.cached_specular_energy_compensation *
        openpbr_look_up_opaque_dielectric_energy_complement(lobe.specular_eta_t_over_eta_i, lobe.specular_alpha, cos_out);

    return brdf_value * specular_energy_compensation;
}

// ------------------------
// Required lobe functions.
// ------------------------

void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                             const vec3 normal_ff,
                             const vec3 view_direction,
                             const vec3 diffuse_albedo,
                             const float diffuse_roughness,
                             const float specular_alpha,
                             const float specular_eta_t_over_eta_i)
{
    lobe.normal_ff = normal_ff;
    lobe.diffuse_albedo = diffuse_albedo;
    lobe.diffuse_roughness = diffuse_roughness;
    lobe.specular_alpha = specular_alpha;
    lobe.specular_eta_t_over_eta_i = specular_eta_t_over_eta_i;

    const float cos_in = dot(view_direction, lobe.normal_ff);
    OPENPBR_ASSERT(cos_in >= 0.0f, "Input normal must be face-forwarded");

    // In the terminology of the original ASM ECD lobe, this calculates (1 - L) / (1 - T).
    // (1 - V) is multiplied in later. TODO: Check terminology.
    // This result is calculated assuming an "untinted" white albedo, and then the
    // underlying BRDF value (e.g., rho / pi for Lambert) is multiplied in later.
    const float untinted_numerator = openpbr_look_up_opaque_dielectric_energy_complement(lobe.specular_eta_t_over_eta_i, lobe.specular_alpha, cos_in);
    const float untinted_denominator =
        openpbr_look_up_opaque_dielectric_average_energy_complement(lobe.specular_eta_t_over_eta_i, lobe.specular_alpha);
    const float untinted = untinted_numerator / openpbr_clamp_average_energy_complement_above_zero(untinted_denominator);
    lobe.cached_specular_energy_compensation = untinted;
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe)
{
    return OpenPBR_BsdfLobeTypeDiffuse | OpenPBR_BsdfLobeTypeReflection;
}

OpenPBR_DiffuseSpecular openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                                                     const vec3 view_direction,
                                                     const vec3 light_direction)
{
    const float cos_out = dot(light_direction, lobe.normal_ff);
    if (cos_out <= 0.0f)
        return openpbr_make_zero_diffuse_specular();

    return openpbr_make_diffuse_specular_from_diffuse(openpbr_diffuse_brdf_value_adjusted_for_specular(lobe, view_direction, light_direction) *
                                                      cos_out);
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    return max(dot(light_direction, lobe.normal_ff), 0.0f) * OpenPBR_RcpPi;
}

bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const vec3 light_direction_local = openpbr_sample_unit_hemisphere_cosine(OPENPBR_SWIZZLE(rand, xy));
    const float cos_out = light_direction_local.z;

    OPENPBR_ASSERT(cos_out >= 0.0f, "Hemisphere sampling is expected to return direction with non-negative Z component");
    // Implementation conventions dictates that the sampling function must return false if the pdf is zero,
    // which could potentially happen in this case if there are any numerical issues in the cosine sampling.
    if (cos_out == 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    // Map to world.
    // TODO: Avoid mapping back and forth.
    light_direction = openpbr_local_to_world(openpbr_make_basis(lobe.normal_ff), light_direction_local);

    weight = openpbr_make_diffuse_specular_from_diffuse(openpbr_diffuse_brdf_value_adjusted_for_specular(lobe, view_direction, light_direction) *
                                                        OpenPBR_Pi);  // the pdf is cos_out/pi so the cos_out terms cancels out
    pdf = cos_out * OpenPBR_RcpPi;
    sampled_type = openpbr_get_lobe_type(lobe);
    return true;
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_EnergyConservingRoughDiffuseLobe) lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    // TODO: Improve.
    return openpbr_max_component_of_throughput_weighted_color(path_throughput, lobe.diffuse_albedo * lobe.cached_specular_energy_compensation);
}

#endif  // !OPENPBR_DIFFUSE_LOBE_H
