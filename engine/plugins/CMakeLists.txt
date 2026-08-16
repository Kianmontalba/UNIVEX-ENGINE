# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
add_library(uve_plugins STATIC
    src/plugin_manifest_validation_uve.cpp
    src/plugin_registry_uve.cpp
    motion_query/Source/Runtime/src/motion_query_plugin_module_uve.cpp
    motion_query/Source/Runtime/src/motion_query_asset_sampling_uve.cpp
)
target_include_directories(uve_plugins PUBLIC
    include
    motion_query/Source/Runtime/include
)
target_link_libraries(uve_plugins PUBLIC uve_asset uve_core)
target_compile_features(uve_plugins PUBLIC cxx_std_20)
uve_set_warnings(uve_plugins)
