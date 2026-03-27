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

#ifndef OPENPBR_BSDF_H
#define OPENPBR_BSDF_H

#include "../openpbr_basis.h"
#include "../openpbr_bsdf_lobe_type.h"
#include "../openpbr_constants.h"
#include "../openpbr_diffuse_specular.h"
#include "../openpbr_resolved_inputs.h"
#include "../openpbr_volume_homogeneous.h"

#include "openpbr_dispersion_utils.h"
#include "openpbr_fuzz_lobe.h"  // only need to include outermost lobe
#include "openpbr_math.h"
#include "openpbr_sampling.h"
#include "openpbr_specialization_constants.h"
#include "openpbr_volume_utils.h"

// Volume-derived properties computed during cache_material_volume.
struct OpenPBR_VolumeDerivedProps
{
    // The properties below are related to special cases that occur with volumes, specifically
    // thin-wall subsurface (for things like a pane of glass or a leaf) and zero-depth absorption.
    vec3 thin_wall_subsurface_color;        // color of subsurface component if in thin-wall mode
    float thin_wall_subsurface_anisotropy;  // anisotropy of subsurface component if in thin-wall mode
    vec3 transmission_tint;                 // tint from zero-distance absorption (needs to be made view-dependent in thin-wall mode)
};

// Final prepared outputs: volume and cached BSDF lobes ready for the integrator.
struct OpenPBR_PreparedBsdf
{
    OpenPBR_HomogeneousVolume volume;                      // final volume to hand to integrator
    OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe fuzz_lobe;  // fully cached lobes ready for eval/pdf/sample
    vec3 view_direction;                                   // view direction used during preparation (must match at eval/sample/pdf)
};

// Determines whether RGB wavelengths are needed for dispersion or thin film effects.
// This is used by the material layer to decide whether to generate RGB wavelengths for the path.
bool openpbr_needs_rgb_wavelengths(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs)
{
    const bool needs_rgb_wavelengths_for_dispersion = OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion) &&
                                                      resolved_inputs.transmission_dispersion_scale > 0.0f && resolved_inputs.base_metalness < 1.0f;
    const float thin_film_thickness_nm = resolved_inputs.thin_film_thickness * 1000.0f;
    const bool needs_rgb_wavelengths_for_thin_film = resolved_inputs.thin_film_weight > 0.0f && thin_film_thickness_nm > 0.0f;
    return needs_rgb_wavelengths_for_dispersion || needs_rgb_wavelengths_for_thin_film;
}

// Derives volume properties and transmission tint from resolved inputs.
// This is the BSDF layer's volume setup logic - it derives volume_derived_props and the final volume
// from the resolved material inputs. Must be called after texture evaluation populates resolved_inputs.
void openpbr_prepare_volume(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_VolumeDerivedProps) volume_derived_props,
                            OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_HomogeneousVolume) volume,
                            const bool volumes_enabled)
{
    // All inputs are now in resolved_inputs - this function derives volume properties and stores them in volume_derived_props.
    // Some properties are needed regardless of whether volumes are enabled (e.g., thin_wall),
    // while others depend on the volumes_enabled flag.

    if (volumes_enabled)
    {
        // Zero-initialize volume-derived local variables and thin-wall parameters to avoid unintended use of uninitialized data.
        OpenPBR_HomogeneousVolume subsurface_volume = openpbr_make_empty_volume();
        volume_derived_props.thin_wall_subsurface_color = vec3(0.0f);
        volume_derived_props.thin_wall_subsurface_anisotropy = 0.0f;

        // Calculate the volume that results from subsurface scattering.
        // Or if in thin-wall mode, set the BSDF params necessary to convert subsurface to diffuse reflection and transmission.
        // Only set the non-zero parameters -- the other parameters are already zero-initialized above.
        if (resolved_inputs.subsurface_weight > 0.0f)
        {
            if (!resolved_inputs.geometry_thin_walled)
            {
                subsurface_volume = openpbr_create_subsurface_volume_from_openpbr_params(resolved_inputs.subsurface_radius,
                                                                                         resolved_inputs.subsurface_radius_scale,
                                                                                         resolved_inputs.subsurface_color,
                                                                                         resolved_inputs.subsurface_scatter_anisotropy);
            }
            else
            {
                // In thin-wall mode, subsurface turns into diffuse reflection and/or transmission.
                volume_derived_props.thin_wall_subsurface_color = resolved_inputs.subsurface_color;
                volume_derived_props.thin_wall_subsurface_anisotropy = resolved_inputs.subsurface_scatter_anisotropy;
            }
        }

        // Default-initialize transmission-related parameters to avoid unintended use of uninitialized data.
        // These will be set properly below if transmission is enabled.
        volume_derived_props.transmission_tint = vec3(1.0f);
        OpenPBR_HomogeneousVolume transmission_volume = openpbr_make_empty_volume();

        // Set the properties of the interior volume and the transmission color.
        // We can either have an interior volume or a transmission color, but not both.
        if (resolved_inputs.transmission_weight > 0.0f)
        {
            if (resolved_inputs.transmission_depth == 0.0f || resolved_inputs.geometry_thin_walled)  // special cases
            {
                // Handle the special case that occurs:
                //   - when the transmission depth is zero OR
                //   - when thin-wall mode is enabled.
                //
                // In the former case, the volume is replaced by a constant transmission tint at the surface.
                // In the latter case, the volume is replaced by a view-dependent transmission tint at the surface
                // (the view dependency itself is applied later during BSDF setup).
                volume_derived_props.transmission_tint = resolved_inputs.transmission_color;

                // Interior volume will not be used in this case, so leave it as an empty volume.
            }
            else  // regular case
            {
                // Calculate the properties of the interior volume.
                transmission_volume = openpbr_create_volume_from_openpbr_params(resolved_inputs.transmission_color,
                                                                                resolved_inputs.transmission_depth,
                                                                                resolved_inputs.transmission_scatter,
                                                                                resolved_inputs.transmission_scatter_anisotropy);

                // Leave transmission tint as white in this case so that the volume is not tinted by the transmission color.
                // This is necessary because the volume transmittance is calculated based on the transmission color,
                // and we want to avoid tinting the transmitted rays with this color (which would result in double color).
            }
        }

        // Combine subsurface and interior volumes into a single unified volume based on their relative contributions,
        // which should add up to one. Blending thick and thin volumetric effects results in medium-thickness volumetric effects.
        // Unlike stochastic blending, this produces physically plausible results.
        // TODO(sss): Consider combining the subsurface volume and the interior volume (volume) stochastically if it is deemed to be
        //            more artistically intuitive. If we do that, try to incorporate transmission_tint stochastically somehow too.
        const float subsurface_fraction_of_dielectric = (1.0f - resolved_inputs.transmission_weight) * resolved_inputs.subsurface_weight;
        const float transmission_fraction_of_dielectric = resolved_inputs.transmission_weight;
        const float subsurface_and_transmission_fraction_of_dielectric = subsurface_fraction_of_dielectric + transmission_fraction_of_dielectric;
        const float reciprocal_of_subsurface_and_transmission_fraction_of_dielectric =
            openpbr_safe_divide(1.0f, subsurface_and_transmission_fraction_of_dielectric, 0.0f);
        const OpenPBR_HomogeneousVolume combined_volume =
            openpbr_add_volumes(subsurface_volume,
                                transmission_volume,
                                subsurface_fraction_of_dielectric * reciprocal_of_subsurface_and_transmission_fraction_of_dielectric,
                                transmission_fraction_of_dielectric * reciprocal_of_subsurface_and_transmission_fraction_of_dielectric);
        volume = combined_volume;
    }
    else  // !volumes_enabled
    {
        if (!resolved_inputs.geometry_thin_walled)
        {
            // Zero-initialize the unneeded thin-wall subsurface parameters.
            volume_derived_props.thin_wall_subsurface_color = vec3(0.0f);
            volume_derived_props.thin_wall_subsurface_anisotropy = 0.0f;
        }
        else
        {
            // Volumes are not enabled, however in thin-wall mode subsurface still exists -- it is just converted to a non-volumetric effect.
            // So we need to properly initialize all parameters that are relevant to thin-wall subsurface scattering.

            // Calculate the amount of subsurface scattering.
            // Note that we calculate the value here regardless of the value of EnableTranslucency, since
            // EnableTranslucency could technically be set to false if the subsurface is entirely backward-scattering
            // (subsurface anisotropy of -1), in which case it is converted entirely to diffuse reflection.
            // (The value is already in resolved_inputs.subsurface_weight from texture evaluation above.)

            // In thin-wall mode, subsurface turns into diffuse reflection and transmission.
            volume_derived_props.thin_wall_subsurface_color = resolved_inputs.subsurface_color;
            volume_derived_props.thin_wall_subsurface_anisotropy = resolved_inputs.subsurface_scatter_anisotropy;
        }

        // Handle the special case that occurs when the transmission depth is zero or we're in thin-wall mode.
        // When volumes_enabled is true, transmission_tint is initialized in openpbr_prepare_volume.
        // When volumes_enabled is false, transmission_tint is initialized here instead.
        if (resolved_inputs.transmission_depth == 0.0f || resolved_inputs.geometry_thin_walled)  // special cases
        {
            volume_derived_props.transmission_tint =
                resolved_inputs
                    .transmission_color;  // tint transmitted rays (even in thin-wall mode since transmission distance is zero in this case)
                                          // TODO(OpenPBR): Consider using sqrt so that passing in and out of surface results in specified color
                                          //                and make sure this behavior is in OpenPBR spec.
        }
        else  // regular case
        {
            volume_derived_props.transmission_tint = vec3(1.0f);  // do not tint transmitted rays
        }

        // Initialize volume in case it's used in some code that doesn't take into
        // account whether volumes are enabled (such as during shadow-ray tracing).
        volume = openpbr_make_empty_volume();
    }
}

