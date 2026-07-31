#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------
#
# Detects whether the active compiler/standard-library combination fully
# supports std::format (C++20). If it does, UVE_STD_FORMAT_AVAILABLE is set
# and no extra dependency is needed. If it does not, {fmt} is fetched via
# FetchContent as a drop-in replacement. Either way, uve_link_format_backend()
# gives the uve_debug target everything it needs, and
# engine/debug/include/uve/debug/log_format_uve.h presents an identical
# FormatLogMessageUVE() signature regardless of which backend is active — no
# engine code outside that one header needs to know which backend was picked.

include(CheckCXXSourceCompiles)
include(FetchContent)

set(CMAKE_REQUIRED_FLAGS "-std=c++20")
check_cxx_source_compiles(
    "#include <format>
     #include <string>
     int main() {
         std::string s = std::format(\"{}-{}\", 1, 2);
         return s == \"1-2\" ? 0 : 1;
     }"
    UVE_STD_FORMAT_AVAILABLE
)
unset(CMAKE_REQUIRED_FLAGS)

if(UVE_STD_FORMAT_AVAILABLE)
    message(STATUS "UVE: std::format is available - using it as the logging format backend")
else()
    message(STATUS "UVE: std::format is NOT available - fetching {fmt} as the logging format backend")
    FetchContent_Declare(
        uve_fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
    )
    FetchContent_MakeAvailable(uve_fmt)
endif()

function(uve_link_format_backend target)
    if(UVE_STD_FORMAT_AVAILABLE)
        target_compile_definitions(${target} PUBLIC UVE_HAS_STD_FORMAT=1)
    else()
        target_compile_definitions(${target} PUBLIC UVE_HAS_STD_FORMAT=0)
        target_link_libraries(${target} PUBLIC fmt::fmt)
    endif()
endfunction()
