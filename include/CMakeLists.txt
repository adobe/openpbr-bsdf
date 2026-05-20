add_library(openpbr-bsdf-headers INTERFACE)
add_library(openpbr-bsdf::headers ALIAS openpbr-bsdf-headers)
set_target_properties(openpbr-bsdf-headers PROPERTIES EXPORT_NAME "headers")
target_sources(openpbr-bsdf-headers
    PUBLIC FILE_SET HEADERS
    FILES
        openpbr/impl/data/openpbr_energy_array_access.h
        openpbr/impl/data/openpbr_energy_arrays.h
        openpbr/impl/data/openpbr_ideal_dielectric_avg_energy_complement_data.h
        openpbr/impl/data/openpbr_ideal_dielectric_energy_complement_data.h
        openpbr/impl/data/openpbr_ideal_dielectric_reflection_ratio_data.h
        openpbr/impl/data/openpbr_ideal_metal_avg_energy_complement_data.h
        openpbr/impl/data/openpbr_ideal_metal_energy_complement_data.h
        openpbr/impl/data/openpbr_ltc_array.h
        openpbr/impl/data/openpbr_ltc_data.h
        openpbr/impl/data/openpbr_opaque_dielectric_avg_energy_complement_data.h
        openpbr/impl/data/openpbr_opaque_dielectric_energy_complement_data.h
        openpbr/impl/openpbr_aggregate_lobe.h
        openpbr/impl/openpbr_bsdf.h
        openpbr/impl/openpbr_coating_lobe.h
        openpbr/impl/openpbr_complex.h
        openpbr/impl/openpbr_comprehensive_microfacet_lobe.h
        openpbr/impl/openpbr_constant_reflection_coefficient.h
        openpbr/impl/openpbr_diffuse_lobe.h
        openpbr/impl/openpbr_dispersion_utils.h
        openpbr/impl/openpbr_fuzz_lobe.h
        openpbr/impl/openpbr_generic_flipped_lobe.h
        openpbr/impl/openpbr_generic_minimal_microfacet_lobe.h
        openpbr/impl/openpbr_ior_reflection_coefficient.h
        openpbr/impl/openpbr_lobe_utils.h
        openpbr/impl/openpbr_math.h
        openpbr/impl/openpbr_microfacet_multiple_scattering_data.h
        openpbr/impl/openpbr_microfacet_multiple_scattering_lobes.h
        openpbr/impl/openpbr_reflection_transmission_coefficient.h
        openpbr/impl/openpbr_reflection_transmission_coefficient_structs.h
        openpbr/impl/openpbr_sampling.h
        openpbr/impl/openpbr_thin_film_iridescence_utils.h
        openpbr/impl/openpbr_thin_wall_diffuse_transmission_lobe.h
        openpbr/impl/openpbr_thin_wall_specular_transmission_lobe.h
        openpbr/impl/openpbr_vndf_microfacet_distribution.h
        openpbr/impl/openpbr_volume_utils.h
        openpbr/interop/openpbr_interop.h
        openpbr/interop/openpbr_interop_cpp.h
        openpbr/interop/openpbr_interop_cuda.h
        openpbr/interop/openpbr_interop_glsl.h
        openpbr/interop/openpbr_interop_msl.h
        openpbr/interop/openpbr_interop_slang.h
        openpbr/openpbr.h
        openpbr/openpbr_api.h
        openpbr/openpbr_basis.h
        openpbr/openpbr_bsdf_lobe_type.h
        openpbr/openpbr_constants.h
        openpbr/openpbr_data_constants.h
        openpbr/openpbr_diffuse_specular.h
        openpbr/openpbr_homogeneous_volume.h
        openpbr/openpbr_resolved_inputs.h
        openpbr/openpbr_settings.h
)
target_include_directories(openpbr-bsdf-headers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

get_target_property(sources openpbr-bsdf-headers SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${sources})

if(OPENPBRBSDF_ENABLE_INSTALL)
    install(TARGETS openpbr-bsdf-headers EXPORT openpbr-bsdf-targets FILE_SET HEADERS)
endif()