// Function to compute the final, potentially view-dependent (in the case of thin-wall) transmission tint.
// For context, in the OpenPBR spec, in thin-wall mode the transmission_color is defined to be the color
// due to volumetric absorption at normal incidence, so we need to adjust this color based on the actual
// angle of the refracted ray(s) traveling through the material.
// TODO: Take multiple reflections into account. (Otherwise, is it even worth doing this as it doesn't have much impact?)
vec3 openpbr_compute_final_transmission_tint(const vec3 transmission_tint,
                                             const bool thin_wall,
                                             const float cos_i,                        // non-negative cosine of the unrefracted view angle
                                             const float relative_ior_for_refraction)  // eta = n_t / n_i
{
    if (!thin_wall)
    {
        // If not view‐dependent, just return the base tint.
        return transmission_tint;
    }

    OPENPBR_ASSERT(cos_i >= 0.0f, "cos_i is expected to be non-negative for thin-wall view-dependent transmission tinting");

    // Compute sin^2(theta_i).
    // (Note that nothing will break if sin2_i is slightly negative.)
    const float sin2_i = 1.0f - openpbr_square(cos_i);

    // Compute sin^2(theta_t) = (sin^2(theta_i)) / eta^2.
    const float sin2_t = sin2_i / openpbr_square(relative_ior_for_refraction);

    // Total internal reflection: no transmission.
    if (sin2_t >= 1.0f)
    {
        return vec3(0.0f);
    }

    // Compute cos(theta_t) without trig.
    const float cos_t = sqrt(1.0f - sin2_t);

    // Path length through the thin sheet relative to normal incidence.
    // (In OpenPBR, the thin-wall transmittance due to absorption at normal incidence is defined
    //  to be represented by the transmission_color parameter, which we call transmission_tint here.)
    const float path_length = 1.0f / cos_t;

    // Apply volumetric absorption: raise tint to the path_length power.
    // Mathematically equivalent to using Beer-Lambert law:
    //     transmission_tint = exp(-mu_t)
    //     transmission_tint^path_length = exp(-mu_t)^path_length
    //     transmission_tint^path_length = exp(-mu_t * path_length)
    return pow(transmission_tint, vec3(path_length));
}

