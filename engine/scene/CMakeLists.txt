#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------
#
# ArchetypeUVE/ChunkUVE/ArchetypeSignatureUVE are private implementation
# details colocated with their .cpp files under src/ — target_include_directories
# below exposes only include/, so no other target ever sees them.
#
# nlohmann_json is linked PRIVATE (confined to scene_serializer_uve.cpp, exactly like
# ConfigManagerUVE's PIMPL) and relies on engine/config's CMakeLists.txt having already run
# FetchContent_Declare/MakeAvailable(nlohmann_json) — engine/CMakeLists.txt must
# add_subdirectory(config) before add_subdirectory(scene).

add_library(uve_scene STATIC
    src/archetype_signature_uve.cpp
    src/chunk_uve.cpp
    src/archetype_uve.cpp
    src/entity_manager_uve.cpp
    src/scene_graph_uve.cpp
    src/scene_serializer_uve.cpp
    src/prefab_system_uve.cpp
)
target_include_directories(uve_scene PUBLIC include)
target_link_libraries(uve_scene
    PUBLIC uve_platform uve_debug uve_memory uve_events uve_math uve_asset
    PRIVATE nlohmann_json::nlohmann_json
)
target_compile_features(uve_scene PUBLIC cxx_std_20)
uve_set_warnings(uve_scene)
