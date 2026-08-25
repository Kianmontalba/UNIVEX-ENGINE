# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
add_library(uve_plugins STATIC
    src/plugin_manifest_validation_uve.cpp
    src/plugin_registry_uve.cpp
    Animation/control_rig/Source/Runtime/src/control_rig_uve.cpp
    Animation/control_rig/Source/Runtime/src/control_rig_autorig_uve.cpp
    Animation/control_rig/Source/Editor/src/control_rig_editor_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_feature_channels_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_history_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_database_contract_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_plugin_module_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_asset_sampling_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_asset_registry_cache_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_asset_ingestion_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_schema_compatibility_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_lod_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_continuity_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_transition_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_runtime_telemetry_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_search_index_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_animation_node_uve.cpp
    Animation/motion_query/Source/Runtime/src/motion_query_interaction_runtime_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_editor_authoring_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_debugging_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_trace_replay_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_trace_replay_session_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_trace_replay_regression_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_trace_replay_baseline_registry_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_live_debug_session_uve.cpp
    Animation/motion_query/Source/Editor/src/motion_query_live_debug_trace_persistence_uve.cpp
)
target_include_directories(uve_plugins PUBLIC
    include
    Animation/control_rig/Source/Runtime/include
    Animation/control_rig/Source/Editor/include
    Animation/motion_query/Source/Runtime/include
    Animation/motion_query/Source/Editor/include
)
target_link_libraries(uve_plugins PUBLIC uve_asset uve_core PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(uve_plugins PUBLIC cxx_std_20)
uve_set_warnings(uve_plugins)
