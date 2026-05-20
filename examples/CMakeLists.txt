find_package(glm REQUIRED)

add_executable(openpbr-bsdf-minimal-cpp-example)
add_executable(openpbr-bsdf::minimal-cpp-example ALIAS openpbr-bsdf-minimal-cpp-example)
set_property(TARGET openpbr-bsdf-minimal-cpp-example PROPERTY EXPORT_NAME "minimal-cpp-example")
target_sources(openpbr-bsdf-minimal-cpp-example PRIVATE minimal_cpp_example.cpp)
target_link_libraries(openpbr-bsdf-minimal-cpp-example PRIVATE glm::glm openpbr-bsdf::headers)

get_target_property(sources openpbr-bsdf-minimal-cpp-example SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${sources})

if(OPENPBRBSDF_ENABLE_INSTALL)
    install(TARGETS openpbr-bsdf-minimal-cpp-example EXPORT openpbr-bsdf-targets)
endif()
