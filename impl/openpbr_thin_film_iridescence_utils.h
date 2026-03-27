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

#ifndef OPENPBR_THIN_FILM_IRIDESCENCE_UTILS_H
#define OPENPBR_THIN_FILM_IRIDESCENCE_UTILS_H

#include "../openpbr_constants.h"
#include "openpbr_complex.h"
#include "openpbr_math.h"

// Iridescence-specific constants
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_MinDenomMag = 1e-9f;  // Minimum denominator magnitude to prevent division by zero

// Struct for polarized scalar quantities
struct OpenPBR_PolarizedFloat
{
    float s;  // S-polarization component
    float p;  // P-polarization component
};

// Struct for polarized complex numbers
struct OpenPBR_PolarizedComplex
{
    openpbr_complex s;  // S-polarization complex component
    openpbr_complex p;  // P-polarization complex component
};

// Struct for polarized color quantities (per color channel)
struct OpenPBR_PolarizedColor
{
    vec3 s;  // S-polarization components for RGB
    vec3 p;  // P-polarization components for RGB
};

// Enforce physical branch: choose cos(theta_t) such that Im(eta_t * cos(theta_t)) >= 0
openpbr_complex openpbr_enforce_decaying_branch(const openpbr_complex eta_t, openpbr_complex cos_theta_t)
{
    const openpbr_complex eta_t_cos = openpbr_complex_multiply(eta_t, cos_theta_t);
    if (eta_t_cos.y < 0.0f)
        cos_theta_t = openpbr_complex_negate(cos_theta_t);
    return cos_theta_t;
}

// Unified Snell's Law: works for real or complex eta_t. Handles TIR by producing complex cos_theta_t.
openpbr_complex openpbr_snell_cos_unified(const float cos_theta_i, const float eta_i, const openpbr_complex eta_t)
{
    const float sin_theta_i = sqrt(saturate(1.0f - cos_theta_i * cos_theta_i));

    // Apply Snell's Law: sin(theta_t) = (eta_i / eta_t) * sin(theta_i)
    const openpbr_complex eta_i_over_eta_t =
        openpbr_complex_divide(openpbr_complex(eta_i, 0.0f), eta_t, OpenPBR_MinDenomMag, openpbr_complex(1.0f, 0.0f));

    const openpbr_complex sin_theta_t = openpbr_complex_scalar_multiply(sin_theta_i, eta_i_over_eta_t);

    // Calculate cos(theta_t) from sin(theta_t) using sin^2 + cos^2 = 1
    const openpbr_complex sin_theta_t_squared = openpbr_complex_multiply(sin_theta_t, sin_theta_t);
    const openpbr_complex cos_theta_t_squared = openpbr_complex_subtract(openpbr_complex(1.0f, 0.0f), sin_theta_t_squared);

    openpbr_complex cos_theta_t = openpbr_complex_sqrt(cos_theta_t_squared);

    // Enforce physical branch selection: choose sign such that Im(eta_t * cos_theta_t) >= 0
    cos_theta_t = openpbr_enforce_decaying_branch(eta_t, cos_theta_t);

    return cos_theta_t;
}

