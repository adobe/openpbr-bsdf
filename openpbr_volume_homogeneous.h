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

#ifndef OPENPBR_VOLUME_HOMOGENEOUS_H
#define OPENPBR_VOLUME_HOMOGENEOUS_H

#include "impl/openpbr_sampling.h"
#include "openpbr_basis.h"

// Use a hard-coded magic constant to represent the extinction coefficient of the ambient volume.
// The ambient volume is whatever volume exists in the space outside the objects --
// currently the ambient volume is an empty volume overlayed with heterogeneous volumes.
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_AmbientVolumeExtinctionCoefficient = -3.402823466e+38f;
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_AmbientVolumeAlbedo =
    0.0f;  // can be arbitrary because unused; zero can be compressed without special handling
OPENPBR_CONSTEXPR_GLOBAL float OpenPBR_AmbientVolumeAnisotropy =
    0.0f;  // can be arbitrary because unused; zero can be compressed without special handling

// Struct storing the properties of a homogeneous volume.
// Extinction and albedo are used instead of absorption and scattering
// to reduce the number of calculations necessary in certain functions.
struct OpenPBR_HomogeneousVolume
{
    vec3 extinction_coefficient;  // extinction coefficient
    vec3 albedo;                  // single-scattering albedo
    float anisotropy;             // mean cosine of the scattering angle
};

// Initializes a volume from extinction, albedo, and anisotropy.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume
openpbr_make_volume_from_extinction_coefficient_and_albedo_and_anisotropy(vec3 extinction_coefficient, vec3 albedo, float anisotropy)
{
    OpenPBR_HomogeneousVolume volume;
    volume.extinction_coefficient = extinction_coefficient;
    volume.albedo = albedo;
    volume.anisotropy = anisotropy;
    return volume;
}

// Initializes a volume from extinction and albedo.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_volume_from_extinction_coefficient_and_albedo(vec3 extinction_coefficient, vec3 albedo)
{
    return openpbr_make_volume_from_extinction_coefficient_and_albedo_and_anisotropy(extinction_coefficient, albedo, 0.0f);
}

// Initializes an empty volume (vacuum).
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_empty_volume()
{
    return openpbr_make_volume_from_extinction_coefficient_and_albedo_and_anisotropy(vec3(0.0f), vec3(0.0f), 0.0f);
}

// Initializes a default ambient volume.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_ambient_volume()
{
    return openpbr_make_volume_from_extinction_coefficient_and_albedo_and_anisotropy(
        vec3(OpenPBR_AmbientVolumeExtinctionCoefficient), vec3(OpenPBR_AmbientVolumeAlbedo), OpenPBR_AmbientVolumeAnisotropy);
}

// Initializes a volume from extinction, creating an ambient volume if necessary.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_volume_from_extinction_coefficient(vec3 extinction_coefficient)
{
    if (all(equal(extinction_coefficient, vec3(OpenPBR_AmbientVolumeExtinctionCoefficient))))
    {
        return openpbr_make_ambient_volume();
    }
    else
    {
        return openpbr_make_volume_from_extinction_coefficient_and_albedo(extinction_coefficient, vec3(0.0f));
    }
}

// Initializes a volume from an absorption coefficient and a scattering coefficient.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume
openpbr_make_volume_from_absorption_and_scattering_coefficients_and_anisotropy(vec3 absorption_coefficient,
                                                                               vec3 scattering_coefficient,
                                                                               float anisotropy)
{
    OpenPBR_HomogeneousVolume volume;
    volume.extinction_coefficient = absorption_coefficient + scattering_coefficient;
    if (all(greaterThan(volume.extinction_coefficient, vec3(0.0f))))
    {
        volume.albedo = scattering_coefficient / volume.extinction_coefficient;
    }
    else
    {
        for (int color_channel = 0; color_channel < 3; ++color_channel)
        {
            volume.albedo[color_channel] = volume.extinction_coefficient[color_channel] > 0.0f
                                               ? scattering_coefficient[color_channel] / volume.extinction_coefficient[color_channel]
                                               : 0.0f;
        }
    }
    volume.anisotropy = anisotropy;
    return volume;
}