// Sets up lobes based on input params.
// Incoming direction must point away from the surface.
// Shading normal frame must be orthonormal.
void openpbr_prepare_lobes(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                           OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_VolumeDerivedProps) volume_derived_props,
                           OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_FuzzLobe_CoatingLobe_AggregateLobe) fuzz_lobe,
                           const vec3 path_throughput,
                           const vec3 rgb_wavelengths_nm,
                           const float exterior_ior,
                           const vec3 view_direction)
{
    // Determine whether we're hitting the surface from the front or the back
    // (based on normals, not on IORs) and whether we're inside the object
    // (if there is an interior at all).
    const float cos_theta = dot(view_direction, resolved_inputs.shading_basis.n);
    const bool back_facing = cos_theta < 0.0f;
    const bool inside = back_facing && !resolved_inputs.geometry_thin_walled;  // if thin-walled, we always hit from the outside

    // Calculate normals and TBN bases usable for anisotropic specular reflections.
    // The "_ff" suffix (for "forward-facing" or "face-forwarded") marks normals and bases
    // that have been conditionally flipped to always be in the same hemisphere as the view
    // direction. This ensures that dot products, hemisphere tests, and lobe initialization
    // work correctly regardless of whether the surface is hit from the front or the back,
    // without special-casing throughout the shading code.

    // Set up normal_ff, the forward-facing version of the shading normal.
    OpenPBR_Basis specular_anisotropy_basis_ff = resolved_inputs.shading_basis;
    openpbr_rotate_basis_around_normal(specular_anisotropy_basis_ff, resolved_inputs.specular_anisotropy_rotation_cos_sin);
    specular_anisotropy_basis_ff.t = normalize(specular_anisotropy_basis_ff.t);
    specular_anisotropy_basis_ff.b = normalize(specular_anisotropy_basis_ff.b);

    if (back_facing)
    {
        // If hitting the surface from the back, we invert the normal,
        // swap the IORs (treating the input IOR as an absolute IOR),
        // and shade as if we hit from the front.
        openpbr_invert_basis(specular_anisotropy_basis_ff);
    }
    const vec3 normal_ff = specular_anisotropy_basis_ff.n;

    // Make coat_normal_ff depend on coat_basis.n instead of shading_basis.n
    // and update the coat normal basis to be in the same hemisphere as the view direction.
    OpenPBR_Basis coat_anisotropy_basis_ff = resolved_inputs.coat_basis;
    openpbr_rotate_basis_around_normal(coat_anisotropy_basis_ff, resolved_inputs.coat_anisotropy_rotation_cos_sin);
    coat_anisotropy_basis_ff.t = normalize(coat_anisotropy_basis_ff.t);
    coat_anisotropy_basis_ff.b = normalize(coat_anisotropy_basis_ff.b);

    if (dot(view_direction, resolved_inputs.coat_basis.n) < 0.0f)  // back-facing coat
        openpbr_invert_basis(coat_anisotropy_basis_ff);
    const vec3 coat_normal_ff = coat_anisotropy_basis_ff.n;

    // Calculate IORs considering interactions between layers.
    // IOR variables are assumed to be absolute IORs unless explicitly marked as relative IORs.

    const float weighted_specular_ior = openpbr_apply_specular_weight_to_ior(resolved_inputs.specular_ior, resolved_inputs.specular_weight);
    float relative_ior_for_refraction;         // needs to be set
    float relative_ior_for_trans_reflection;   // needs to be set
    float relative_ior_for_opaque_reflection;  // needs to be set
    float weighted_coat_ior = exterior_ior;    // can be left at default value except when entering coat from outside
    float relative_coat_ior = 1.0f;            // can be left at default value except when entering coat from outside
    if (inside)
    {
        // If hitting the surface from the back, we swap the IORs
        // (treating the input IOR as an absolute IOR)
        // and shade as if we hit from the front.
        relative_ior_for_refraction = exterior_ior / weighted_specular_ior;
        relative_ior_for_trans_reflection = relative_ior_for_refraction;
        relative_ior_for_opaque_reflection = weighted_specular_ior / exterior_ior;  // reflect from opaque part as if hit from the outside
    }
    else  // entering
    {
        relative_ior_for_refraction = weighted_specular_ior / exterior_ior;

        // Unless we're hitting the surface from the outside,
        // ignore the coat (except the way it tints light passing through it).
        // TODO: Refine how coat is handled when hit from the inside.
        const bool enter_coat = resolved_inputs.coat_weight > 0.0f && !inside;
        if (enter_coat)
        {
            // Adjust the base IOR to be relative to the coat IOR.
            // To handle partial coat, linearly interpolate the coat IOR, which, for low coat
            // roughnesses, results in a nearly linear interpolation of the total energy reflected
            // from the coat, consistent with the behavior of the coating lobe with partial presence.
            // To avoid introducing nonphysical total internal reflection that would be caused by rays
            // hitting the base surface at implausible grazing angles due to the coat not changing the
            // ray direction, make sure the base IOR isn't flipped from external to internal reflection.
            // TODO: Figure out how to apply this or a similar heuristic to thin-film iridescence.
            weighted_coat_ior = mix(exterior_ior, resolved_inputs.coat_ior, resolved_inputs.coat_weight);
            if (weighted_coat_ior > weighted_specular_ior && weighted_specular_ior >= exterior_ior)
            {
                // Use the inverted relative IOR to avoid nonphysical total internal reflection.
                relative_ior_for_trans_reflection = weighted_coat_ior / weighted_specular_ior;
            }
            else
            {
                // Use the relative IOR (eta_t / eta_i) directly.
                relative_ior_for_trans_reflection = weighted_specular_ior / weighted_coat_ior;
            }
            // Do not use the weighted_coat_ior here since the weight will be applied in the coating lobe.
            relative_coat_ior = resolved_inputs.coat_ior / exterior_ior;
        }
        else
        {
            relative_ior_for_trans_reflection = relative_ior_for_refraction;
        }
        relative_ior_for_opaque_reflection = relative_ior_for_trans_reflection;
    }

    // To avoid numerical issues, clamp the IOR that's used for refraction.
    // This needs to be done before potentially swapping the IORs below.
    // TODO: Considering clamping the coat IOR also for consistency.
    OPENPBR_CONSTEXPR_LOCAL float MinRefractionIorAboveOne = 1.0001f;  // Empirically, this can be reduced to 1+epsilon without serious artifacts.
    if (relative_ior_for_refraction >= 1.0f)
    {
        relative_ior_for_refraction = max(relative_ior_for_refraction, MinRefractionIorAboveOne);
    }
    else
    {
        OPENPBR_CONSTEXPR_LOCAL float InverseMinRefractionIorAboveOne = 1.0f / MinRefractionIorAboveOne;
        relative_ior_for_refraction = min(relative_ior_for_refraction, InverseMinRefractionIorAboveOne);
    }

    // Calculate roughnesses (considering interactions between layers if applicable).

    // Adjust the coat roughness to account for the fuzz.
    // TODO: This is an empirical adjustment. Improve this.
    // TODO: Make the fuzz affect the tint of the underlying surface, too, and vice versa.
    OPENPBR_CONSTEXPR_LOCAL float FuzzEffectOnUnderlyingRoughness = 0.005f;  // (at max weight, color, and roughness)
    const float fuzz_roughness_adjustment_factor =
        openpbr_average(resolved_inputs.fuzz_color) * resolved_inputs.fuzz_roughness * FuzzEffectOnUnderlyingRoughness;  // TODO: Improve this.
    const float adjusted_coat_roughness =
        mix(resolved_inputs.coat_roughness,
            openpbr_fourth_root(min(1.0f,
                                    openpbr_fourth_power(resolved_inputs.coat_roughness) +
                                        fuzz_roughness_adjustment_factor * openpbr_fourth_power(resolved_inputs.fuzz_roughness))),
            resolved_inputs.fuzz_weight);

    // Adjust the base roughness to account for the coat.
    // The heuristic suggested in the OpenPBR spec is too strong as it doesn't take the coat IOR into account.
    // Light doesn't scatter as much from a low IOR surface even if the roughness is high, because the scattering
    // comes from refraction and refraction doesn't bend light as much when the IOR is low.
    // And if the coat IOR is 1, the coat shouldn't affect the roughnesses of the underlying layers at all.
    // Furthermore, even at high IOR, randomly oriented microfacets can only refract light into a half sphere
    // of directions while they can reflect light into a full sphere of directions.
    // So we need to take the coat IOR into account.
    //
    // Original version from OpenPBR spec:
    // adjusted_specular_roughness =
    //     mix(specular_roughness,
    //         openpbr_fourth_root(min(1.0f, openpbr_fourth_power(specular_roughness) + 2.0f * openpbr_fourth_power(adjusted_coat_roughness))),
    //         coat);
    //
    // Our current improved version:
    const float coat_roughness_adjustment_factor =
        mix(1.0f, 0.0f, relative_coat_ior >= 1.0f ? 1.0f / relative_coat_ior : relative_coat_ior);  // TODO: Improve this.
    const float adjusted_specular_roughness =
        mix(resolved_inputs.specular_roughness,
            openpbr_fourth_root(min(1.0f,
                                    openpbr_fourth_power(resolved_inputs.specular_roughness) +
                                        coat_roughness_adjustment_factor * openpbr_fourth_power(adjusted_coat_roughness))),
            resolved_inputs.coat_weight);

    OPENPBR_CONSTEXPR_LOCAL bool DeltaSpecularSupported = false;    // TODO: Support isotropic and delta specular lobes and remove this constant.
    OPENPBR_CONSTEXPR_LOCAL float MinMicrofacetRoughness = 0.001f;  // empirically, visible artifacts start appearing a little above 1e-7f
    OPENPBR_CONSTEXPR_LOCAL float MinMicrofacetAlpha = openpbr_square(MinMicrofacetRoughness);

    const float specular_alpha = openpbr_square(adjusted_specular_roughness);
    const vec2 anisotropic_specular_alpha =
        openpbr_compute_anisotropic_alpha(specular_alpha, resolved_inputs.specular_roughness_anisotropy, DeltaSpecularSupported, MinMicrofacetAlpha);

    const float coat_alpha = openpbr_square(adjusted_coat_roughness);
    const vec2 anisotropic_coat_alpha =
        openpbr_compute_anisotropic_alpha(coat_alpha, resolved_inputs.coat_roughness_anisotropy, DeltaSpecularSupported, MinMicrofacetAlpha);

    // Base layer.

    // We want opaque + metal + trans = 1 (barycentric coords).
    // Metal supersedes dielectric.
    // Trans supersedes SSS.
    // TODO: Rename these internal variables (which come from old Lantern code).

    const float non_trans = 1.0f - resolved_inputs.transmission_weight;
    const float non_sss = 1.0f - resolved_inputs.subsurface_weight;

    const float dielectric = 1.0f - resolved_inputs.base_metalness;
    const float opaque_dielectric = non_trans * non_sss * dielectric;

    const float trans = dielectric * resolved_inputs.transmission_weight;
    const float sss = non_trans * dielectric * resolved_inputs.subsurface_weight;

    const float darkened_metal = resolved_inputs.base_metalness * resolved_inputs.specular_weight;
    const vec3 weighted_base_color = resolved_inputs.base_color * resolved_inputs.base_weight;
    vec3 metal_average_fresnel = vec3(1.0f);
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
    {
        metal_average_fresnel = openpbr_metal_average_fresnel_with_f82_tint(weighted_base_color, resolved_inputs.specular_color);
    }

    // Adjust the color of everything beneath the coat to account for internal reflections from the inside of the coat
    // (and the subsequent extra absorption events that occur at the base surface).
    // Some of the terminology used here is from the OpenPBR spec, in particular:
    //     K_s = internal diffuse reflection coefficient for the specular part
    //     K_r = internal diffuse reflection coefficient for the diffuse part
    //     K = internal diffuse reflection coefficient
    //     E_b = albedo of the base surface
    //     Delta = darkening factor
    //     modulated_coat_darkening = darkening factor modulated by the coat presence and coat darkening parameter
    // The high-level structure of the formula is based on the OpenPBR spec, but the details are different --
    // for example we incorporate fuzz and other features into the albedo of the underlying surface.
    // TODO: Revisit, improve, and extend the physics here.
    // TODO: Consider making fuzz darken the underlying layers as well.

    // Calculate the reflectance from the inside of the coat for perfectly specular and perfectly diffuse underlying surfaces.

    // These formulas correspond to Equation 67 and Equation 66 from the OpenPRB spec (v1.1):
    const float K_s = openpbr_average_fresnel(relative_coat_ior);  // TODO: Use directional Fresnel (of exterior coat reflection).
    const float K_r = 1.0f - (1.0f - openpbr_average_fresnel(relative_coat_ior)) / openpbr_square(relative_coat_ior);

    // Calculate the effective roughness of everything beneath the coat.
    // TODO: Take colors and additional weights into account.
    const float specularity_opaque_dielectric = openpbr_average_fresnel(relative_ior_for_opaque_reflection);  // TODO: Use directional Fresnel.
    const float specularity_sss_trans = openpbr_average_fresnel(relative_ior_for_trans_reflection);           // TODO: Use directional Fresnel.
    const float specularity_dielectric = opaque_dielectric * specularity_opaque_dielectric + (sss + trans) * specularity_sss_trans;
    const float specularity_metal = resolved_inputs.base_metalness;
    const float specularity_base = specularity_dielectric + specularity_metal;
    const float effective_roughness_base = mix(1.0f, adjusted_specular_roughness, specularity_base);

    // This formula corresponds to Equation 68 from the OpenPRB spec (v1.1):
    const float K = mix(K_s, K_r, effective_roughness_base);

    // Estimate the base surface albedo.

    const vec3 base_albedo_from_metal = darkened_metal * metal_average_fresnel;
    const vec3 base_albedo_from_opaque_dielectric = opaque_dielectric * mix(weighted_base_color, vec3(1.0f), specularity_opaque_dielectric);
    const vec3 base_albedo_from_sss = sss * resolved_inputs.subsurface_color;  // considering all internal bounces
    const vec3 base_albedo_from_trans = vec3(trans);  // considering zero internal bounces (we don't know how dense the volume is)
    const vec3 E_b = base_albedo_from_metal + base_albedo_from_opaque_dielectric + base_albedo_from_sss + base_albedo_from_trans;

    // Calculate the final coat darkening factor.

    // These formulas correspond to Equation 65 and Equation 71 from the OpenPRB spec (v1.1):
    const vec3 Delta = vec3(1.0f - K) / (vec3(1.0f) - E_b * K);
    const vec3 modulated_coat_darkening = mix(vec3(1.0f), Delta, resolved_inputs.coat_weight * resolved_inputs.coat_darkening);

    // Create specular and diffuse reflection lobes.

    // Create and add a specular lobe.
    // Generally only a single base specular lobe is needed because we use the
    // combined reflection/transmission lobe with configurable reflection/transmission coefficients.
    // However, in the case of dispersion, three base specular lobes are required, one for each of
    // the color channels (which enables divergence between color channels, MIS between color channels,
    // and efficient sampling of the dominant color channel).

    // First, create a microfacet distribution.

    const OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr = OPENPBR_MAKE_STRUCT_3(
        OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution, anisotropic_specular_alpha, specular_anisotropy_basis_ff, specular_alpha);

    // Next, calculate some colors.

    const float trans_including_sss = trans + sss;

    vec3 scale_for_trans_reflection;
    vec3 thin_wall_aware_scale_for_trans_reflection;
    vec3 thin_wall_specular_transmission_albedo_scale;
    vec3 scale_for_opaque_reflection;
    if (inside)
    {
        // Note that specular tint does not affect dielectric internal reflection.
        scale_for_trans_reflection = vec3(trans_including_sss);
        thin_wall_aware_scale_for_trans_reflection = scale_for_trans_reflection;  // no difference (if inside, we must not be in thin-wall mode)
        thin_wall_specular_transmission_albedo_scale = vec3(0.0f);  // no thin-wall contribution (if inside, we must not be in thin-wall mode)
        scale_for_opaque_reflection = resolved_inputs.specular_color * opaque_dielectric;
    }
    else  // entering
    {
        scale_for_trans_reflection = resolved_inputs.specular_color * trans_including_sss;
        if (resolved_inputs.geometry_thin_walled)
        {
            // In thin-wall mode, we only consider the reflection from the "sss" component, not "trans_including_sss" ("trans" + "sss")
            // because the reflection from the "trans" component is handled separately based on a formula that incorporates the
            // reflection from the back surface and all internal bounces.
            thin_wall_aware_scale_for_trans_reflection =
                resolved_inputs.specular_color * sss;  // used for specular reflection from subsurface component
            thin_wall_specular_transmission_albedo_scale =
                resolved_inputs.specular_color * trans;  // used for specular reflection from transmission component
        }
        else
        {
            thin_wall_aware_scale_for_trans_reflection = scale_for_trans_reflection;  // no difference
            thin_wall_specular_transmission_albedo_scale = vec3(0.0f);                // no thin-wall contribution
        }
        scale_for_opaque_reflection = resolved_inputs.specular_color * opaque_dielectric;
    }

    // Apply dispersion to all of the relative IORs.
    // TODO: Apply dispersion to the original absolute IORs instead of to the derived relative IORs.

    float dispersion;  // will be set below if dispersion is enabled

    vec3 relative_ior_for_refraction_with_dispersion;
    vec3 relative_ior_for_trans_reflection_with_dispersion;
    vec3 relative_ior_for_opaque_reflection_with_dispersion;

    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion) && resolved_inputs.transmission_dispersion_scale > 0.0f &&
        resolved_inputs.base_metalness < 1.0f)  // dispersion check (subset of openpbr_needs_rgb_wavelengths)
    {
        dispersion = openpbr_safe_divide(
            20.0f,
            openpbr_safe_divide(resolved_inputs.transmission_dispersion_abbe_number, resolved_inputs.transmission_dispersion_scale, 0.0f),
            0.0f);

        for (int channel = 0; channel < OpenPBR_NumRgbChannels; ++channel)
        {
            // Adjust the IORs to account for dispersion.

            // Use stochastic wavelengths from payload.
            const float wavelength = rgb_wavelengths_nm[channel];

            // The conditions ensure that calculations are not duplicated in the common cases where the IORs match.
            relative_ior_for_refraction_with_dispersion[channel] =
                openpbr_dispersion_adjusted_ior(relative_ior_for_refraction, dispersion, wavelength);
            relative_ior_for_trans_reflection_with_dispersion[channel] =
                relative_ior_for_trans_reflection == relative_ior_for_refraction
                    ? relative_ior_for_refraction_with_dispersion[channel]
                    : openpbr_dispersion_adjusted_ior(relative_ior_for_trans_reflection, dispersion, wavelength);
            relative_ior_for_opaque_reflection_with_dispersion[channel] =
                relative_ior_for_opaque_reflection == relative_ior_for_trans_reflection
                    ? relative_ior_for_trans_reflection_with_dispersion[channel]
                    : openpbr_dispersion_adjusted_ior(relative_ior_for_opaque_reflection, dispersion, wavelength);
        }
    }
    else
    {
        dispersion = 0.0f;  // dispersion disabled

        relative_ior_for_refraction_with_dispersion = vec3(relative_ior_for_refraction);
        relative_ior_for_trans_reflection_with_dispersion = vec3(relative_ior_for_trans_reflection);
        relative_ior_for_opaque_reflection_with_dispersion = vec3(relative_ior_for_opaque_reflection);
    }

    // Now, do some necessary calculations and adjustments to weights and IORs.

    // Check whether thin-wall mode is enabled to drive subsequent weights.
    const float thin_wall_weight = resolved_inputs.geometry_thin_walled ? 1.0f : 0.0f;
    const float non_thin_wall_weight = 1.0f - thin_wall_weight;
    const float thin_wall_aware_cos_theta =
        resolved_inputs.geometry_thin_walled
            ? abs(cos_theta)
            : cos_theta;  // cos(theta) should always positive for thin-walled surfaces since we're always entering from the front

    const vec3 final_transmission_tint = openpbr_compute_final_transmission_tint(
        volume_derived_props.transmission_tint, resolved_inputs.geometry_thin_walled, thin_wall_aware_cos_theta, relative_ior_for_refraction);
    const vec3 tinted_trans = trans * final_transmission_tint;
    const vec3 tinted_trans_including_sss = tinted_trans + vec3(sss);
    const vec3 thin_wall_aware_tinted_trans_including_sss = non_thin_wall_weight * tinted_trans_including_sss;

    // Set up the surrounding absolute IORs needed for the thin film.
    // Note that thin film is only applied on both the exterior as well as the interior.
    // Note that combining thin film with coat or dispersion works but is not fully correct and could exhibit artifacts.
    // - Coat artifacts: Unphysical total internal reflection can occur when the coat IOR is higher than the base IOR due to
    //   the refraction of light by the coat not being taken into account.
    // - Dispersion artifacts: The front interface of the thin film is currently not affected by dispersion, which can lead to
    //   mismatches between the reflection and the refraction (which is affected by dispersion) when hitting the surface from
    // the inside (in which case the front surface corresponds to the base surface which has dispersion).
    // TODO: Properly handle the interaction between coat, dispersion, and thin film.
    float thin_film_exterior_ior;  // absolute IOR of medium above thin film - dispersion not supported
    vec3 thin_film_interior_ior;   // absolute IOR of medium below thin film - dispersion supported
    if (inside)
    {
        thin_film_exterior_ior = weighted_specular_ior;
        thin_film_interior_ior = vec3(weighted_coat_ior);
    }
    else  // entering
    {
        thin_film_exterior_ior = weighted_coat_ior;
        thin_film_interior_ior = vec3(weighted_specular_ior);

        // Apply dispersion to the absolute IOR below the thin film.
        // TODO: Consider also supporting dispersion for the absolute IOR above the thin film,
        //       which is actually the material's interior IOR when hitting the surface from the inside.
        if (dispersion > 0.0f)  // if dispersion was enabled above, apply it here too
        {
            for (int channel = 0; channel < OpenPBR_NumRgbChannels; ++channel)
            {
                thin_film_interior_ior[channel] =
                    openpbr_dispersion_adjusted_ior(thin_film_interior_ior[channel], dispersion, rgb_wavelengths_nm[channel]);
            }
        }
    }

    // Next, set up the thin-wall lobes if applicable.

    // Calculate values needed for thin-wall specular, which is implemented as specular reflection + unrefracted specular transmission.
    //
    // For the thin-wall specular, calculate the reflectance from both the front and back surfaces including all internal bounces
    // (i.e., window reflectance). These formulas are exact in the important case where the roughness is zero, which is useful
    // for rendering windows and other thin sheets of clear material. However, results will not be fully accurate for higher
    // roughness values or if certain other material features are used that violate the assumptions of the formulas.
    // TODO: Incorporate more factors and improve accuracy for a wider variety of configurations.
    //
    // We calculate both the overall reflectance and transmittance here, though we use them in different places:
    // we add the reflectance to the existing specular and diffuse lobes, and we create separate lobes for the transmission.

    vec3 thin_wall_specular_reflection_albedo = vec3(0.0f);
    vec3 thin_wall_specular_transmission_albedo = vec3(0.0f);
    if (resolved_inputs.geometry_thin_walled)
    {
        const float window_reflectance = openpbr_thin_wall_fresnel(relative_ior_for_trans_reflection, thin_wall_aware_cos_theta);
        const float window_transmittance = 1.0f - openpbr_thin_wall_fresnel(relative_ior_for_refraction, thin_wall_aware_cos_theta);

        thin_wall_specular_reflection_albedo = thin_wall_specular_transmission_albedo_scale * window_reflectance;
        thin_wall_specular_transmission_albedo = tinted_trans * window_transmittance;
    }

    // Set up thin-wall specular-transmission lobe.

    openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe.thin_wall_specular_trans_lobe,
                            normal_ff,
                            microfacet_distr,  // TODO: Incorporate IOR so blur reduces as IOR approaches 1.
                            thin_wall_specular_transmission_albedo);

    // Calculate values needed for thin-wall subsurface, which is implemented as diffuse reflection + diffuse transmission.

    vec3 thin_wall_subsurface_transmission_albedo = vec3(0.0f);
    vec3 thin_wall_subsurface_reflection_albedo = vec3(0.0f);
    if (resolved_inputs.geometry_thin_walled)
    {
        const float thin_wall_subsurface_transmission_proportion = (volume_derived_props.thin_wall_subsurface_anisotropy + 1.0f) * 0.5f;
        const float thin_wall_subsurface_reflection_proportion = 1.0f - thin_wall_subsurface_transmission_proportion;
        const vec3 thin_wall_subsurface_albedo = sss * volume_derived_props.thin_wall_subsurface_color;

        thin_wall_subsurface_transmission_albedo = thin_wall_subsurface_albedo * thin_wall_subsurface_transmission_proportion;
        thin_wall_subsurface_reflection_albedo = thin_wall_subsurface_albedo * thin_wall_subsurface_reflection_proportion;
    }

    // Set up thin-wall diffuse-transmission lobe.

    openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe.thin_wall_diffuse_trans_lobe,
                            normal_ff,
                            view_direction,
                            thin_wall_subsurface_transmission_albedo,
                            resolved_inputs.base_diffuse_roughness,
                            specular_alpha,
                            relative_ior_for_opaque_reflection);

    // Done with thin-wall-specific preparations; specular and diffuse reflection components will be incorporated below.

    // Next, create a reflection/transmission coefficient.

    const float thin_film_thickness_nm = resolved_inputs.thin_film_thickness * 1000.0f;

    const OpenPBR_ComprehensiveReflectionTransmissionCoefficient refl_trans_coeff =
        OPENPBR_MAKE_STRUCT_15(OpenPBR_ComprehensiveReflectionTransmissionCoefficient,
                               relative_ior_for_trans_reflection_with_dispersion,
                               relative_ior_for_opaque_reflection_with_dispersion,
                               thin_wall_aware_scale_for_trans_reflection,
                               scale_for_opaque_reflection,
                               thin_wall_aware_tinted_trans_including_sss,
                               weighted_base_color,
                               resolved_inputs.specular_color,
                               darkened_metal,
                               resolved_inputs.thin_film_weight,
                               thin_film_thickness_nm,
                               thin_film_exterior_ior,
                               resolved_inputs.thin_film_ior,
                               thin_film_interior_ior,
                               rgb_wavelengths_nm,
                               thin_wall_specular_reflection_albedo);

    // Finally, create the actual specular lobe.

    openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe.specular_lobe,
                            normal_ff,
                            microfacet_distr,
                            refl_trans_coeff,
                            relative_ior_for_refraction_with_dispersion,
                            path_throughput);

    // Add specular microfacet multiple scattering lobe(s) for the base dielectric and metal components.
    // Use the original alpha (roughness^2) value for the specular microfacet-multiple-scattering lobes and the diffuse lobe,
    // since these lobes depend on alpha to conserve energy but don't directly support anisotropy.
    // TODO: Avoid initializing and using these lobes if they don't contribute.
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency))
    {
        openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe.dielectric_mms_lobe,
                                normal_ff,
                                view_direction,
                                specular_alpha,
                                relative_ior_for_refraction,
                                scale_for_trans_reflection,
                                tinted_trans_including_sss);
    }
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableMetallic))
    {
        // Note that color is squared to account for the minimum of one additional absorption event (at the
        // secondary microfacet intersection) (although even more than one additional bounce may occur in reality).
        const vec3 scale_for_metal_reflection = openpbr_square(metal_average_fresnel) * darkened_metal;
        openpbr_initialize_lobe(
            fuzz_lobe.coating_lobe.base_lobe.metal_mms_lobe, normal_ff, view_direction, specular_alpha, scale_for_metal_reflection);
    }

    // Create and add a diffuse lobe.
    const vec3 diffuse_albedo = weighted_base_color * opaque_dielectric;
    const vec3 thin_wall_aware_diffuse_albedo = diffuse_albedo + thin_wall_subsurface_reflection_albedo;
    // TODO: Avoid initializing and using this lobe if it doesn't contribute.
    openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe.diffuse_lobe,
                            normal_ff,
                            view_direction,
                            thin_wall_aware_diffuse_albedo,
                            resolved_inputs.base_diffuse_roughness,
                            specular_alpha,
                            relative_ior_for_opaque_reflection);

    // Finish setting up the aggregate lobe based on its constituent lobes.
    openpbr_initialize_lobe(fuzz_lobe.coating_lobe.base_lobe, view_direction, path_throughput);

    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat))
    {
        // Add a coating layer if applicable.
        // The coating is assumed to be on the outside of the object; rays passing from the inside to
        // the outside of the object are tinted by the coat, but do not otherwise interact with the coat.
        const OpenPBR_IorReflectionCoefficient coat_refl_trans_coeff = OPENPBR_MAKE_STRUCT_1(OpenPBR_IorReflectionCoefficient, relative_coat_ior);
        const OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution coat_microfacet_distr = OPENPBR_MAKE_STRUCT_3(
            OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution, anisotropic_coat_alpha, coat_anisotropy_basis_ff, coat_alpha);
        openpbr_initialize_lobe(fuzz_lobe.coating_lobe.coat_reflection_lobe, coat_normal_ff, coat_microfacet_distr, coat_refl_trans_coeff);
        openpbr_initialize_lobe(fuzz_lobe.coating_lobe,
                                coat_normal_ff,
                                view_direction,
                                resolved_inputs.coat_color,
                                resolved_inputs.coat_weight,
                                modulated_coat_darkening,
                                inside);

        // Add a fuzz layer if present.
        // Fuzz is considered to be on the outside of the object and is
        // not considered at all when hitting the object from the inside.
        // TODO: Avoid initializing and using this lobe if it doesn't contribute.

        // The fuzz normal is a blend of the base normal and the coat normal based on the presence of the coat.
        const vec3 unnormalized_fuzz_normal_ff = mix(normal_ff, coat_normal_ff, resolved_inputs.coat_weight);
        const vec3 fuzz_normal_ff = openpbr_fast_normalize(unnormalized_fuzz_normal_ff);
        // Verify that the normalization succeeded.
        // TODO: The normalization could completely fail if the base and coat normals are pointing in nearly opposite directions.
        //       The normalization could also slightly fail due to inaccuracies in the openpbr_fast_normalize function.
        //       Even if the normalization succeeds, the result could potentially end up in the wrong hemisphere
        //       if both face-forwarded input normals are nearly coplanar.
        //       If any of these cases occurs in practice, we should use the regular normalize function and/or fall back
        //       to the base normal; for now, we just assert.
        OPENPBR_ASSERT(openpbr_is_normalized(fuzz_normal_ff) && dot(view_direction, fuzz_normal_ff) >= 0.0f,
                       "Fuzz normal must be a face-forwarded unit vector");

        openpbr_initialize_lobe(fuzz_lobe,
                                fuzz_normal_ff,
                                view_direction,
                                resolved_inputs.fuzz_roughness,
                                resolved_inputs.fuzz_color,
                                inside ? 0.0f : resolved_inputs.fuzz_weight);
    }
}

