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

#ifndef OPENPBR_LOBE_UTILS_H
#define OPENPBR_LOBE_UTILS_H

#include "../openpbr_basis.h"
#include "../openpbr_bsdf_lobe_type.h"
#include "../openpbr_constants.h"
#include "../openpbr_diffuse_specular.h"
#include "openpbr_math.h"
#include "openpbr_sampling.h"

// This function ensures that a random number is in [0, 1),
// which may occasionally not be the case due to rounding error or fast-math transformations in remapping calculations.
void openpbr_clamp_remapped_random_number(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(float) rand)
{
    OPENPBR_ASSERT(rand >= 0.0f, "It is assumed that the random number is already >= 0");
    // TODO: Address numerical vulnerabilities, uncomment this assertion, and disable the following clamp.
    // OPENPBR_ASSERT(rand < 1.0f, "It is assumed that the random number is already < 1");
    rand = min(rand, OpenPBR_LargestFloatBelowOne);
}

// Applies anisotropy to an isotropic GGX microfacet distribution "alpha" (square roughness)
// using the formulas from the OpenPBR spec.
//
// When applicable, also applies a clamp to the anisotropic alpha values to ensure they are above a minimum value.
vec2 openpbr_compute_anisotropic_alpha(const float isotropic_alpha,
                                       const float openpbr_anisotropy,
                                       const bool delta_specular_supported,
                                       const float min_microfacet_alpha)
{
    vec2 anisotropic_alpha;

    if (openpbr_anisotropy > 0.0f)
    {
        // Apply the formulas from the OpenPBR spec.
        anisotropic_alpha.x = isotropic_alpha * sqrt(2.0f / (1.0f + openpbr_square(1.0f - openpbr_anisotropy)));
        anisotropic_alpha.y = (1.0f - openpbr_anisotropy) * anisotropic_alpha.x;
    }
    else  // isotropic
    {
        anisotropic_alpha = vec2(isotropic_alpha);
    }

    // Unlike in the isotropic case where we could switch to delta lobes at low roughnesses,
    // we must always use microfacet lobes when there is anisotropy; therefore we need to
    // clamp the alphas to avoid numerical issues.
    //
    // If delta specular is not supported, we also need clamp isotropic alpha values to
    // ensure they are above a minimum value. (If delta specular is supported, we can
    // use the delta specular lobe to handle the case where the alpha is too small.)
    if (openpbr_anisotropy > 0.0f || !delta_specular_supported)
    {
        anisotropic_alpha = max(anisotropic_alpha, vec2(min_microfacet_alpha));
    }

    return anisotropic_alpha;
}

// Evaluate isotropic GGX.
float openpbr_eval_iso_ggx(const float cos_theta, const float alpha)
{
    if (cos_theta <= 0.0f)
        return 0.0f;
    const float c2 = openpbr_square(cos_theta);
    const float t2 = (1.0f - c2) / c2;
    const float a2 = openpbr_square(alpha);
    const float denom = OpenPBR_Pi * openpbr_square(c2) * openpbr_square(a2 + t2);
    return a2 / denom;
}

// Sample NDF of isotropic GGX with Smith masking function.
vec3 openpbr_sample_iso_ggx(const vec2 rand, const float alpha)
{
    const float theta = atan(alpha * openpbr_fast_sqrt(rand.x / (1.0f - rand.x)));
    const float phi = OpenPBR_TwoPi * rand.y;
    const float st = sin(theta);
    const float ct = cos(theta);
    const float sp = sin(phi);
    const float cp = cos(phi);
    return vec3(st * cp, st * sp, ct);
}

vec3 openpbr_sample_iso_ggx_world(const vec2 rand, const OpenPBR_Basis basis, const float alpha)
{
    const vec3 result = openpbr_sample_iso_ggx(rand, alpha);
    return normalize(result.x * basis.t + result.y * basis.b + result.z * basis.n);
}

// Evaluate isotropic Smith shadowing or masking function.
float openpbr_eval_iso_smith_g1(const float cos_theta, const float alpha)
{
    if (cos_theta <= 0.0f)
        return 0.0f;
    const float c2 = openpbr_square(cos_theta);
    const float t2 = (1.0f - c2) / c2;
    const float a2 = openpbr_square(alpha);
    const float tmp = openpbr_fast_sqrt(1.0f + a2 * t2);
    return 2.0f / (1.0f + tmp);
}

// Evaluate isotropic Smith shadowing and masking function.
float openpbr_eval_iso_smith_g2(const float cos1, const float cos2, const float alpha)
{
    return openpbr_eval_iso_smith_g1(cos1, alpha) * openpbr_eval_iso_smith_g1(cos2, alpha);
}

