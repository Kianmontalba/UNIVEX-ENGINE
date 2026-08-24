# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# Relies on engine/config's CMakeLists.txt having already run
# FetchContent_Declare/MakeAvailable(nlohmann_json) — this module deliberately does not repeat
# that declaration. The parent engine/CMakeLists.txt must add_subdirectory(config) before
# add_subdirectory(asset) for the nlohmann_json::nlohmann_json target to already exist here.
# Similarly, add_subdirectory(math) must come before add_subdirectory(asset) for uve_math to
# already exist — MeshAssetUVE/MaterialAssetUVE's public headers use Vector3UVE/AabbUVE.

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)

add_library(uve_asset STATIC
    src/asset_guid_uve.cpp
    src/asset_database_uve.cpp
    src/asset_content_fingerprint_uve.cpp
    src/derived_artifact_cache_uve.cpp
    src/uve_file_envelope_uve.cpp
    src/asset_manager_uve.cpp
    src/asset_importer_uve.cpp
    src/asset_import_queue_uve.cpp
    src/hot_reload_uve.cpp
    src/asset_bundle_uve.cpp
    src/file_system_uve.cpp
    src/project_file_index_uve.cpp
    src/project_change_watcher_uve.cpp
    src/mesh_asset_uve.cpp
    src/texture_asset_uve.cpp
    src/audio_asset_uve.cpp
    src/bmp_metadata_uve.cpp
    src/bmp_importer_uve.cpp
    src/tga_metadata_uve.cpp
    src/tga_importer_uve.cpp
    src/png_metadata_uve.cpp
    src/png_importer_uve.cpp
    src/obj_metadata_uve.cpp
    src/obj_mesh_converter_uve.cpp
    src/obj_importer_uve.cpp
    src/gltf_metadata_uve.cpp
    src/gltf_mesh_converter_uve.cpp
    src/gltf_importer_uve.cpp
    src/jpeg_metadata_uve.cpp
    src/jpeg_importer_uve.cpp
    src/mtl_metadata_uve.cpp
    src/mtl_material_converter_uve.cpp
    src/mtl_importer_uve.cpp
    src/animation_clip_metadata_uve.cpp
    src/shader_asset_uve.cpp
    src/material_asset_uve.cpp
    src/data_table_uve.cpp
    src/data_table_asset_uve.cpp
    src/data_table_asset_registration_uve.cpp
    src/data_table_asset_manager_uve.cpp
    src/data_table_importer_uve.cpp
    src/data_table_pipeline_uve.cpp
    src/data_table_registry_uve.cpp
    src/resource_dependency_graph_uve.cpp
)
target_include_directories(uve_asset PUBLIC include)
target_link_libraries(uve_asset
    PUBLIC uve_platform uve_debug uve_threading uve_events uve_math
    PRIVATE nlohmann_json::nlohmann_json ZLIB::ZLIB JPEG::JPEG
)
target_compile_features(uve_asset PUBLIC cxx_std_20)
uve_set_warnings(uve_asset)