// Main initialization entrypoint: prepares both volume and BSDF lobes from resolved inputs.
// For specialized code paths that only need the volume (e.g., translucent shadow rays for caustics replacement),
// call openpbr_prepare_volume and openpbr_prepare_lobes separately to avoid unnecessary lobe setup.
OpenPBR_PreparedBsdf openpbr_prepare_bsdf_and_volume_impl(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                                                          const vec3 path_throughput,
                                                          const vec3 rgb_wavelengths_nm,
                                                          const float exterior_ior,
                                                          const vec3 view_direction)
{
    OpenPBR_PreparedBsdf prepared;

    // Cache the view direction for use in eval/sample/pdf.
    prepared.view_direction = view_direction;

    OpenPBR_VolumeDerivedProps volume_derived_props;

    OPENPBR_CONSTEXPR_LOCAL bool VolumesEnabled = true;

    // Step 1: Set up the volume and any derived properties from resolved inputs.
    openpbr_prepare_volume(resolved_inputs, volume_derived_props, prepared.volume, VolumesEnabled);

    // Step 2: Set up BSDF lobes using resolved inputs and the volume-derived properties.
    openpbr_prepare_lobes(
        resolved_inputs, volume_derived_props, prepared.fuzz_lobe, path_throughput, rgb_wavelengths_nm, exterior_ior, view_direction);

    return prepared;
}

