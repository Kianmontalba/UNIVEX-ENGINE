# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# UVE_BUILD_CONFIGURATION is the product-facing profile name. The underlying
# CMake build type stays conventional so third-party tooling and generators
# continue to recognize the standard optimization/debug-info semantics.
set(UVE_BUILD_CONFIGURATION "Debug" CACHE STRING
    "UVE profile: Debug, Development, Release, or Shipping")
set_property(CACHE UVE_BUILD_CONFIGURATION PROPERTY STRINGS Debug Development Release Shipping)

set(_uve_supported_build_configurations Debug Development Release Shipping)
if(NOT UVE_BUILD_CONFIGURATION IN_LIST _uve_supported_build_configurations)
    message(FATAL_ERROR
        "UVE_BUILD_CONFIGURATION must be one of Debug, Development, Release, or Shipping; got '${UVE_BUILD_CONFIGURATION}'")
endif()

if(UVE_BUILD_CONFIGURATION STREQUAL "Debug")
    set(_uve_cmake_build_type Debug)
    set(_uve_profile_assertions 1)
    set(_uve_profile_default_log_level 0)
elseif(UVE_BUILD_CONFIGURATION STREQUAL "Development")
    set(_uve_cmake_build_type RelWithDebInfo)
    set(_uve_profile_assertions 1)
    set(_uve_profile_default_log_level 1)
elseif(UVE_BUILD_CONFIGURATION STREQUAL "Release")
    set(_uve_cmake_build_type Release)
    set(_uve_profile_assertions 0)
    set(_uve_profile_default_log_level 2)
else()
    set(_uve_cmake_build_type MinSizeRel)
    set(_uve_profile_assertions 0)
    set(_uve_profile_default_log_level 3)
endif()

# UniVex currently uses a single-config workflow (Ninja/GCC and the hosted
# Linux build). The product profile owns the conventional CMake type so a
# cached Debug default cannot silently produce a different profile.
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "${_uve_cmake_build_type}" CACHE STRING "CMake build type" FORCE)
endif()

string(TOUPPER "${UVE_BUILD_CONFIGURATION}" _uve_build_configuration_upper)
add_compile_definitions("UVE_BUILD_CONFIGURATION_${_uve_build_configuration_upper}=1")
if(UVE_BUILD_CONFIGURATION STREQUAL "Shipping")
    add_compile_definitions(UVE_SHIPPING=1)
else()
    add_compile_definitions(UVE_SHIPPING=0)
endif()
add_compile_definitions(UVE_PROFILE_ASSERTIONS_ENABLED=${_uve_profile_assertions})
add_compile_definitions(UVE_PROFILE_DEFAULT_LOG_LEVEL=${_uve_profile_default_log_level})

unset(_uve_build_configuration_upper)
unset(_uve_cmake_build_type)
unset(_uve_profile_assertions)
unset(_uve_profile_default_log_level)
unset(_uve_supported_build_configurations)