// Unified Fresnel amplitude coefficients (s/p), valid for dielectric or conductor eta_t
// Returns complex r and t, and complex cos_theta_t
// Handles total internal reflection (TIR) by returning complex coefficients with phase information
void openpbr_compute_fresnel_unified_polarized_amplitude(
    const float cos_theta_i,
    const float eta_i,
    const openpbr_complex eta_t,  // eta_t = n_t + i * k_t (complex IOR for conductors, or n_t + i*0 for dielectrics)
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_PolarizedComplex) r,
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_PolarizedComplex) t,
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(openpbr_complex) cos_theta_t)
{
    // Apply Snell's Law to compute complex cos(theta_t), which handles TIR by producing complex angles
    cos_theta_t = openpbr_snell_cos_unified(cos_theta_i, eta_i, eta_t);

    // Calculate intermediate products used in Fresnel equations
    const openpbr_complex eta_i_cos_theta_i = openpbr_complex(eta_i * cos_theta_i, 0.0f);
    const openpbr_complex eta_t_cos_theta_t = openpbr_complex_multiply(eta_t, cos_theta_t);
    const openpbr_complex eta_t_cos_theta_i = openpbr_complex_scalar_multiply(cos_theta_i, eta_t);
    const openpbr_complex eta_i_cos_theta_t = openpbr_complex_scalar_multiply(eta_i, cos_theta_t);

    // Transmission numerator is the same for both s and p polarizations
    const openpbr_complex t_numerator = openpbr_complex(2.0f * eta_i * cos_theta_i, 0.0f);

    // For s-polarization:
    // r_s = (eta_i * cos_theta_i - eta_t * cos_theta_t) / (eta_i * cos_theta_i + eta_t * cos_theta_t)
    // t_s = (2 * eta_i * cos_theta_i) / (eta_i * cos_theta_i + eta_t * cos_theta_t)
    const openpbr_complex r_s_numerator = openpbr_complex_subtract(eta_i_cos_theta_i, eta_t_cos_theta_t);
    const openpbr_complex denominator_s = openpbr_complex_add(eta_i_cos_theta_i, eta_t_cos_theta_t);
    r.s = openpbr_complex_divide(r_s_numerator, denominator_s, OpenPBR_MinDenomMag, openpbr_complex(1.0f, 0.0f));
    t.s = openpbr_complex_divide(t_numerator, denominator_s, OpenPBR_MinDenomMag, openpbr_complex(0.0f, 0.0f));

    // For p-polarization:
    // r_p = (eta_t * cos_theta_i - eta_i * cos_theta_t) / (eta_t * cos_theta_i + eta_i * cos_theta_t)
    // t_p = (2 * eta_i * cos_theta_i) / (eta_t * cos_theta_i + eta_i * cos_theta_t)
    const openpbr_complex r_p_numerator = openpbr_complex_subtract(eta_t_cos_theta_i, eta_i_cos_theta_t);
    const openpbr_complex denominator_p = openpbr_complex_add(eta_t_cos_theta_i, eta_i_cos_theta_t);
    r.p = openpbr_complex_divide(r_p_numerator, denominator_p, OpenPBR_MinDenomMag, openpbr_complex(1.0f, 0.0f));
    t.p = openpbr_complex_divide(t_numerator, denominator_p, OpenPBR_MinDenomMag, openpbr_complex(0.0f, 0.0f));
}

// Compute amplitude Fresnel coefficients for dielectrics (outputs PolarizedFloat)
// This is a fast path for the purely dielectric case (exterior-to-film interface),
// using real arithmetic instead of the complex arithmetic required by the unified function
// This is more efficient when we know both materials are dielectrics
// For the film-to-base interface, we use the unified function since it may encounter TIR, requiring complex coefficients to preserve phase information
// Note: Could be vectorized to output PolarizedColor, but the TIR branching would add complexity that likely outweighs vectorization benefits
void openpbr_compute_fresnel_dielectric_polarized_amplitude(const float cos_theta_i,
                                                            const float eta_i,
                                                            const float eta_t,
                                                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_PolarizedFloat)
                                                                r,  // Amplitude reflection coefficients
                                                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_PolarizedFloat)
                                                                t,  // Amplitude transmission coefficients
                                                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) cos_theta_t)
{
    const float sin_theta_i = sqrt(saturate(1.0f - cos_theta_i * cos_theta_i));

    // Apply Snell's Law
    const float sin_theta_t = eta_i / eta_t * sin_theta_i;

    // Check for total internal reflection
    if (sin_theta_t >= 1.0f)
    {
        r.s = 1.0f;
        r.p = 1.0f;
        t.s = 0.0f;
        t.p = 0.0f;
        cos_theta_t = 0.0f;
        return;
    }

    // Calculate the cosine of the angle of refraction, which will always be positive
    cos_theta_t = sqrt(saturate(1.0f - sin_theta_t * sin_theta_t));

    // Calculate intermediate products
    const float eta_i_cos_theta_i = eta_i * cos_theta_i;
    const float eta_t_cos_theta_t = eta_t * cos_theta_t;
    const float eta_t_cos_theta_i = eta_t * cos_theta_i;
    const float eta_i_cos_theta_t = eta_i * cos_theta_t;

    // Calculate denominators
    OpenPBR_PolarizedFloat denom;
    denom.s = eta_i_cos_theta_i + eta_t_cos_theta_t;
    denom.p = eta_t_cos_theta_i + eta_i_cos_theta_t;

    // Prevent division by zero
    denom.s = (abs(denom.s) < OpenPBR_MinDenomMag) ? (denom.s >= 0.0f ? OpenPBR_MinDenomMag : -OpenPBR_MinDenomMag) : denom.s;
    denom.p = (abs(denom.p) < OpenPBR_MinDenomMag) ? (denom.p >= 0.0f ? OpenPBR_MinDenomMag : -OpenPBR_MinDenomMag) : denom.p;

    // Precompute reciprocal denominators to minimize number of divisions
    OpenPBR_PolarizedFloat rcp_denom;
    rcp_denom.s = 1.0f / denom.s;
    rcp_denom.p = 1.0f / denom.p;

    // Amplitude reflection coefficients for s and p polarizations
    r.s = (eta_i_cos_theta_i - eta_t_cos_theta_t) * rcp_denom.s;
    r.p = (eta_t_cos_theta_i - eta_i_cos_theta_t) * rcp_denom.p;

    // Amplitude transmission coefficients for s and p polarizations
    t.s = (2.0f * eta_i_cos_theta_i) * rcp_denom.s;
    t.p = (2.0f * eta_i_cos_theta_i) * rcp_denom.p;
}