// Importance-samples a light direction and returns weight, PDF, and sampled lobe type.
//
// The returned weight includes the cosine term; specifically it is the BSDF value
// multiplied by cos(theta) and divided by the PDF (with respect to solid angle).
// It takes all lobes and all sampling techniques into account (effectively doing MIS internally).
//
// IMPORTANT: Check the pdf output to determine if sampling succeeded:
//   - pdf > 0: Valid sample generated; light_direction, weight, and sampled_type are set
//   - pdf == 0: No valid sample; output parameters are undefined
//   - pdf is never negative
void openpbr_sample_impl(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_PreparedBsdf) prepared,
                         const vec3 rand,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const bool valid_sample =
        OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat)
            ? openpbr_sample_lobe(prepared.fuzz_lobe, rand, prepared.view_direction, light_direction, weight, pdf, sampled_type)
            : openpbr_sample_lobe(
                  prepared.fuzz_lobe.coating_lobe.base_lobe, rand, prepared.view_direction, light_direction, weight, pdf, sampled_type);

    OPENPBR_ASSERT((valid_sample && pdf > 0.0f) || !valid_sample, "Lobe sampling must provide a positive PDF when it returns true");

    if (!valid_sample)
    {
        // The caller uses the pdf to determine whether the sample is valid,
        // but internal lobes use a bool to specify this explicitly.
        pdf = 0.0f;
    }
}