// Initializes a volume from an absorption coefficient and a scattering coefficient.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_volume_from_absorption_and_scattering_coefficients(vec3 absorption_coefficient,
                                                                                                                  vec3 scattering_coefficient)
{
    return openpbr_make_volume_from_absorption_and_scattering_coefficients_and_anisotropy(absorption_coefficient, scattering_coefficient, 0.0f);
}

// Initializes a purely absorbing volume from an absorption coefficient alone.
OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_make_volume_from_absorption_coefficient(vec3 absorption_coefficient)
{
    return openpbr_make_volume_from_extinction_coefficient_and_albedo(absorption_coefficient, vec3(0.0f));
}

// Checks if a volume is empty (vacuum).
OPENPBR_INLINE_FUNCTION bool openpbr_is_empty_volume(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume)
{
    return all(equal(volume.extinction_coefficient, vec3(0.0f)));
}

// Checks if a volume is an ambient volume.
OPENPBR_INLINE_FUNCTION bool openpbr_is_ambient_volume(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume)
{
    return all(equal(volume.extinction_coefficient, vec3(OpenPBR_AmbientVolumeExtinctionCoefficient))) &&
           all(equal(volume.albedo, vec3(OpenPBR_AmbientVolumeAlbedo))) && volume.anisotropy == OpenPBR_AmbientVolumeAnisotropy;
}

// Special version of function above with alternative interface
// for use when only the extinction coefficient is known.
OPENPBR_INLINE_FUNCTION bool openpbr_is_ambient_volume(const vec3 extinction_coefficient)
{
    return all(equal(extinction_coefficient, vec3(OpenPBR_AmbientVolumeExtinctionCoefficient)));
}

// Checks if a volume is a non-empty non-ambient volume.
OPENPBR_INLINE_FUNCTION bool openpbr_is_homogeneous_volume(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume)
{
    return (!openpbr_is_empty_volume(volume)) && (!openpbr_is_ambient_volume(volume));
}

// Calculates absorption coefficient of volume based on extinction and albedo.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_absorption_coefficient(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                          volume)
{
    return volume.extinction_coefficient * (1.0f - volume.albedo);
}

// Calculates scattering coefficient of volume based on extinction and albedo.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_scattering_coefficient(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                          volume)
{
    return volume.extinction_coefficient * volume.albedo;
}

// Calculates negative optical depth for volume.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_negative_optical_depth(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                          volume,
                                                                      const float distance)
{
    return volume.extinction_coefficient * -distance;
}

// Calculates volume transmittance at the given finite distance.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_transmittance_at_distance(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                             volume,
                                                                         const float distance)
{
    OPENPBR_ASSERT(openpbr_is_homogeneous_volume(volume), "This function should only be called for valid homogeneous volumes");

    return exp(openpbr_calculate_negative_optical_depth(volume, distance));
}

// Special version of function above with alternative interface
// for use when only the extinction coefficient is known.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_transmittance_at_distance(const vec3 extinction_coefficient, const float distance)
{
    const OpenPBR_HomogeneousVolume volume = openpbr_make_volume_from_extinction_coefficient(extinction_coefficient);

    // Special case to support ambient volumes and speed up transmittance calculation for empty volumes.
    if (!openpbr_is_homogeneous_volume(volume))
    {
        return vec3(1.0f);
    }

    return openpbr_calculate_transmittance_at_distance(volume, distance);
}

// Calculates the transmittance at an infinite distance.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_transmittance_at_infinity(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                             volume)
{
    return vec3(equal(volume.extinction_coefficient, vec3(0.0f)));
}