// Gulbrandsen's helper functions for converting two colors to a complex IOR

float openpbr_n_min(const float clamped_r)
{
    return (1.0f - clamped_r) / (1.0f + clamped_r);
}

float openpbr_n_max(const float clamped_r)
{
    const float sqrt_clamped_r = sqrt(clamped_r);
    return (1.0f + sqrt_clamped_r) / (1.0f - sqrt_clamped_r);
}

float openpbr_get_n(const float clamped_r, const float g)
{
    return mix(openpbr_n_max(clamped_r), openpbr_n_min(clamped_r), g);
}

float openpbr_get_k_squared(float clamped_r, float n)
{
    const float numerator = openpbr_square(n + 1.0f) * clamped_r - openpbr_square(n - 1.0f);
    const float denominator = 1.0f - clamped_r;
    return numerator / denominator;
}

// Compute n and k for a metal using the Gulbrandsen method
// Uses the approach outlined in the paper "Artist Friendly Metallic Fresnel" by Gulbrandsen
// "r" and "g" are terms from the paper standing for "reflectivity" and "edge tint"
openpbr_complex openpbr_compute_gulbrandsen_n_and_k(const float r, const float g)
{
    // Clamp r as in the paper
    const float clamped_r = clamp(r, 0.0f, 0.99f);

    // Compute n and k using Gulbrandsen's method
    const float n = openpbr_get_n(clamped_r, g);
    const float k_squared = openpbr_get_k_squared(clamped_r, n);

    // Calculate k from k^2
    const float k = sqrt(max(k_squared, 0.0f));  // Ensure k^2 is non-negative

    // Format n and k as a complex number and return
    const openpbr_complex n_and_k = openpbr_complex(n, k);
    return n_and_k;
}

// Perform the Airy summation for a single polarization and wavelength
// This is equivalent to Equation 3 in the Belcour and Barla paper
float openpbr_compute_airy_reflectance(const float r12,            // Amplitude reflection coefficient at film-to-exterior interface
                                       const float t12,            // Amplitude transmission coefficient at film-to-exterior interface
                                       const float r21,            // Amplitude reflection coefficient from film to exterior (reverse direction)
                                       const float t21,            // Amplitude transmission coefficient from film to exterior
                                       const openpbr_complex r23,  // Complex amplitude reflection coefficient at base-to-film interface
                                       const float delta_phi       // Phase shift due to optical path difference
)
{
    // Exponential term e^(i * delta_phi)
    const openpbr_complex exp_i_delta_phi = openpbr_complex_exp_i(delta_phi);

    // Numerator: t12 * r23 * t21 * e^(i * delta_phi)
    const openpbr_complex numerator = openpbr_complex_scalar_multiply(t12 * t21, openpbr_complex_multiply(r23, exp_i_delta_phi));

    // Denominator: 1 - r21 * r23 * e^(i * delta_phi)
    const openpbr_complex denominator =
        openpbr_complex_subtract(openpbr_complex(1.0f, 0.0f), openpbr_complex_scalar_multiply(r21, openpbr_complex_multiply(r23, exp_i_delta_phi)));

    // Total complex reflection coefficient: r_total = r12 + numerator / denominator
    // This line completes the evaluation of Equation 3 from the Belcour and Barla paper
    // If division by zero occurs, the complex_divide function returns zero and only r12 contributes
    const openpbr_complex r_total =
        openpbr_complex_add(openpbr_complex(r12, 0.0f), openpbr_complex_divide(numerator, denominator, OpenPBR_MinDenomMag, openpbr_complex(0.0f)));

    // Reflectance (power coefficient) is the magnitude squared of the total reflection coefficient
    return openpbr_complex_magnitude_squared(r_total);
}