// Evaluate anisotropic GGX.
// Input vector is in local (z-up) space.
float openpbr_eval_aniso_ggx(const vec3 n, const vec2 alpha)
{
    return 1.0f /
           (OpenPBR_Pi * alpha.x * alpha.y * openpbr_square(openpbr_square(n.x / alpha.x) + openpbr_square(n.y / alpha.y) + openpbr_square(n.z)));
}

// Sample VNDF of anisotropic GGX with Smith masking function.
// Input and output vectors are in local (z-up) space.
// Based on the technique described in the paper "Sampling the GGX Distribution of Visible Normals" (http://jcgt.org/published/0007/04/01/).
vec3 openpbr_sample_aniso_ggx_smith_vndf(const vec2 alpha, const vec3 incoming, const vec2 rand)
{
    // Transform ellipsoid configuration into sphere configuration.
    const vec3 ellipsoid_to_hemisphere = vec3(alpha.x, alpha.y, 1.0f);
    const vec3 incoming_hemisphere = openpbr_fast_normalize(ellipsoid_to_hemisphere * incoming);
    const float incoming_hemisphere_xy_length_squared = openpbr_square(incoming_hemisphere.x) + openpbr_square(incoming_hemisphere.y);
    // Construct an orthonormal basis with transformed incoming direction as normal.
    vec3 tangent;
    if (incoming_hemisphere_xy_length_squared > 0.0f)
    {
        const float incoming_hemisphere_xy_length_inverse = openpbr_fast_rcp_sqrt(incoming_hemisphere_xy_length_squared);
        tangent = vec3(-incoming_hemisphere.y, incoming_hemisphere.x, 0.0f) * incoming_hemisphere_xy_length_inverse;
    }
    else
    {
        tangent = vec3(1.0f, 0.0f, 0.0f);
    }
    const vec3 bitangent = cross(incoming_hemisphere, tangent);
    // Uniformly sample a disk.
    const float radius = openpbr_fast_sqrt(rand.x);
    const float angle = OpenPBR_TwoPi * rand.y;
    const float sin_angle = sin(angle);
    const float cos_angle = cos(angle);
    const float bitangent_component_unscaled = radius * sin_angle;
    const float tangent_component = radius * cos_angle;
    // Remap vertical coordinate to warp disk into projected hemisphere.
    const float scale = 0.5f * (1.0f + incoming_hemisphere.z);
    const float bitangent_component =
        (1.0f - scale) * openpbr_fast_sqrt(1.0f - openpbr_square(tangent_component)) + scale * bitangent_component_unscaled;
    // Project point on projected hemisphere back onto hemisphere.
    float normal_component_squared = 1.0f - (openpbr_square(tangent_component) + openpbr_square(bitangent_component));
    // Prevent negative number due to rounding error.
    if (normal_component_squared < 0.0f)
    {
        normal_component_squared = 0.0f;
    }
    vec3 microfacet_normal_hemisphere =
        tangent * tangent_component + bitangent * bitangent_component + incoming_hemisphere * openpbr_fast_sqrt(normal_component_squared);
    // Prevent microfacet normal below ground due to rounding error.
    if (microfacet_normal_hemisphere.z < 0.0f)
    {
        microfacet_normal_hemisphere.z = 0.0f;
    }
    // The inverse transformation for the sampled normal is the same
    // as the forward transformation for the incoming direction.
    return openpbr_fast_normalize(ellipsoid_to_hemisphere * microfacet_normal_hemisphere);
}

// Evaluate anisotropic Smith shadowing or masking function.
// Input vector is in local (z-up) space.
float openpbr_eval_aniso_smith_g1(const vec3 v, const vec2 alpha)
{
    const float lambda =
        (-1.0f + openpbr_fast_sqrt(1.0f + (openpbr_square(alpha.x) * openpbr_square(v.x) + openpbr_square(alpha.y) * openpbr_square(v.y)) /
                                              openpbr_square(v.z))) /
        2.0f;
    return 1.0f / (1.0f + lambda);
}

// Evaluate anisotropic Smith non-correlated shadowing and masking function.
// Input vectors are in local (z-up) space.
float openpbr_eval_aniso_smith_g2(const vec3 v1, const vec3 v2, const vec2 alpha)
{
    return openpbr_eval_aniso_smith_g1(v1, alpha) * openpbr_eval_aniso_smith_g1(v2, alpha);
}