// BSDF values returned include cosine term.
OpenPBR_DiffuseSpecular openpbr_eval_impl(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_PreparedBsdf) prepared, const vec3 light_direction)
{
    return OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat)
               ? openpbr_calculate_lobe_value(prepared.fuzz_lobe, prepared.view_direction, light_direction)
               : openpbr_calculate_lobe_value(prepared.fuzz_lobe.coating_lobe.base_lobe, prepared.view_direction, light_direction);
}

float openpbr_pdf_impl(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_PreparedBsdf) prepared, const vec3 light_direction)
{
    return OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat)
               ? openpbr_calculate_lobe_pdf(prepared.fuzz_lobe, prepared.view_direction, light_direction)
               : openpbr_calculate_lobe_pdf(prepared.fuzz_lobe.coating_lobe.base_lobe, prepared.view_direction, light_direction);
}

// Computes the emission that exits the surface after passing through the coat and fuzz layers.
//
// Reads emission_luminance and emission_color from resolved_inputs and scales by the
// transmittance of the coat and fuzz layers as seen from the view direction used during
// openpbr_prepare_lobes(). Both fields must be populated in resolved_inputs before calling
// (openpbr_make_default_resolved_inputs() sets emission_luminance = 0 so default inputs
// produce zero emission). openpbr_prepare_lobes() or openpbr_prepare_bsdf_and_volume()
// must also have been called first so that the coat and fuzz lobes in "prepared" are ready.
//
// Returns emission in nits (cd/m²) per the OpenPBR specification. Coat attenuation is
// wavelength-dependent (vec3); fuzz attenuation is achromatic (scalar). Respects the
// EnableSheenAndCoat specialization constant.
//
// Renderers that do not work in physical units must scale the result before accumulating.
// E.g., a renderer where 1.0 = 1000 nits should multiply the result by 0.001.
//
// Note: emission is a first-class output of the OpenPBR model and must be obtained by
// calling this function — reading emission_luminance * emission_color directly from
// resolved_inputs bypasses the coat and fuzz attenuation. Unlike translucent shadows
// (an integrator-level approximation), this is a candidate for promotion to openpbr_api.h;
// see README.md for the open design decisions.
vec3 openpbr_compute_emission(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                              OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_PreparedBsdf) prepared)
{
    vec3 result = resolved_inputs.emission_luminance * resolved_inputs.emission_color;
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat))
    {
        // Coat absorption is wavelength-dependent; scale by the vec3 coat transmittance.
        if (resolved_inputs.coat_weight > 0.0f)
            result *= openpbr_base_layer_scale_incoming(prepared.fuzz_lobe.coating_lobe);

        // Fuzz transmittance is achromatic; scale by the scalar fuzz transmittance.
        if (resolved_inputs.fuzz_weight > 0.0f)
            result *= vec3(openpbr_base_layer_scale_incoming(prepared.fuzz_lobe));
    }
    return result;
}

