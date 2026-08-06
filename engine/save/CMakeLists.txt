# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# nlohmann_json is linked PRIVATE (confined to save_game_system_uve.cpp, exactly like
# SceneSerializerUVE's/ConfigManagerUVE's own PIMPL-confinement pattern) and relies on
# engine/config's CMakeLists.txt having already run FetchContent_Declare/MakeAvailable
# (nlohmann_json) — engine/CMakeLists.txt must add_subdirectory(config) before
# add_subdirectory(save).

add_library(uve_save STATIC
    src/save_game_system_uve.cpp
    src/checkpoint_manager_uve.cpp
)
target_include_directories(uve_save PUBLIC include)
# uve_scene: ISceneSerializerUVE/IEntityManagerUVE (mirrors uve_physics's own uve_scene
# dependency for PhysicsSystemUVE). uve_asset: WriteUveFileUVE/ReadUveFileUVE/AssetKindUVE for
# the .uvesave envelope.
target_link_libraries(uve_save
    PUBLIC uve_platform uve_debug uve_scene uve_asset
    PRIVATE nlohmann_json::nlohmann_json
)
target_compile_features(uve_save PUBLIC cxx_std_20)
uve_set_warnings(uve_save)