// Calculates probabilities of sampling a distance based on each color channel.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_color_channel_probability(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                             volume,
                                                                         const vec3 throughput)
{
    const vec3 total_contribution = throughput * volume.albedo;
    const float total_contribution_sum = openpbr_sum(total_contribution);
    // We check if total_contribution_sum >= FloatMin instead of > 0.0f because
    // 1.0f / total_contribution_sum could overflow if total_contribution_sum is subnormal.
    // In that case Infs and NaNs would result and cause invalid array indexing downstream.
    if (total_contribution_sum >= OpenPBR_FloatMin)
    {
        return total_contribution * (1.0f / total_contribution_sum);
    }
    else
    {
        return vec3(1.0f / 3.0f);
    }
}

// This function optimally samples a single extinction or transmission event based on the expected
// contribution of each event type, assuming no knowledge of the lighting or geometric configuration
// or the next surface intersection distance. Specifically, this function samples an interaction
// distance from a PDF that combines the PDFs of the individual color channels based on their expected
// contributions. The result is that the weights and the average path throughput remain stable even
// when the volume has highly chromatic extinction and high albedo.
//
// This function sets the distance argument if light interacted with the volume. If there is no
// volume, the distance argument is left as is.
OPENPBR_INLINE_FUNCTION void openpbr_sample_event_distance(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume,
                                                           const vec3 throughput,
                                                           const float rand,
                                                           OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(float) distance)
{
    // We could return for empty volumes, but not calling this function for empty volumes saves a random number.
    OPENPBR_ASSERT(openpbr_is_homogeneous_volume(volume), "This function should only be called for valid homogeneous volumes");

    // Make sure the random number is in [0, 1).
    OPENPBR_ASSERT(rand < 1.0f, "Random number should be less than 1");

    // Select a color channel.
    const vec3 color_channel_probability = openpbr_calculate_color_channel_probability(volume, throughput);
    const float rand_color_channel =
        rand * openpbr_sum(color_channel_probability);  // avoid numerical issues (probability sum equals one mathematically but not numerically)
    float cdf = 0.0f;
    int sampled_color_channel = -1;
    float rand_distance = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        const float probability = color_channel_probability[i];
        const float cdf_next = cdf + probability;
        if (rand_color_channel < cdf_next)
        {
            sampled_color_channel = i;
            rand_distance = (rand_color_channel - cdf) /
                            (cdf_next - cdf);  // avoid numerical issues (denominator equals probability mathematically but not numerically)

            if (rand_distance == 1.0f)
                rand_distance = OpenPBR_LargestFloatBelowOne;  // avoid numerical issues (rand_distance is always less than one mathematically but can
                                                               // be exactly one when calculated in single precision)

            break;
        }
        cdf = cdf_next;
    }

    // Verify that a color channel was selected successfully.
    OPENPBR_ASSERT(sampled_color_channel > -1 && rand_distance < 1.0f, "Color channel was not selected successfully");

    // Make sure the extinction of the sampled color channel is greater than zero;
    // otherwise, the sampled distance is infinity and the distance argument is left as is.
    const float extinction_of_sampled_color_channel = volume.extinction_coefficient[sampled_color_channel];
    if (extinction_of_sampled_color_channel > 0.0f)
    {
        // Perfectly sample an event distance for the selected color channel.
        distance = -log(1.0f - rand_distance) /
                   extinction_of_sampled_color_channel;  // one minus rand is never zero, preventing log from being negative infinity
    }
}

// Calculate the weight to apply to a volume interaction that occurs before a surface interaction.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_weight_for_event_at_distance(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                                volume,
                                                                            const vec3 throughput,
                                                                            const float distance)
{
    // We could return zero for empty volumes, but it doesn't make sense to call this function for empty volumes in the first place.
    OPENPBR_ASSERT(openpbr_is_homogeneous_volume(volume), "Volume interactions should not occur in empty volumes");

    const vec3 color_channel_probability = openpbr_calculate_color_channel_probability(volume, throughput);
    const vec3 transmittance = openpbr_calculate_transmittance_at_distance(volume, distance);

    const vec3 color_channel_pdf = transmittance * volume.extinction_coefficient;        // color-channel pdf at event distance
    const float event_pdf = openpbr_sum(color_channel_probability * color_channel_pdf);  // overall pdf based on contributions of all color channels
    OPENPBR_ASSERT(event_pdf > 0.0f, "Event PDF should be positive");

    // Note: 1.0f/event_pdf could overflow if event_pdf is subnormal.
    return transmittance * openpbr_calculate_scattering_coefficient(volume) *
           (1.0f / event_pdf);  // equals albedo in the case of achromatic extinction
}