// Helper function to compute thin-film reflectance for a given base material
float openpbr_compute_combined_reflectance_for_thin_film_and_base(
    const OpenPBR_PolarizedFloat r12,    // Amplitude reflection coefficients at film-to-exterior interface (s and p)
    const OpenPBR_PolarizedFloat t12,    // Amplitude transmission coefficients at film-to-exterior interface (s and p)
    const OpenPBR_PolarizedFloat r21,    // Amplitude reflection coefficients from film to exterior (reverse direction) (s and p)
    const OpenPBR_PolarizedFloat t21,    // Amplitude transmission coefficients from film to exterior (s and p)
    const OpenPBR_PolarizedComplex r23,  // Complex amplitude reflection coefficients at base-to-film interface (s and p)
    const float delta_phi                // Phase shift due to optical path difference
)
{
    // S-polarization
    const float reflectance_s = openpbr_compute_airy_reflectance(r12.s, t12.s, r21.s, t21.s, r23.s, delta_phi);

    // P-polarization
    const float reflectance_p = openpbr_compute_airy_reflectance(r12.p, t12.p, r21.p, t21.p, r23.p, delta_phi);

    // Average over polarizations to get unpolarized reflectance
    const float reflectance = 0.5f * (reflectance_s + reflectance_p);

    return reflectance;
}

// Helper function to compute thin-film reflectance for a dielectric base
vec3 openpbr_compute_dielectric_reflectance(const OpenPBR_PolarizedFloat r12,
                                            const OpenPBR_PolarizedFloat t12,
                                            const OpenPBR_PolarizedFloat r21,
                                            const OpenPBR_PolarizedFloat t21,
                                            const float eta_film,
                                            const float cos_theta_t_film,
                                            const vec3 delta_phi,
                                            const vec3 eta_base,  // Refractive index(es) of the dielectric base
                                            const bool enable_dispersion)
{
    // Compute Fresnel coefficients at base-to-film interface for dielectric base
    // Note: Must keep complex r23 to preserve phase under TIR at the base interface
    OpenPBR_PolarizedComplex r23[OpenPBR_NumRgbChannels];

    for (int color_channel = 0; color_channel < OpenPBR_NumRgbChannels; ++color_channel)
    {
        // Calculate Fresnel coefficients independently for each color channel when there's dispersion
        // Otherwise, calculate the coefficients for only the first channel and reuse them for all channels
        if (enable_dispersion || color_channel == 0)
        {
            OpenPBR_PolarizedComplex t23_unused;
            openpbr_complex cos_theta_t_base_unused;

            // Unified complex Fresnel (dielectric base => eta_t is real, pass as complex(real, 0))
            openpbr_compute_fresnel_unified_polarized_amplitude(
                cos_theta_t_film, eta_film, openpbr_complex(eta_base[color_channel], 0.0f), r23[color_channel], t23_unused, cos_theta_t_base_unused);
        }
        else
        {
            // In this case there's no dispersion so just copy the coefficients from the first channel
            r23[color_channel] = r23[0];
        }
    }

    // Compute reflectance for each color channel
    vec3 reflectance_dielectric;
    for (int color_channel = 0; color_channel < OpenPBR_NumRgbChannels; ++color_channel)
    {
        reflectance_dielectric[color_channel] =
            openpbr_compute_combined_reflectance_for_thin_film_and_base(r12, t12, r21, t21, r23[color_channel], delta_phi[color_channel]);
    }

    return reflectance_dielectric;
}

