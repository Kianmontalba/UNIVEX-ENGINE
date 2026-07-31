#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

include(FetchContent)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

add_executable(uve_tests
    debug/logger_uve_tests.cpp
    debug/log_format_uve_tests.cpp
    utilities/timer_uve_tests.cpp
    events/event_system_uve_tests.cpp
    core/engine_state_uve_tests.cpp
    core/engine_core_uve_tests.cpp
    core/engine_services_uve_tests.cpp
    core/version_uve_tests.cpp
)
target_link_libraries(uve_tests PRIVATE
    uve_platform
    uve_debug
    uve_utilities
    uve_events
    uve_core
    GTest::gtest
    GTest::gtest_main
)
target_compile_features(uve_tests PRIVATE cxx_std_20)
uve_set_warnings(uve_tests)

include(GoogleTest)
gtest_discover_tests(uve_tests)