// Calculate the weight to apply to a surface interaction that occurs before a volume interaction.
// Specifically, this incorporates the volume transmittance and the probability of the sampled
// volume distance falling beyond the provided surface distance.
OPENPBR_INLINE_FUNCTION vec3 openpbr_calculate_weight_for_surface_at_distance(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                                  volume,
                                                                              const vec3 throughput,
                                                                              const float distance)
{
    // Special case for non-interaction with an empty volume.
    if (!openpbr_is_homogeneous_volume(volume))
    {
        return vec3(1.0f);
    }

    const vec3 color_channel_probability = openpbr_calculate_color_channel_probability(volume, throughput);
    const vec3 transmittance = openpbr_calculate_transmittance_at_distance(volume, distance);

    const float surface_probability = openpbr_sum(
        color_channel_probability * transmittance);  // overall probability of reaching the surface based on contributions of all color channels
    OPENPBR_ASSERT(surface_probability > 0.0f, "Surface probability should be positive");

    // Note: 1.0f/surface_probability could overflow if surface_probability is subnormal.
    return transmittance * (1.0f / surface_probability);  // equals one in the case of achromatic extinction
}

// Perfectly samples the isotropic phase function. The weight is implicitly one.
// The albedo is not relevant because it was already included in the distance-sampling weight.
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_isotropic_phase_function(const vec2 rand)
{
    return openpbr_sample_unit_sphere_uniform(rand);
}

// Returns the value of the isotropic phase function for any direction.
OPENPBR_INLINE_FUNCTION float openpbr_calculate_isotropic_phase_function_value()
{
    return OpenPBR_RcpFourPi;
}

// Returns the probability density of sampling any direction from the isotropic phase function.
OPENPBR_INLINE_FUNCTION float openpbr_calculate_isotropic_phase_function_pdf()
{
    return OpenPBR_RcpFourPi;
}

// Perfectly samples the anisotropic Henyey-Greenstein phase function. The weight is implicitly one.
// The albedo is not relevant because it was already included in the distance-sampling weight.
OPENPBR_INLINE_FUNCTION vec3 openpbr_sample_anisotropic_phase_function(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                           volume,
                                                                       const vec3 view_direction,
                                                                       const vec2 rand)
{
    OPENPBR_ASSERT(abs(volume.anisotropy) < 1.0f, "Invalid volume scattering anisotropy.");

    // For details, see https://www.astro.umd.edu/~jph/HG_note.pdf
    const float g = volume.anisotropy;
    const float s = 2.0f * rand.x - 1.0f;
    const float cos_theta = g == 0 ? s : (0.5f / g) * (1.0f + openpbr_square(g) - openpbr_square((1.0f - openpbr_square(g)) / (1.0f + g * s)));

    const float sin_theta = sqrt(max(0.0f, 1.0f - openpbr_square(cos_theta)));
    const float phi = OpenPBR_TwoPi * rand.y;

    const OpenPBR_Basis basis = openpbr_make_basis(-view_direction);
    return cos_theta * basis.n + sin_theta * cos(phi) * basis.t + sin_theta * sin(phi) * basis.b;
}

// Returns the value of the anisotropic Henyey-Greenstein phase function for any direction.
OPENPBR_INLINE_FUNCTION float
openpbr_calculate_anisotropic_phase_function_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume,
                                                   const vec3 view_direction,
                                                   const vec3 light_direction)
{
    OPENPBR_ASSERT(abs(volume.anisotropy) < 1.0f, "Invalid volume scattering anisotropy.");

    // For details, see https://www.astro.umd.edu/~jph/HG_note.pdf
    const float g = volume.anisotropy;
    const float cos_theta = dot(-view_direction, light_direction);
    return OpenPBR_RcpFourPi * (1.0f - openpbr_square(g)) / openpbr_three_halves_power(1.0f + openpbr_square(g) - 2.0f * g * cos_theta);
}