// Helper function to compute thin-film reflectance for a metal base
vec3 openpbr_compute_metal_reflectance(const OpenPBR_PolarizedFloat r12,
                                       const OpenPBR_PolarizedFloat t12,
                                       const OpenPBR_PolarizedFloat r21,
                                       const OpenPBR_PolarizedFloat t21,
                                       const float eta_film,
                                       const float cos_theta_t_film,
                                       const vec3 delta_phi,
                                       const vec3 metal_F0,
                                       const vec3 metal_F82_tint)
{
    // Compute reflectance for each color channel
    vec3 reflectance_metal;
    for (int color_channel = 0; color_channel < OpenPBR_NumRgbChannels; ++color_channel)
    {
        // Compute complex IOR n and k from metal_F0 and metal_F82_tint
        const openpbr_complex n_and_k = openpbr_compute_gulbrandsen_n_and_k(metal_F0[color_channel], metal_F82_tint[color_channel]);

        // Compute Fresnel coefficients at the film-metal interface using unified function
        OpenPBR_PolarizedComplex r23_metal;
        OpenPBR_PolarizedComplex t23_unused;
        openpbr_complex cos_theta_t_base_unused;
        openpbr_compute_fresnel_unified_polarized_amplitude(cos_theta_t_film, eta_film, n_and_k, r23_metal, t23_unused, cos_theta_t_base_unused);

        // Compute and store reflectance for this color channel
        reflectance_metal[color_channel] =
            openpbr_compute_combined_reflectance_for_thin_film_and_base(r12, t12, r21, t21, r23_metal, delta_phi[color_channel]);
    }

    return reflectance_metal;
}

// Struct to hold the results of the thin film and base reflectance computations
// The caller can weight and combine these results as needed
// The caller can also calculate the transmittance by subtracting the reflectance from 1
struct OpenPBR_ThinFilmResults
{
    vec3 reflectance_dielectric;  // Reflectance of the dielectric base
    vec3 reflectance_metal;       // Reflectance of the metal base
};

// Main function to compute reflectance (RGB power coefficient) of combination of thin film and base surface
// In the rare case that dispersion of the dielectric IOR is needed, this function can be called separately for each color channel
OpenPBR_ThinFilmResults
openpbr_thin_film_and_base_reflectance(const float cos_theta_i,       // Cosine of the angle of incidence from the exterior medium (assumed >= 0)
                                       const float eta_exterior,      // Refractive index of the exterior medium (e.g., air)
                                       const float eta_film,          // Refractive index of the thin film
                                       const vec3 eta_base,           // Refractive index(es) of the first dielectric base material
                                       const bool enable_dispersion,  // Enable dispersion of the dielectric IOR
                                       const vec3 metal_F0,           // Metal reflectivity at normal incidence (power coefficient, color)
                                       const vec3 metal_F82_tint,     // Tint of metal reflectivity at 82-degree angle (power coefficient, color)
                                       const bool enable_dielectric,  // Enable the first dielectric base
                                       const bool enable_metal,       // Enable the metal base
                                       const float thin_film_thickness_nm,  // Thickness of the thin film (in nanometers)
                                       const vec3 lambda_rgb_nm             // Wavelengths for R, G, B (in nanometers)
)
{
    // 1. Common Calculations

    // Ensure cos_theta_i is non-negative
    // Otherwise, this function should not have been called
    OPENPBR_ASSERT(cos_theta_i >= 0.0f, "cos_theta_i must be non-negative");

    // Ensure thin_film_thickness_nm is positive
    // Otherwise, there is no thin film to compute
    // The caller should have caught this by checking thin_film_presence_multiplier
    OPENPBR_ASSERT(thin_film_thickness_nm > 0.0f, "thin-film thickness must be positive");

    // Default the results to zero in case some base components are disabled
    OpenPBR_ThinFilmResults results;
    results.reflectance_dielectric = vec3(0.0f);
    results.reflectance_metal = vec3(0.0f);

    // Return if all base components are disabled
    // If the caller were to check this, this early exit could be replaced with an assertion
    if (!(enable_dielectric || enable_metal))
        return results;

    // Compute Fresnel coefficients at film-to-exterior interface
    float cos_theta_t_film;
    OpenPBR_PolarizedFloat r12;  // Amplitude reflection coefficients at film-to-exterior interface
    OpenPBR_PolarizedFloat t12;  // Amplitude transmission coefficients at film-to-exterior interface
    openpbr_compute_fresnel_dielectric_polarized_amplitude(cos_theta_i, eta_exterior, eta_film, r12, t12, cos_theta_t_film);

    // Check for total internal reflection on the outside of the film
    if (cos_theta_t_film <= 0.0f)
    {
        // Total internal reflection occurs on the outside of the film, so set all reflectances to one and return
        if (enable_dielectric)
            results.reflectance_dielectric = vec3(1.0f);
        if (enable_metal)
            results.reflectance_metal = vec3(1.0f);
        return results;
    }

    // Reflection coefficients for reverse direction
    // r21 = -r12 to account for phase shift by 180 degrees
    OpenPBR_PolarizedFloat r21;
    r21.s = -r12.s;
    r21.p = -r12.p;

    // Transmission coefficients for reverse direction
    // t21 is derived from r12 using the appropriate reciprocity relation
    // Note that amplitude transmission coefficients can be greater than one due to refraction
    OpenPBR_PolarizedFloat t21;
    const float safe_cos_theta_i = max(cos_theta_i, OpenPBR_MinDenomMag);  // Prevent numerical issues at grazing incidence where cos_theta_i -> 0
    const float t21_scale = (eta_film * cos_theta_t_film) / (eta_exterior * safe_cos_theta_i);
    t21.s = t12.s * t21_scale;
    t21.p = t12.p * t21_scale;

    // Compute optical path difference and phase shift per color channel
    const float opd = 2.0f * eta_film * thin_film_thickness_nm * cos_theta_t_film;
    const vec3 delta_phi = 2.0f * OpenPBR_Pi * opd / lambda_rgb_nm;

    // 2. Dielectric Base Calculations

    // Compute reflectance for dielectric base component
    if (enable_dielectric)
        results.reflectance_dielectric = openpbr_compute_dielectric_reflectance(r12,
                                                                                t12,
                                                                                r21,
                                                                                t21,
                                                                                eta_film,
                                                                                cos_theta_t_film,
                                                                                delta_phi,
                                                                                eta_base,  // Refractive index of the first dielectric base
                                                                                enable_dispersion);

    // 3. Metal Base Calculations

    // Compute reflectance for metal base component
    if (enable_metal)
        results.reflectance_metal =
            openpbr_compute_metal_reflectance(r12, t12, r21, t21, eta_film, cos_theta_t_film, delta_phi, metal_F0, metal_F82_tint);

    // 4. Return Final Reflectance Results

    return results;
}

