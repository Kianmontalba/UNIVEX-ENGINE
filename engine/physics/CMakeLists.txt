#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

add_library(uve_physics STATIC
    src/collision_system_uve.cpp
    src/physics_system_uve.cpp
)
target_include_directories(uve_physics PUBLIC include)
target_link_libraries(uve_physics PUBLIC uve_platform uve_debug uve_math uve_scene)
target_compile_features(uve_physics PUBLIC cxx_std_20)
uve_set_warnings(uve_physics)
