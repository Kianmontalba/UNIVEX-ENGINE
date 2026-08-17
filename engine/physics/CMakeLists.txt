# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_library(uve_physics STATIC
    src/area_overlap_lifecycle_tracker_uve.cpp
    src/character_controller_uve.cpp
    src/area_overlap_system_uve.cpp
    src/collider_world_aabb_cache_uve.cpp
    src/collision_system_uve.cpp
    src/physics_system_uve.cpp
    src/raycast_system_uve.cpp
)
target_include_directories(uve_physics PUBLIC include)
target_link_libraries(uve_physics PUBLIC uve_platform uve_debug uve_math uve_scene)
target_compile_features(uve_physics PUBLIC cxx_std_20)
uve_set_warnings(uve_physics)
