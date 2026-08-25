# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


cmake_minimum_required(VERSION 3.24)
project(UniVexEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(UVE_BUILD_TESTS "Build the UVE unit test suite" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(UveCompilerWarnings)
include(UveFormatSupport)
include(UveBuildConfiguration)

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_Examples OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(engine)

if(UVE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
