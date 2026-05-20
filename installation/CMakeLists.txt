file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/openpbr-bsdf-config.cmake.in" [=[
@PACKAGE_INIT@

set(OPENPBRBSDF_ENABLE_EXAMPLES @OPENPBRBSDF_ENABLE_EXAMPLES@)

include(CMakeFindDependencyMacro)

if(OPENPBRBSDF_ENABLE_EXAMPLES)
    find_dependency(glm)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/openpbr-bsdf-targets.cmake")

check_required_components("openpbr-bsdf")
]=])

configure_package_config_file(
    "${CMAKE_CURRENT_BINARY_DIR}/openpbr-bsdf-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/openpbr-bsdf-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/openpbr-bsdf
)

install(
    EXPORT      openpbr-bsdf-targets
    NAMESPACE   "openpbr-bsdf::"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/openpbr-bsdf"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/openpbr-bsdf-config.cmake"
    DESTINATION
        "${CMAKE_INSTALL_DATADIR}/openpbr-bsdf"
)
