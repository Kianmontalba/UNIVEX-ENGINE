#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

add_library(uve_core STATIC
    src/version_uve.cpp
    src/engine_services_uve.cpp
    src/engine_core_uve.cpp
)
target_include_directories(uve_core PUBLIC include)
target_link_libraries(uve_core PUBLIC uve_platform uve_debug uve_utilities uve_events uve_memory uve_threading)
target_compile_features(uve_core PUBLIC cxx_std_20)
uve_set_warnings(uve_core)
