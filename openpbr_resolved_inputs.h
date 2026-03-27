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

#ifndef OPENPBR_RESOLVED_INPUTS_H
#define OPENPBR_RESOLVED_INPUTS_H

#include "impl/openpbr_math.h"
#include "openpbr_basis.h"

// Fully-resolved, texture-free OpenPBR inputs matching the official
// OpenPBR parameter set with a few modifications (noted in comments).
// Subsystems may only fill/read the subset they need.
struct OpenPBR_ResolvedInputs
{
    // Base layer
    float base_weight;
    vec3 base_color;
    float base_diffuse_roughness;
    float base_metalness;

    // Subsurface
    float subsurface_weight;
    vec3 subsurface_color;
    float subsurface_radius;
    vec3 subsurface_radius_scale;
    float subsurface_scatter_anisotropy;

    // Specular
    float specular_weight;
    vec3 specular_color;
    float specular_roughness;
    float specular_roughness_anisotropy;
    float specular_ior;
    // Optional extension (Eclair): cosine and sine of the anisotropy rotation angle; set to (1, 0) if unused.
    vec2 specular_anisotropy_rotation_cos_sin;

    // Coat
    float coat_weight;
    vec3 coat_color;
    float coat_roughness;
    float coat_roughness_anisotropy;
    float coat_ior;
    float coat_darkening;
    // Optional extension (Eclair): cosine and sine of the anisotropy rotation angle; set to (1, 0) if unused.
    vec2 coat_anisotropy_rotation_cos_sin;

    // Fuzz
    float fuzz_weight;
    vec3 fuzz_color;
    float fuzz_roughness;

    // Transmission / volume
    float transmission_weight;
    vec3 transmission_color;
    float transmission_depth;
    vec3 transmission_scatter;
    float transmission_scatter_anisotropy;
    float transmission_dispersion_scale;
    float transmission_dispersion_abbe_number;

    // Thin-film
    float thin_film_weight;
    float thin_film_thickness;
    float thin_film_ior;

    // Emission
    float emission_luminance;
    vec3 emission_color;

    // Geometry
    float geometry_opacity;
    bool geometry_thin_walled;

    // Precomputed bases (must be provided). Populate from evaluated geometry_normal/geometry_tangent
    // (and derived bitangent) for the base, and geometry_coat_normal/geometry_coat_tangent
    // (and derived bitangent) for the coat. Caller must compute bitangents and handedness correctly.
    OpenPBR_Basis shading_basis;
    OpenPBR_Basis coat_basis;
};

// Returns a fully initialized set of resolved inputs using the OpenPBR specification defaults.
OPENPBR_INLINE_FUNCTION OpenPBR_ResolvedInputs openpbr_make_default_resolved_inputs()
{
    OpenPBR_ResolvedInputs inputs;

    // Base layer
    inputs.base_weight = 1.0f;
    inputs.base_color = vec3(0.8f);
    inputs.base_diffuse_roughness = 0.0f;
    inputs.base_metalness = 0.0f;

    // Subsurface
    inputs.subsurface_weight = 0.0f;
    inputs.subsurface_color = vec3(0.8f);
    inputs.subsurface_radius = 1.0f;
    inputs.subsurface_radius_scale = vec3(1.0f, 0.5f, 0.25f);
    inputs.subsurface_scatter_anisotropy = 0.0f;

    // Specular
    inputs.specular_weight = 1.0f;
    inputs.specular_color = vec3(1.0f);
    inputs.specular_roughness = 0.3f;
    inputs.specular_roughness_anisotropy = 0.0f;
    inputs.specular_ior = 1.5f;
    inputs.specular_anisotropy_rotation_cos_sin = vec2(1.0f, 0.0f);

    // Coat
    inputs.coat_weight = 0.0f;
    inputs.coat_color = vec3(1.0f);
    inputs.coat_roughness = 0.0f;
    inputs.coat_roughness_anisotropy = 0.0f;
    inputs.coat_ior = 1.6f;
    inputs.coat_darkening = 1.0f;
    inputs.coat_anisotropy_rotation_cos_sin = vec2(1.0f, 0.0f);

    // Fuzz
    inputs.fuzz_weight = 0.0f;
    inputs.fuzz_color = vec3(1.0f);
    inputs.fuzz_roughness = 0.5f;

    // Transmission / volume
    inputs.transmission_weight = 0.0f;
    inputs.transmission_color = vec3(1.0f);
    inputs.transmission_depth = 0.0f;
    inputs.transmission_scatter = vec3(0.0f);
    inputs.transmission_scatter_anisotropy = 0.0f;
    inputs.transmission_dispersion_scale = 0.0f;
    inputs.transmission_dispersion_abbe_number = 20.0f;

    // Thin-film
    inputs.thin_film_weight = 0.0f;
    inputs.thin_film_thickness = 0.5f;
    inputs.thin_film_ior = 1.4f;

    // Emission
    inputs.emission_luminance = 0.0f;
    inputs.emission_color = vec3(1.0f);

    // Geometry
    inputs.geometry_opacity = 1.0f;
    inputs.geometry_thin_walled = false;

    // Bases
    inputs.shading_basis = OPENPBR_MAKE_STRUCT_3(OpenPBR_Basis, vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
    inputs.coat_basis = inputs.shading_basis;

    return inputs;
}

#endif  // OPENPBR_RESOLVED_INPUTS_H