// Weights a color by the path throughput and then returns the max component of the product.
// Used for estimating the average contribution of a BSDF lobe, which is used for lobe selection.
float openpbr_max_component_of_throughput_weighted_color(const vec3 path_throughput, const vec3 x)
{
    OPENPBR_ASSERT(all(greaterThanEqual(path_throughput, vec3(0.0f))), "Throughput is expected to be non-negative");
    const vec3 throughput_weighted_color = path_throughput * x;
    return openpbr_max3(throughput_weighted_color);
}

// Converts IOR to F0.
OPENPBR_GENERAL_CONSTEXPR_FUNCTION float openpbr_f0_from_ior(const float eta_t_over_eta_i)
{
    return openpbr_square((eta_t_over_eta_i - 1.0f) / (eta_t_over_eta_i + 1.0f));
}

// Converts F0 to IOR.
float openpbr_ior_from_f0(const float f0)
{
    OPENPBR_ASSERT(f0 < 1.0f, "Can only compute valid finite IOR for incident reflectivity less than one.");
    const float sqrt_f0 = openpbr_fast_sqrt(f0);
    const float eta_t_over_eta_i = (1.0f + sqrt_f0) / (1.0f - sqrt_f0);  // infinity if f0 is 1
    return eta_t_over_eta_i;
}

// Adjusts the given IOR using the ASM-style "specular level" param.
// When the "specular level" is 0.5, this function is a no-op.
float openpbr_apply_specular_level_to_ior(const float eta_t_over_eta_i, const float specular_level)
{
    const float unscaled_f0 = openpbr_f0_from_ior(eta_t_over_eta_i);
    const float scaled_f0 = unscaled_f0 * specular_level * 2.0f;
    OPENPBR_CONSTEXPR_LOCAL float MaxF0 = 0.9999f;
    const float clamped_scaled_f0 = min(scaled_f0, MaxF0);
    const float external_ior = openpbr_ior_from_f0(clamped_scaled_f0);
    const bool internal_reflection = eta_t_over_eta_i < 1.0f;
    return internal_reflection ? 1.0f / external_ior : external_ior;
}

// Adjusts the given IOR using the OpenPBR "specular weight" param.
// When the "specular weight" is 1.0, this function is a no-op.
float openpbr_apply_specular_weight_to_ior(const float eta_t_over_eta_i, const float specular_weight)
{
    const float unscaled_f0 = openpbr_f0_from_ior(eta_t_over_eta_i);
    const float scaled_f0 = unscaled_f0 * specular_weight;
    OPENPBR_CONSTEXPR_LOCAL float MaxF0 = 0.9999f;
    const float clamped_scaled_f0 = min(scaled_f0, MaxF0);
    const float external_ior = openpbr_ior_from_f0(clamped_scaled_f0);
    const bool internal_reflection = eta_t_over_eta_i < 1.0f;
    return internal_reflection ? 1.0f / external_ior : external_ior;
}

// Calculates Schlick Fresnel reflectivity given color F0.
vec3 openpbr_schlick(const float cos_theta, const vec3 f0)
{
    return f0 + (vec3(1.0f) - f0) * openpbr_fifth_power(1.0f - abs(cos_theta));
}

// Calculates Schlick Fresnel t reflectivity given scalar F0.
float openpbr_schlick(const float cos_theta, const float f0)
{
    return f0 + (1.0f - f0) * openpbr_fifth_power(1.0f - min(abs(cos_theta), 1.0f));
}

// Calculates Schlick Fresnel reflectivity adjusted to handle internal reflection.
// eta_i is incoming medium's IOR, eta_t is transmitted medium's IOR, and the sign of cos_theta does not matter.
float openpbr_schlick_with_tir(const float eta_t_over_eta_i, const float cos_theta)
{
    float adjusted_cos_theta = cos_theta;

    if (eta_t_over_eta_i < 1.0f)
    {
        const float sin_t2 = (1.0f - openpbr_square(adjusted_cos_theta)) / openpbr_square(eta_t_over_eta_i);
        if (sin_t2 > 1.0f)
            return 1.0f;  // TIR

        // The use of openpbr_fast_sqrt() here can lead to numerical issues
        adjusted_cos_theta = sqrt(1.0f - sin_t2);
    }

    const float f0 = openpbr_f0_from_ior(eta_t_over_eta_i);

    return openpbr_schlick(adjusted_cos_theta, f0);
}