// Returns the probability density of sampling any direction from the anisotropic Henyey-Greenstein phase function.
OPENPBR_INLINE_FUNCTION float
openpbr_calculate_anisotropic_phase_function_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume,
                                                 const vec3 view_direction,
                                                 const vec3 light_direction)
{
    return openpbr_calculate_anisotropic_phase_function_value(volume, view_direction, light_direction);
}

OPENPBR_INLINE_FUNCTION OpenPBR_HomogeneousVolume openpbr_add_volumes(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                          volume_A,
                                                                      OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume)
                                                                          volume_B,
                                                                      const float volume_A_weight,
                                                                      const float volume_B_weight)
{
    OPENPBR_ASSERT(!openpbr_is_ambient_volume(volume_A), "Adding ambient volumes is not currently supported.");
    OPENPBR_ASSERT(!openpbr_is_ambient_volume(volume_B), "Adding ambient volumes is not currently supported.");

    const vec3 total_absorption_coefficient =
        openpbr_calculate_absorption_coefficient(volume_A) * volume_A_weight + openpbr_calculate_absorption_coefficient(volume_B) * volume_B_weight;
    const vec3 total_scattering_coefficient =
        openpbr_calculate_scattering_coefficient(volume_A) * volume_A_weight + openpbr_calculate_scattering_coefficient(volume_B) * volume_B_weight;
    const float average_anisotropy =
        openpbr_safe_divide(volume_A.anisotropy * volume_A_weight + volume_B.anisotropy * volume_B_weight, volume_A_weight + volume_B_weight, 0.0f);

    return openpbr_make_volume_from_absorption_and_scattering_coefficients_and_anisotropy(
        total_absorption_coefficient, total_scattering_coefficient, average_anisotropy);
}

// Returns the value of Eclair's "weighted roughness" heuristic for volumes.
OPENPBR_INLINE_FUNCTION float
openpbr_calculate_weighted_roughness_for_volume(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_HomogeneousVolume) volume)
{
    // This is empirically derived.
    // TODO(sss): Revisit this.
    return sqrt(1.0f - abs(volume.anisotropy));
}

// This function introduces a weight for shadow rays that reduces or eliminates faceting artifacts that appear on dense volumes inside low-poly
// meshes. The weight is a ratio of cosine terms that compensates for the difference between the projected areas of the imaginary smooth surface and
// the actual geometric surface.
//
// Additional context and details: In the case of shadow rays that originate from a volume and pass straight through the enclosing interface, there is
// no foreshortening cosine term explicitly applied to the weight. However, there is foreshortening cosine term implicitly incorporated into the
// distribution of shadow rays over the surface, and this foreshortening is naturally based on the actual geometric surface; it is unaffected by the
// interpolated normal. In the case of a low-poly mesh containing a dense volume, faceting artifacts that follow the geometric surface become visible.
// What the weight in this function does is divide out the implicitly present cosine term that's based on the geometric normal and multiply in a
// cosine term based on the interpolated normal, which results in the desired smooth appearance across the mesh.
OPENPBR_INLINE_FUNCTION float openpbr_volume_faceting_correction(const bool last_scattered_from_volume,
                                                                 const float side_light,
                                                                 const float side_view,
                                                                 const vec3 light_direction,
                                                                 const vec3 geometric_normal_normalized,
                                                                 const vec3 interpolated_normal_normalized)
{
    if (last_scattered_from_volume && side_light > 0.0f && side_view < 0.0f)  // emerging from volume
    {
        return abs(
            openpbr_safe_divide(dot(light_direction, interpolated_normal_normalized), dot(light_direction, geometric_normal_normalized), 1.0f));
    }
    else
    {
        return 1.0f;
    }
}

#endif
