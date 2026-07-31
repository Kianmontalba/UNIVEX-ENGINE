#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------
#
# Relies on engine/config's CMakeLists.txt having already run
# FetchContent_Declare/MakeAvailable(nlohmann_json) — this module deliberately does not repeat
# that declaration. The parent engine/CMakeLists.txt must add_subdirectory(config) before
# add_subdirectory(asset) for the nlohmann_json::nlohmann_json target to already exist here.

add_library(uve_asset STATIC
    src/asset_guid_uve.cpp
    src/asset_database_uve.cpp
    src/uve_file_envelope_uve.cpp
    src/asset_manager_uve.cpp
    src/asset_importer_uve.cpp
    src/hot_reload_uve.cpp
    src/asset_bundle_uve.cpp
    src/file_system_uve.cpp
)
target_include_directories(uve_asset PUBLIC include)
target_link_libraries(uve_asset
    PUBLIC uve_platform uve_debug uve_threading uve_events
    PRIVATE nlohmann_json::nlohmann_json
)
target_compile_features(uve_asset PUBLIC cxx_std_20)
uve_set_warnings(uve_asset)