// Computes the probability and color weight for a biased straight-through shadow ray.
//
// Tracing unbiased refractive shadow paths through transmissive or subsurface-scattering
// materials is expensive and produces difficult-to-sample caustics. This function enables
// a practical approximation: a shadow ray passes straight through the surface with a given
// probability, weighted by Fresnel reflectance, coat tinting, and instantaneous transmission
// absorption. The unbiased complement is handled by the full path-tracing integrator.
//
// The hit surface's own roughness is already factored in: rough transmissive surfaces
// return a lower probability (approaching zero at roughness 1), at which point direct
// light sampling by the path tracer is more effective than the approximation. The
// probability can additionally be scaled by a weight derived from the roughness of the
// originating scattering event to further suppress the approximation when the path tracer
// can find the caustic efficiently (e.g., near-specular origin) and lean on it more when
// it cannot (e.g., diffuse or rough-specular origin).
//
// Returns vec4(weight.xyz, probability):
//   weight.xyz   - color transmittance to apply to the shadow ray throughput
//   probability  - material's straight-through probability (before roughness scaling);
//                  0 for fully opaque materials (early-out; weight is undefined)
//
// cos_theta = dot(surface_normal, shadow_ray_direction); positive = front side.
//
// Preconditions:
//   - openpbr_prepare_volume() must have been called to populate volume_derived_props.
//   - resolved_inputs must be fully populated.
vec4 openpbr_translucent_shadow_weight_and_prob(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ResolvedInputs) resolved_inputs,
                                                OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_VolumeDerivedProps) volume_derived_props,
                                                const float cos_theta)
{
    // TODO: Incorporate anisotropy into roughness as is done for microfacet multiple scattering.
    // TODO: Properly calculate the window transmission for thin-wall surfaces.

    // Calculate the proportion of the scattering that can be considered smooth refraction.
    // This will be used to set the probability of passing straight through the surface.
    // That light that is transmitted will be further weighted by various Fresnel and absorption terms.
    // The untransmitted proportion (including opaque components, thin-film subsurface, and rough refraction)
    // will be properly handled in an unbiased way.

    // Transmission supersedes subsurface in OpenPBR.
    // Metal supersedes transmission in OpenPBR.
    // Subsurface is not considered translucent in thin-wall mode (because it turns into very diffuse transmission).
    // Trans and SSS are considered translucent only to the degree that they involve smooth refraction
    // (because the whole point of translucent shadows is to avoid difficult refractive caustics).
    const float subsurface_contribution_to_translucency = resolved_inputs.geometry_thin_walled ? 0.0f : resolved_inputs.subsurface_weight;
    const float unweighted_translucency = mix(subsurface_contribution_to_translucency, 1.0f, resolved_inputs.transmission_weight);
    const float metal_weighted_translucency = unweighted_translucency * (1.0f - resolved_inputs.base_metalness);
    const float final_translucency = metal_weighted_translucency * (1.0f - resolved_inputs.specular_roughness);

    const float prob = final_translucency;
    if (prob == 0.0f)
        return vec4(0.0f);

    // Apply a Fresnel factor when entering an object to account for Fresnel reflectance
    // from both the front and the back surfaces.
    // Only apply this factor on the way in to avoid darkening subsurface scattering
    // and rough dielectrics, which conserve energy nicely when not darkened.
    // The downside of this system is that volumetrically scattered rays can exit the surface
    // even undarkened even at grazing angles, which could produce slightly unrealistic results.

    // cos_theta is calculated using the view direction.
    // In thin-wall mode, both surfaces have to be passed through in either direction,
    // so we make cos_theta positive so the double Fresnel is always applied.
    const float thin_wall_aware_cos_theta = resolved_inputs.geometry_thin_walled ? abs(cos_theta) : cos_theta;

    // Always use the external IOR.
    // If light passes through a flat pane of glass, the reflectance at the back surface
    // is the same as the reflectance at the front surface because of the way the light bends.
    // TODO: Update to take exterior IOR into account.
    // TODO: Consider whether we want to avoid eliminating intentional TIR on the outside of the object
    //       (even though that would fully darken the transmission) (including in thin-wall mode).
    const float weighted_specular_ior = openpbr_apply_specular_weight_to_ior(resolved_inputs.specular_ior, resolved_inputs.specular_weight);
    const float external_ior = weighted_specular_ior >= 1.0f ? weighted_specular_ior : 1.0f / weighted_specular_ior;

    // Calculate the (double or none) Fresnel factor itself.
    float fresnel_factor;
    if (thin_wall_aware_cos_theta > 0.0f)
    {
        // Calculate Fresnel reflectance.
        // Because dielectric refraction conserves energy in OpenPBR, this reflectance is the
        // only thing that darkens transmission apart from coat and volume absorption.
        // If it becomes necessary to make the window case more accurate, consider
        // accounting for multiple internal reflections.
        const float external_cos_theta = abs(thin_wall_aware_cos_theta);
        const float fresnel_reflectivity_smooth = openpbr_fresnel(external_ior, external_cos_theta);
        const float fresnel_reflectivity_rough = openpbr_average_fresnel(external_ior);
        const float fresnel_reflectivity = mix(fresnel_reflectivity_smooth, fresnel_reflectivity_rough, resolved_inputs.specular_roughness);
        const float fresnel_transmission_one_surface = 1.0f - fresnel_reflectivity;
        fresnel_factor = openpbr_square(fresnel_transmission_one_surface);  // two surfaces
    }
    else
    {
        fresnel_factor = 1.0f;
    }

    // metal_weighted_translucency and specular_roughness are already accounted for in prob.
    vec3 final_transmission = vec3(fresnel_factor);

    // Incorporate coat tint into transmission if applicable.
    // TODO: Should we apply this (and the transmission tint) twice in thin-wall mode?
    if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableSheenAndCoat))
    {
        if (resolved_inputs.coat_weight > 0.0f && openpbr_min3(resolved_inputs.coat_color) < 1.0f)
        {
            // Incorporate the coat weight into the coat tint.
            const vec3 fractional_coat_color = (resolved_inputs.coat_weight < 1.0f)
                                                   ? mix(vec3(1.0f), resolved_inputs.coat_color, resolved_inputs.coat_weight)
                                                   : resolved_inputs.coat_color;

            // The square root of the coat tint is applied upon each passage through the coat,
            // consistent with the logic in the coating lobes (albeit simplified).
            const vec3 sqrt_fractional_coat_color = sqrt(fractional_coat_color);
            final_transmission *= sqrt_fractional_coat_color;
        }
    }

    // Incorporate transmission tint (instantaneous absorption) into the trans component if applicable.
    const vec3 final_transmission_tint = openpbr_compute_final_transmission_tint(
        volume_derived_props.transmission_tint, resolved_inputs.geometry_thin_walled, thin_wall_aware_cos_theta, external_ior);
    final_transmission *= mix(vec3(1.0f), final_transmission_tint, resolved_inputs.transmission_weight / unweighted_translucency);

    // Pack the vec3 transmission and float probability into a vec4 (legacy behavior).
    return vec4(final_transmission, prob);
}

#endif  // OPENPBR_BSDF_H
