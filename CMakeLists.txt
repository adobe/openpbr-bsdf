cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

#
# Initial setup.
#

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
if(NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
if(NOT DEFINED CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

#
# Options.
#

option(OPENPBRBSDF_ENABLE_INSTALL "Enable installation rules" ON)
option(OPENPBRBSDF_ENABLE_EXAMPLES "Enable examples" ON)

#
# Project.
#

project(openpbr-bsdf)

#
# Toolchain-dependent CMake Libraries.
#

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

#
# Subdirectories.
#

add_subdirectory(include)

if(OPENPBRBSDF_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

#
# Installation.
#

if(OPENPBRBSDF_ENABLE_INSTALL)
    add_subdirectory(installation)
endif()