// Handle very thin films by scaling the thin-film presence by this thickness-based multiplier
float openpbr_thin_film_presence_multiplier(const float thin_film_thickness_nm)
{
    // Thickness below which the thin film is fully invisible
    OPENPBR_CONSTEXPR_LOCAL float FullInvisibilityThreshold_nm = OpenPBR_MinDenomMag;  // Slightly positive value to avoid potential numerical issues

    // Thickness above which the thin film is fully visible
    OPENPBR_CONSTEXPR_LOCAL float FullVisibilityThreshold_nm = 30.0f;  // 30 nanometers

    return smoothstep(FullInvisibilityThreshold_nm, FullVisibilityThreshold_nm, thin_film_thickness_nm);
}

// Desaturates the thin film reflectance based on thin film thickness and the cosine of the angle of incidence (assumed >= 0).
// This technique is used by the rasterizer to match the path tracer thin film reflectance while using constant RGB wavelengths.
OpenPBR_ThinFilmResults openpbr_desaturate_thin_film_reflectance(const vec3 dielectric_reflectance,
                                                                 const vec3 metal_reflectance,
                                                                 const float thin_film_thickness_nm,
                                                                 const float idoth)
{
    OpenPBR_ThinFilmResults desaturated_reflectance;
    const float thin_film_desaturation_scale = min(sqrt(thin_film_thickness_nm * 0.001f * idoth), 1.0f);

    desaturated_reflectance.reflectance_dielectric =
        mix(dielectric_reflectance, vec3(openpbr_average(dielectric_reflectance)), thin_film_desaturation_scale);
    desaturated_reflectance.reflectance_metal = mix(metal_reflectance, vec3(openpbr_average(metal_reflectance)), thin_film_desaturation_scale);

    return desaturated_reflectance;
}

#endif  // !OPENPBR_THIN_FILM_IRIDESCENCE_UTILS_H
