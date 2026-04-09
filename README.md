# Adobe's OpenPBR BSDF

A self-contained, portable implementation of the [OpenPBR 1.1](https://academysoftwarefoundation.github.io/OpenPBR/) BSDF, extracted from Adobe's proprietary renderer, Eclair. Written in a GLSL-style language with macros that target C++, GLSL, CUDA, MSL (Metal Shading Language), or Slang, it's designed to drop into any path tracer with minimal setup.

---

## Overview

OpenPBR is an "uber-shader" specification that merges [Adobe Standard Material](https://experienceleague.adobe.com/en/docs/substance-3d/general-knowledge/asm/adobe-standard-material) and [Autodesk Standard Surface](https://autodesk.github.io/standard-surface/) into a single, physically plausible framework. We implement it as a fixed set of nested lobes (e.g., fuzz, coat), all sharing the same interface:

- **Evaluate** the BSDF
- **Sample** the BSDF
- **Compute** the PDF

In addition to those three core functions, the prepared BSDF exposes two additional outputs for direct use by the integrator: a homogeneous volume that consolidates all volumetric contributions from the full OpenPBR parameter set, and surface emission attenuated by the coat and fuzz layers. Volume integration is left to the host renderer, though `openpbr_homogeneous_volume.h` provides production-tested helpers for distance sampling, transmittance, and phase functions.

---

## Why Open Source?

OpenPBR is a powerful but complex material model — implementing it well from scratch requires deep rendering knowledge and a lot of time. We're open-sourcing this to lower that barrier: even a simple path tracer can integrate this production-grade implementation of the full OpenPBR 1.1 parameter set. This is the same code that ships in Eclair today, not a prototype or sample, and we hope it helps teams across the ecosystem adopt OpenPBR faster and validate their own integrations against a real-world reference. The code is released under Apache 2.0.

Note: This is shared as a reference implementation, not run as a community-driven open-source project. Because this code lives in a shipping product, we can't commit to a formal external review process. Pull requests, bug reports, and feature requests are all welcome, and we'll address them on a best-effort basis as our priorities allow. We can't promise to address every item, and large or invasive changes are unlikely to be accepted.

---

## Key Features

- **Single-include API:** `#include "openpbr.h"` brings in the complete public API — all types, settings, and the full BSDF.
- **Multi-language support:** C++, GLSL, CUDA, MSL, or Slang from the same source via a thin interop layer.
- **Self-contained:** No globals, no hidden dependencies — everything lives alongside the BSDF.
- **Configurable:** Compile-time settings in `openpbr_settings.h` control LUT mode, fast math overrides, conflict-suppression hooks, specialization constants, and custom interop.
- **Fixed-lobe architecture:** One struct per material component.
- **Energy-conserving:** The BSDF is designed to be energy-conserving for most common configurations, using precomputed LUTs for multiple-scattering compensation.
- **Reciprocal:** The BSDF satisfies Helmholtz reciprocity for most configurations. See [Path Tracing Direction](#path-tracing-direction) for the exception with transmission. (Note that reciprocity may be traded for better energy conservation in future versions.)

---

## High-Level Architecture

```text
openpbr/
├── openpbr.h                     ← Single include for the entire public API
├── openpbr_settings.h            ← Compile-time feature toggles and configuration
├── openpbr_data_constants.h      ← LUT dimensions and LUT ID constants
├── interop/                      ← Cross-language macro layer (auto-detected per backend)
│   ├── openpbr_interop.h         ← Router: selects the correct backend header
│   └── ...                       ← Per-backend headers (C++, GLSL, CUDA, MSL, Slang)
├── openpbr_resolved_inputs.h     ← Material parameter struct and default initialization
├── openpbr_constants.h           ← Material constants
├── openpbr_basis.h               ← Coordinate frame utilities
├── openpbr_diffuse_specular.h    ← Diffuse/specular component type
├── openpbr_bsdf_lobe_type.h      ← BSDF lobe type flags
├── openpbr_homogeneous_volume.h  ← Volume type (OpenPBR_HomogeneousVolume) and volume integration helpers (distance sampling, transmittance, phase functions)
├── openpbr_api.h                 ← Public BSDF functions (prepare, eval, sample, pdf)
└── impl/                         ← All implementation details (lobes, utilities, data)
    ├── openpbr_bsdf.h                     ← Main BSDF implementation
    ├── openpbr_fuzz_lobe.h                ← Outermost lobe (fuzz/sheen)
    ├── openpbr_coating_lobe.h             ← Coating lobe
    ├── openpbr_aggregate_lobe.h           ← Base layer aggregate
    ├── ...                                ← Other lobe implementations and utilities
    └── data/                              ← Lookup table data arrays
        ├── openpbr_energy_arrays.h        ← Energy compensation table declarations (7 tables)
        ├── openpbr_energy_array_access.h  ← Energy table sampling utilities
        ├── openpbr_ltc_array.h            ← LTC table declaration for fuzz lobe
        └── ...                            ← LUT data files (*_data.h)
```

1. **Resolved Inputs**
   - `OpenPBR_ResolvedInputs` — A struct containing the full OpenPBR 1.1 parameter set after texture evaluation (`base_color`, `specular_roughness`, etc.), plus two required non-spec additions (`geometry_basis` and `geometry_coat_basis`, which replace the spec's `geometry_normal`/`geometry_tangent`/`geometry_coat_normal`/`geometry_coat_tangent` raw vectors) and two optional extensions (`specular_anisotropy_rotation_cos_sin` and `coat_anisotropy_rotation_cos_sin`). See [Departures from the OpenPBR Specification](#departures-from-the-openpbr-specification) for details.
   - `openpbr_make_default_resolved_inputs()` — Creates an `OpenPBR_ResolvedInputs` with default parameter values from the OpenPBR specification.

2. **Initialization**

   - `openpbr_prepare()` → Main entry point. Returns `OpenPBR_PreparedBsdf` containing the volume, emission, and all BSDF state ready for evaluation.

   For renderers that need finer-grained control (e.g., shadow rays that need only the volume), the prepare functions can be called individually in stages — see step 9 in **Getting Started**.

3. **Prepared Outputs**

   `openpbr_prepare()` populates an `OpenPBR_PreparedBsdf` with two public outputs:

   - `prepared.volume` (`OpenPBR_HomogeneousVolume`) — The interior volume of the material: the homogeneous medium that fills the closed surface, derived from the full OpenPBR parameter set. The subsurface scattering and transmission parameters are blended into a single unified volume. Apply it only to ray segments traveling inside a non-thin-walled object. Recompute it at each surface interaction: after either transmission or internal reflection, if the scattered ray is inside the object, use the newly prepared volume for that next interior segment. Contains extinction, single-scattering albedo, and phase-function anisotropy for use by your renderer's volumetric light transport system (see item 5 in the [High-Level Architecture](#high-level-architecture) section for production-tested integration helpers).
   - `prepared.emission` (`vec3`) — Surface emission in nits (cd/m²), attenuated by coat and fuzz layers. Zero when hitting non-thin-walled geometry from the inside.

4. **BSDF Evaluation**
   - `openpbr_eval()` → Returns BSDF value multiplied by the cosine of the angle between the light direction and the shading normal (i.e., f(ωᵢ, ωₒ) × |cos θᵢ|).
   - `openpbr_sample()` → Importance-samples a light direction; returns weight, PDF, and sampled lobe type.
   - `openpbr_pdf()` → Returns sampling PDF for a given light direction.

5. **Volume Integration Helpers** (in `openpbr_homogeneous_volume.h`)

   `openpbr_homogeneous_volume.h` provides a complete, production-tested set of helpers for integrators that act on `prepared.volume`:
   - `openpbr_sample_event_distance()` → Samples a scattering-event distance with spectrally-aware MIS across color channels.
   - `openpbr_calculate_weight_for_event_at_distance()` / `openpbr_calculate_weight_for_surface_at_distance()` → Unbiased path weights for volume and surface interactions, respectively.
   - `openpbr_sample_isotropic_phase_function()` / `openpbr_sample_anisotropic_phase_function()` → Importance-sample the isotropic or Henyey-Greenstein phase function.
   - `openpbr_calculate_isotropic_phase_function_value()` / `openpbr_calculate_isotropic_phase_function_pdf()` and `openpbr_calculate_anisotropic_phase_function_value()` / `openpbr_calculate_anisotropic_phase_function_pdf()` → Evaluate phase function values and PDFs for the isotropic and Henyey-Greenstein models.
   - `openpbr_calculate_transmittance_at_distance()` / `openpbr_calculate_transmittance_at_infinity()` → Beer–Lambert transmittance.
   - Multiple constructors for `OpenPBR_HomogeneousVolume` from extinction+albedo, absorption+scattering, or extinction alone.

6. **Advanced Shading API** (in `impl/openpbr_bsdf.h`; not yet part of the stable public API — signatures may evolve; see comments above each function for full preconditions)
   - `openpbr_translucent_shadow_weight_and_prob()` → Returns a straight-through shadow weight and probability for transmissive/SSS materials, enabling a practical biased shadow shortcut.

7. **Lobe Interface Pattern**

   GLSL has no class system, so each lobe is implemented as a plain struct paired with free functions that take the struct as their first argument. Every lobe exposes the same three function signatures:

   ```glsl
   openpbr_eval(lobe, light_direction)
   openpbr_sample(lobe, rand, light_direction, weight, pdf, sampled_type)
   openpbr_pdf(lobe, light_direction)
   ```

   The correct implementation is selected by the struct type via function overloading — compile-time polymorphism with no virtual dispatch overhead. The outermost lobe composes inward, calling the same interface on each inner lobe, so adding or replacing a lobe means implementing the same three functions for its struct type.

---

## Lookup Tables: Dual-Mode Architecture

The BSDF uses precomputed lookup tables (LUTs) for physically accurate energy compensation and fuzz/sheen lobe evaluation:

- Energy compensation tables: 7 tables for microfacet multiple scattering (ideal/opaque dielectrics, ideal metals)
- LTC table: Linearly Transformed Cosines for the fuzz/sheen lobe

These tables support two modes via the `OPENPBR_USE_TEXTURE_LUTS` compile-time switch:

### Array Mode (`OPENPBR_USE_TEXTURE_LUTS = 0`)
- Tables embedded as constant arrays in shader code
- No texture bindings needed
- Software linear/bilinear/trilinear interpolation
- Larger shader binary size
- Slower on GPU

### Texture Mode (`OPENPBR_USE_TEXTURE_LUTS = 1`)
- Tables stored as 1D/2D/3D textures
- Requires texture binding infrastructure
- Hardware-accelerated filtering and interpolation
- Smaller shader binary size
- Faster on GPU

**Array Mode is the default** because it eliminates external dependencies on texture assets and binding infrastructure.

Source data format:
- All LUT data is stored in separate header files in the `impl/data/` directory
- Data is formatted as flattened C arrays matching the original multi-dimensional layout (e.g., `array[x][y][z]` with rightmost index contiguous)
- Users can read these files directly to populate GPU textures if needed

Implementation details:
- Energy tables: 32×32 or 32×32×32 normalized integer values [0, 65535] → [0.0, 1.0]
  - Storage type is auto-selected per language by the interop layer: 16-bit (`unsigned short`) for C++/MSL/CUDA/Slang; 32-bit (`uint`) for GLSL — no user action required
- LTC table: 32×32 vec3 values
- Both modes use identical indexing and coordinate mappings for exact equivalence

---

## Getting Started

1. Clone this repository.
2. Include `openpbr.h` in your renderer (this provides the complete public API).
3. Select a shading language backend. Three backends are auto-detected from built-in compiler macros:
   - CUDA (`__CUDACC__`), MSL (`__METAL_VERSION__`), and C++ (`__cplusplus`) are detected automatically.
   - GLSL is the default fallback when none of the above macros are defined — no explicit setting needed.
   - Slang has no standard built-in detection macro, so it **requires** `OPENPBR_LANGUAGE_TARGET_SLANG=1` to be defined before including `openpbr.h`. Without it, the GLSL backend will be selected, which will likely fail to compile under the Slang compiler. (For HLSL-style pipelines, use this Slang path.)

   Any backend can also be forced explicitly by setting the corresponding `OPENPBR_LANGUAGE_TARGET_CPP`, `_GLSL`, `_MSL`, `_CUDA`, or `_SLANG` flag, which overrides auto-detection.

   **C++ users** must also pre-include GLM (`<glm/glm.hpp>`) before `openpbr.h`; the interop header will emit a clear error if it is missing. See `minimal_cpp_example.cpp` for a complete working example.
4. Choose lookup table mode: Set `OPENPBR_USE_TEXTURE_LUTS` to 0 for self-contained array mode (default) or 1 for texture mode. In texture mode, define `OPENPBR_SAMPLE_2D_TEXTURE(lut_id, uv)` and `OPENPBR_SAMPLE_3D_TEXTURE(lut_id, uvw)` before `openpbr.h`; the headers emit clear errors if either macro is missing. The available texture IDs are documented in `openpbr_data_constants.h`.
5. Optional — Pre-include hooks: Define any of the following before `openpbr.h` to customize behavior:
   - **Fast math:** `OPENPBR_FAST_RCP_SQRT(x)`, `OPENPBR_FAST_SQRT(x)`, `OPENPBR_FAST_NORMALIZE(v)` — supply faster platform-specific implementations (GPU hardware intrinsics, CPU approximations, etc.). Any undefined hook falls back to the standard library.
   - **Conflict suppression:** Set `OPENPBR_USE_CUSTOM_SATURATE = 1` if your host already defines `saturate()`; set `OPENPBR_USE_CUSTOM_VEC_TYPES = 1` if it already provides `vec2`/`vec3`/`vec4` and, in C++, the GLM math function aliases. (For GLSL the vec types are built-in, so `OPENPBR_USE_CUSTOM_VEC_TYPES` has no effect on that backend.)
   - **Specialization constants:** Override `OPENPBR_GET_SPECIALIZATION_CONSTANT(name)` to connect feature-toggle queries to your renderer's pipeline. OpenPBR calls this with one of the four toggle names (`EnableSheenAndCoat`, `EnableDispersion`, `EnableTranslucency`, `EnableMetallic`) and uses the returned bool to enable or skip entire lobe code paths. The default always returns `true`, enabling all features — correct for offline renderers and standalone use. See `openpbr_settings.h` for the full contract.
   - **Custom interop (advanced):** Set `OPENPBR_USE_CUSTOM_INTEROP = 1` and provide your own interop header before `openpbr.h` to replace the entire language abstraction layer. Prefer `OPENPBR_USE_CUSTOM_SATURATE` and `OPENPBR_USE_CUSTOM_VEC_TYPES` for targeted suppressions; reach for this only if you need to replace every macro OpenPBR defines.
6. Populate `OpenPBR_ResolvedInputs` with texture-evaluated parameters and geometry data, or start with `openpbr_make_default_resolved_inputs()` and override specific properties.
7. Initialize the BSDF by calling `openpbr_prepare()`, which returns an `OpenPBR_PreparedBsdf` ready for evaluation.
8. Evaluate using `openpbr_eval()`, `openpbr_sample()`, or `openpbr_pdf()` with the prepared BSDF.
9. **Staged initialization (advanced):** For renderers that need to skip unnecessary work (e.g., shadow rays that only need the volume), `openpbr_prepare_volume()`, `openpbr_prepare_lobes()`, and `openpbr_prepare_emission()` can be called individually — in that order, as needed — instead of `openpbr_prepare()`. Each function populates only its own fields within `OpenPBR_PreparedBsdf`; be sure to only read fields that have been initialized by the calls you've made. See `impl/openpbr_bsdf.h` for full signatures and preconditions (this file is already included by `openpbr.h`).

---

## Minimal Usage Example

```cpp
// 1. Include the entire OpenPBR BSDF
#include "openpbr.h"

// 2. Create default inputs and customize
OpenPBR_ResolvedInputs inputs = openpbr_make_default_resolved_inputs();
inputs.base_color = vec3(0.1f, 0.9f, 0.1f);  // green base color for illustration

// 3. Prepare the BSDF, volume, and emission
const OpenPBR_PreparedBsdf prepared = openpbr_prepare(inputs,
                                                      vec3(1.0f),                     // path throughput (for importance sampling)
                                                      OpenPBR_BaseRgbWavelengths_nm,  // RGB wavelengths in nanometers
                                                      OpenPBR_VacuumIor,              // exterior IOR
                                                      view_direction);                // incident direction (pointing away from surface)

// 4. Sample and/or evaluate the BSDF
vec3 light_direction;
OpenPBR_DiffuseSpecular weight;
float pdf;
OpenPBR_BsdfLobeType sampled_type;
openpbr_sample(prepared,
               vec3(rand1, rand2, rand3),  // three independent uniform random numbers in [0,1)
               light_direction,            // outgoing direction
               weight,                     // BSDF * cosine / PDF; multiply into path throughput
               pdf,                        // positive if sample is valid — check before using outputs
               sampled_type);              // the type of lobe that was sampled
```

### Standalone C++ example

A self-contained example is provided in `minimal_cpp_example.cpp`. It demonstrates the core path-tracing loop — prepare the BSDF, importance-sample a new direction, and accumulate the throughput weight — with a minimal random-number generator so no external dependencies are needed beyond GLM. Build instructions are at the top of the file.

---

## Important Usage Notes

### Geometry Opacity

`geometry_opacity` is not consumed by the BSDF. The host renderer should implement opacity itself (e.g., stochastic cutout), skipping the entire shading evaluation for transparent samples.

### Coordinate Space Requirements

All directional vectors and geometric data passed to the BSDF must be in a consistent coordinate space:

- `view_direction` and `light_direction` must use the same coordinate system
- Typically world space, but any consistent space works (object space, tangent space, etc.)
- Shading normals and basis vectors in `OpenPBR_ResolvedInputs` must also be in this same space
- All directions point away from the surface (not toward it)
- All input unit vectors (`view_direction`, `light_direction`, basis normals/tangents/bitangents) are assumed to be normalized unless otherwise specified; assertions guard this at key entry points

#### Internal Local Space Convention

Internally, lobes that require a local frame convert directions to a z-up local space via `OpenPBR_Basis`. In this frame the normal aligns with the +Z axis, the tangent with +X, and the bitangent with +Y. Variables suffixed with `_local` are expressed in this local frame. Callers never need to work in this space directly — it is an internal implementation detail.

#### Geometry Bases and Default Coordinate Frame

`OpenPBR_ResolvedInputs` uses two pre-orthonormalized `OpenPBR_Basis` structs — `geometry_basis` and `geometry_coat_basis` — in place of the spec's four geometry vector parameters (see [Departures from the OpenPBR Specification](#departures-from-the-openpbr-specification)). The default returned by `openpbr_make_default_resolved_inputs()` is a z-up identity frame (tangent = X, bitangent = Y, normal = Z). In a real renderer, populate them from the geometry at the ray-surface intersection, e.g.:

```cpp
inputs.geometry_basis.t = world_tangent;    // from mesh UVs or procedural tangent
inputs.geometry_basis.b = world_bitangent;  // derived as cross(n, t) * handedness
inputs.geometry_basis.n = world_normal;     // interpolated or normal-mapped
```

As long as `view_direction`, `light_direction`, and the basis vectors are all expressed in the same space, any consistent choice of space will produce correct results.

### Distance Units

As per the OpenPBR specification, distances are assumed to be in world-space units. If different units are needed, unit conversions need to happen outside the BSDF code.

### View Direction Consistency

The `view_direction` passed to `openpbr_prepare()` is cached internally for energy compensation calculations. This means:

- The same `view_direction` must be used for all subsequent `openpbr_eval()`, `openpbr_sample()`, and `openpbr_pdf()` calls on that prepared BSDF
- Using a different view direction during evaluation would produce incorrect energy compensation and physically inaccurate results
- When using staged initialization (`openpbr_prepare_volume()` + `openpbr_prepare_lobes()`), `openpbr_prepare_lobes()` caches the view direction in `prepared` automatically — no separate assignment is needed

### Volume Integration

Actual volume integration — random walks, transmittance queries, shadow rays through volumes — is left to the integrator. `openpbr_homogeneous_volume.h` provides production-tested helpers; see item 5 in the [High-Level Architecture](#high-level-architecture) section for the full list.

### Path Tracing Direction

This BSDF is designed for unidirectional path tracing from camera to lights. Bidirectional methods (photon mapping, light tracing) would require an adjoint BSDF with inverted square IOR scaling for transmission.

### Sampling Return Behavior

`openpbr_sample()` returns `void`, but indicates success/failure through the `pdf` output parameter:

- **`pdf > 0`**: Valid sample generated; `light_direction` and `weight` are set
- **`pdf == 0`**: No valid sample; output parameters are undefined
- **`pdf` is never negative**; it's always ≥ 0

Check the PDF value to determine if sampling succeeded before using the outputs.

The `weight` accounts for the entire BSDF — effectively MIS across all lobes — whereas `sampled_type` identifies only the lobe that generated the sample.

### Diffuse/Specular Splits

The BSDF functions return BSDF values and weights in `OpenPBR_DiffuseSpecular` format, which contains diffuse and specular components that together add up to the overall value. In cases where only the overall value is needed, it can be retrieved with `openpbr_get_sum_of_diffuse_specular()`.

### RGB Wavelength Sampling

The `rgb_wavelengths_nm` parameter to `openpbr_prepare()` specifies the wavelength (in nanometers) associated with each color channel. For standard RGB rendering, pass `OpenPBR_BaseRgbWavelengths_nm`, or substitute your own representative center wavelengths for the color channels your renderer uses.

Materials with dispersion (wavelength-dependent IOR in dielectrics) or thin-film iridescence (interference that shifts with wavelength) benefit from *stochastically-sampled* wavelengths instead. For each path, draw one random wavelength per channel from that channel's empirical camera spectral sensitivity curve and pass the three samples as `rgb_wavelengths_nm`. The three channels are evaluated independently, with no path reuse across them. Over many paths, this lets spectral effects integrate smoothly across the visible spectrum: dispersion produces natural rainbow colors rather than discrete RGB bands, and thick thin-film converges to a neutral gray rather than unresolvable fine-scale ripples. Purely wavelength-independent materials are unaffected.

To check whether a material activates these effects before deciding how to sample wavelengths, call `openpbr_needs_rgb_wavelengths(resolved_inputs)` (in `impl/openpbr_bsdf.h`).

---

## Departures from the OpenPBR Specification

`OpenPBR_ResolvedInputs` implements the full [OpenPBR 1.1 parameter set](https://academysoftwarefoundation.github.io/OpenPBR/) with two intentional departures described below.

### 1. Geometry parameters replaced by pre-orthonormalized basis structs

The OpenPBR 1.1 specification defines four geometry input parameters:

| Spec parameter          | Type      | Description               |
|-------------------------|-----------|---------------------------|
| `geometry_normal`       | `vector3` | Surface shading normal    |
| `geometry_tangent`      | `vector3` | Surface tangent           |
| `geometry_coat_normal`  | `vector3` | Coat-layer shading normal |
| `geometry_coat_tangent` | `vector3` | Coat-layer tangent        |

This implementation replaces those four parameters with two pre-orthonormalized `OpenPBR_Basis` structs:

| Struct field          | Encodes                                                                         |
|-----------------------|---------------------------------------------------------------------------------|
| `geometry_basis`      | `geometry_normal`, `geometry_tangent`, and the derived bitangent                |
| `geometry_coat_basis` | `geometry_coat_normal`, `geometry_coat_tangent`, and the derived coat bitangent |

Callers must supply orthonormalized bases. `openpbr_make_basis()` (in `openpbr_basis.h`) provides overloads that accept `(normal)`, `(normal, tangent, handedness)`, or `(normal, tangent, bitangent)` and perform orthonormalization via modified Gram–Schmidt.

This representation is used because the BSDF performs numerous dot products, cross products, and coordinate-frame changes in local space. Storing a pre-orthonormalized frame eliminates repeated normalization inside the hot path and ensures the local frame is consistent across all lobes regardless of floating-point rounding in the caller's tangent computation.

### 2. Anisotropy rotation: (cos θ, sin θ) extension (not in OpenPBR spec)

OpenPBR 1.1 defines `specular_roughness_anisotropy` and `coat_roughness_anisotropy` (anisotropy magnitude in `[0, 1]`), but provides no rotation angle parameter. This implementation adds two optional extension fields:

| Extension field                        | Type   | Default  | Description                                        |
|----------------------------------------|--------|----------|----------------------------------------------------|
| `specular_anisotropy_rotation_cos_sin` | `vec2` | `(1, 0)` | (cos θ, sin θ) of the specular anisotropy rotation |
| `coat_anisotropy_rotation_cos_sin`     | `vec2` | `(1, 0)` | (cos θ, sin θ) of the coat anisotropy rotation     |

Both default to `(1, 0)` = (cos 0°, sin 0°), which is a no-op equivalent to having no rotation at all. The vec2 does not need to be unit-length (as can happen after texture filtering); the BSDF normalizes the rotated tangent frame internally.

This representation is used for two main reasons:

1. **Texture-filtering correctness.** A scalar angle texture has a hard discontinuity at the 0°/360° wrap boundary: two texels that are nearly identical in angle would be filtered toward 180° across the seam. Storing (cos θ, sin θ) keeps both components smooth and continuous everywhere, so bilinear or trilinear filtering across that boundary produces the correct interpolated rotation.
2. **Computational efficiency.** The BSDF uses the rotation directly as (cos, sin) to rotate the anisotropy tangent frame, so storing it in that form avoids a per-shading-point trig conversion from an angle representation.

When sourcing rotation from a scalar angle (e.g., a material parameter rather than a texture), encode it with `vec2(cos(angle_radians), sin(angle_radians))`.

---

## Known Limitations

### Thin Film with Thin-Walled Geometry

Thin-film iridescence and thin-walled transmission are not currently compatible: when `geometry_thin_walled` is enabled, iridescence is not visible on the transmission component. (Thin-walled subsurface scattering is unaffected and shows iridescence normally.)

Note that thin-walled mode is not the right tool for soap-bubble or iridescent membrane effects regardless of this limitation — it models an incoherent two-surface windowpane (suited to thick glass), not a nanometer-scale interference coating. For those effects, use a *closed solid surface* (`geometry_thin_walled = false`) with the interior specular IOR set to 1.0 (air) and thin film enabled.

---

## Contribution and Roadmap

We welcome issues and pull requests, for example:

- Enhancements to the simple demo app (with the goal of keeping it minimal, self-contained, and readable — it illustrates how to use the BSDF API, not how to build a renderer; more complex tests belong in a separate program)
- Expanded unit tests for evaluation, sampling, and energy conservation
- Testing of the CUDA backend, which hasn't yet been used in production code
- Testing of the Slang backend in HLSL-style pipelines (and, if needed, adapting the Slang interop aliases or proposing a dedicated HLSL interop header)

Planned or potential future work:

- Specialization constants currently cover four per-lobe feature toggles (`EnableSheenAndCoat`, `EnableDispersion`, `EnableTranslucency`, `EnableMetallic`), all defaulting to `true`; more may be added (e.g., for thin-film and thin-wall) to cover additional lobes
- Reciprocity may be traded for better energy conservation in a future version
- 1D texture sampler support: energy tables with a single dimension are currently stored as thin 2D textures (1 × N); using a real 1D sampler when the platform supports it would reduce sampler overhead
- Implementing OpenPBR 1.2
- Continued iteration on the implementation — some naming conventions and API details may evolve as this is live production code

When contributing code, please follow the existing style: `snake_case` for names, `const` on every variable and parameter that won't be reassigned, and descriptive full-word names — no nonstandard abbreviations or single-letter variables except in tightly scoped math contexts.

Please refer to [CONTRIBUTING.md](https://github.com/adobe/.github/blob/main/.github/CONTRIBUTING.md) for details and guidelines.

---

## License

This code is released under the Apache License 2.0.