// Calculates real unpolarized Fresnel reflectivity.
float openpbr_fresnel(const float eta_t_over_eta_i, const float cos_theta_i)
{
    OPENPBR_ASSERT(cos_theta_i >= 0.0f, "Fresnel input cosine must be non-negative");
    if (eta_t_over_eta_i == 1.0f)
        return 0.0f;  // indexed-matched media
    const float sin_theta_i_squared = 1.0f - openpbr_square(cos_theta_i);
    const float sin_theta_t_squared = sin_theta_i_squared / openpbr_square(eta_t_over_eta_i);
    if (sin_theta_t_squared >= 1.0f)
        return 1.0f;  // total internal reflection (or grazing reflection)
    const float cos_theta_t = openpbr_fast_sqrt(1.0f - sin_theta_t_squared);
    const float eta_t_over_eta_i_cos_theta_i = eta_t_over_eta_i * cos_theta_i;
    const float eta_t_over_eta_i_cos_theta_t = eta_t_over_eta_i * cos_theta_t;
    const float Rs = openpbr_square((cos_theta_i - eta_t_over_eta_i_cos_theta_t) / (cos_theta_i + eta_t_over_eta_i_cos_theta_t));
    const float Rp = openpbr_square((cos_theta_t - eta_t_over_eta_i_cos_theta_i) / (cos_theta_t + eta_t_over_eta_i_cos_theta_i));
    return 0.5f * (Rs + Rp);  // unpolarized
}

// This is an RGB version of the Fresnel function.
// The OpenPBR specialization constant for ENABLE_DISPERSION should be passed into enable_dispersion.
vec3 openpbr_fresnel_rgb(const vec3 eta_t_over_eta_i, const float cos_theta_i, const bool enable_dispersion)
{
    if (enable_dispersion)
        return vec3(openpbr_fresnel(eta_t_over_eta_i.r, cos_theta_i),
                    openpbr_fresnel(eta_t_over_eta_i.g, cos_theta_i),
                    openpbr_fresnel(eta_t_over_eta_i.b, cos_theta_i));
    else
        return vec3(openpbr_fresnel(eta_t_over_eta_i.r, cos_theta_i));
}

// Approximates the cosine-weighted average of the Fresnel reflection coefficient over the hemisphere.
// See https://blog.selfshadow.com/publications/s2017-shading-course/imageworks/s2017_pbs_imageworks_slides_v2.pdf, slide 18.
float openpbr_average_fresnel(const float eta_t_over_eta_i)
{
    if (eta_t_over_eta_i > 1.0f)
        return (eta_t_over_eta_i - 1.0f) / (4.08567f + 1.00071f * eta_t_over_eta_i);
    else
        return 0.997118f + 0.1014f * eta_t_over_eta_i - 0.965241f * (eta_t_over_eta_i * eta_t_over_eta_i) -
               0.130607f * (eta_t_over_eta_i * eta_t_over_eta_i) * eta_t_over_eta_i;
}

// Calculates the total reflectance from the front and back of a thin-walled surface,
// including all internal bounces. You can think of this as the reflectance you would
// see from a single surface representing a glass windowpane, for example.
float openpbr_thin_wall_fresnel(const float eta_t_over_eta_i, const float cos_theta_i)
{
    const float front_reflectance = openpbr_fresnel(eta_t_over_eta_i, cos_theta_i);
    const float total_reflectance = (2.0f * front_reflectance) / (1.0f + front_reflectance);
    return total_reflectance;
    // Note that total transmittance = 1 - (2 * R) / (1 + R) = (1 - R) / (1 + R).
}

// This is a modified version of Naty Hoffman's F82 model for metallic Fresnel, which was in turn
// based on the Lazanyi-Schlick approximation. We can call this new formulation the F82-tint model
// because it allows the edge tint to be applied multiplicatively rather than being used directly
// as the reflectivity. This formulation has the new property that it reduces to regular Schlick
// when the edge tint is set to a universal default value of white. Like the original F82 model,
// it can approximate the real Fresnel dip by using a custom reflectivity at approximately
// 82 degrees, and it can also be used for creative control as the edge tint is clearly visible
// regardless of the selected colors.

vec3 openpbr_compute_metal_schlick_b_factor(const vec3 f0, const vec3 f82_tint)
{
    OPENPBR_CONSTEXPR_LOCAL float CosThetaMax = 1.0f / 7.0f;
    OPENPBR_CONSTEXPR_LOCAL float OneMinusCosThetaMax = 1.0f - CosThetaMax;
    OPENPBR_CONSTEXPR_LOCAL float OneMinusCosThetaMaxToTheFifth = openpbr_fifth_power(OneMinusCosThetaMax);
    OPENPBR_CONSTEXPR_LOCAL float OneMinusCosThetaMaxToTheSixth = openpbr_sixth_power(OneMinusCosThetaMax);
    const vec3 r = f0;        // switch to terminology from the "Fresnel Equations Considered Harmful" slides
    const vec3 t = f82_tint;  // custom terminology: "t" (for "tint") = h / (r + (1 - r) * (1 - CosThetaMax)^5)
    const vec3 white_minus_r = vec3(1.0f) - r;
    const vec3 white_minus_t = vec3(1.0f) - t;
    const vec3 b_numerator = (r + white_minus_r * OneMinusCosThetaMaxToTheFifth) * white_minus_t;
    OPENPBR_CONSTEXPR_LOCAL float BDenominator = CosThetaMax * OneMinusCosThetaMaxToTheSixth;
    OPENPBR_CONSTEXPR_LOCAL float BDenominatorReciprocal = 1.0f / BDenominator;
    return b_numerator * BDenominatorReciprocal;  // analogous to "a" in the "Fresnel Equations Considered Harmful" slides
}

vec3 openpbr_metal_schlick_with_f82_tint(const vec3 f0, const vec3 f82_tint, const float cos_theta)
{
    OPENPBR_ASSERT(cos_theta >= 0.0f, "F82-tint input cosine must be non-negative");
    const vec3 r = f0;  // switch to terminology from the "Fresnel Equations Considered Harmful" slides
    const vec3 white_minus_r = vec3(1.0f) - r;
    const vec3 b = openpbr_compute_metal_schlick_b_factor(f0, f82_tint);
    const float one_minus_cos_theta = 1.0f - cos_theta;
    const vec3 offset_from_r = (white_minus_r - b * cos_theta * one_minus_cos_theta) * openpbr_fifth_power(one_minus_cos_theta);
    const vec3 f_theta = r + offset_from_r;
    return saturate(f_theta);
}

// Returns the cosine-weighted average of the F82 metallic Fresnel function over the hemisphere.
vec3 openpbr_metal_average_fresnel_with_f82_tint(const vec3 f0, const vec3 f82_tint)
{
    const vec3 r = f0;  // switch to terminology from the "Fresnel Equations Considered Harmful" slides
    const vec3 white_minus_r = vec3(1.0f) - r;
    const vec3 b = openpbr_compute_metal_schlick_b_factor(f0, f82_tint);
    const vec3 average_albedo = r + white_minus_r * (1.0f / 21.0f) - b * (1.0f / 126.0f);
    return saturate(average_albedo);
}

// Calculates refracted direction.
bool openpbr_refract(const vec3 wi,
                     const vec3 n,
                     const float cos_theta_i,
                     const float eta_t_over_eta_i,
                     OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) wt)
{
    const float eta_i_over_eta_t = 1.0f / eta_t_over_eta_i;

    // Compute cos_theta_t using Snell's law.
    const float sin2_theta_i = max(0.0f, float(1.0f - openpbr_square(cos_theta_i)));
    const float sin2_theta_t = openpbr_square(eta_i_over_eta_t) * sin2_theta_i;

    // Handle total internal reflection for transmission.
    if (sin2_theta_t >= 1.0f)
    {
        wt = vec3(0.0f);
        return false;
    }

    // The use of openpbr_fast_sqrt() here will lead to divide by zero in PDFs
    const float cos_theta_t = sqrt(1.0f - sin2_theta_t);

    // Technically this vector is normalized by construction, but rounding error can be significant.
    wt = openpbr_fast_normalize(-wi * eta_i_over_eta_t + n * (eta_i_over_eta_t * cos_theta_i - cos_theta_t));
    return true;
}

// In lobes relying on tabulated data, this function prevents NaNs when dividing by an
// average energy complement that has been decoded from low-precision fixed-point form.
float openpbr_clamp_average_energy_complement_above_zero(const float average_energy_complement)
{
    OPENPBR_CONSTEXPR_LOCAL float min_value = 1e-12f;
    return max(average_energy_complement, min_value);
}

float openpbr_combine_coat_and_base_pdfs(const float pdf_coat, const float pdf_base, const float coat_reflection_prob)
{
    return mix(pdf_base, pdf_coat, coat_reflection_prob);
}

// Clears the output parameters of a lobe sampling function when sampling fails.
void openpbr_clear_lobe_sampling_output(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                                        OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    light_direction = vec3(0.0f);
    weight = openpbr_make_zero_diffuse_specular();
    pdf = 0.0f;
    sampled_type = OpenPBR_BsdfLobeTypeNone;
}

#endif  // !OPENPBR_LOBE_UTILS_H
