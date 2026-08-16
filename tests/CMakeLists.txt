# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
    memory/heap_allocator_uve_tests.cpp
    memory/pool_allocator_uve_tests.cpp
    memory/stack_allocator_uve_tests.cpp
    memory/memory_manager_uve_tests.cpp
    memory/allocator_utils_uve_tests.cpp
    memory/memory_alignment_uve_tests.cpp
    threading/job_counter_uve_tests.cpp
    threading/thread_pool_uve_tests.cpp
    commandline/command_line_uve_tests.cpp
    config/config_manager_uve_tests.cpp
    math/vector2_uve_tests.cpp
    math/vector3_uve_tests.cpp
    math/quaternion_uve_tests.cpp
    math/matrix4x4_uve_tests.cpp
    math/aabb_uve_tests.cpp
    math/plane_uve_tests.cpp
    math/frustum_uve_tests.cpp
    physics/collision_system_uve_tests.cpp
    physics/physics_system_uve_tests.cpp
    physics/physics_material_uve_tests.cpp
    physics/raycast_system_uve_tests.cpp
    input/input_system_uve_tests.cpp
    asset/asset_database_uve_tests.cpp
    asset/uve_file_envelope_uve_tests.cpp
    asset/asset_manager_uve_tests.cpp
    asset/asset_importer_uve_tests.cpp
    asset/asset_import_queue_uve_tests.cpp
    asset/hot_reload_uve_tests.cpp
    asset/asset_bundle_uve_tests.cpp
    asset/file_system_uve_tests.cpp
    asset/project_file_index_uve_tests.cpp
    asset/project_change_watcher_uve_tests.cpp
    asset/mesh_asset_uve_tests.cpp
    asset/texture_asset_uve_tests.cpp
    asset/shader_asset_uve_tests.cpp
    asset/material_asset_uve_tests.cpp
    asset/data_table_uve_tests.cpp
    asset/data_table_asset_uve_tests.cpp
    asset/data_table_asset_registration_uve_tests.cpp
    asset/data_table_asset_manager_uve_tests.cpp
    asset/data_table_importer_uve_tests.cpp
    asset/data_table_pipeline_uve_tests.cpp
    asset/data_table_registry_uve_tests.cpp
    asset/resource_dependency_graph_uve_tests.cpp
    render/null_render_device_uve_tests.cpp
    render/render_system_uve_tests.cpp
    render/render_graph_uve_tests.cpp
    render/camera_system_uve_tests.cpp
    render/render_queue_uve_tests.cpp
    render/mesh_renderer_uve_tests.cpp
    render/primitive_geometry_uve_tests.cpp
    render/light_system_uve_tests.cpp
    render/renderer_3d_uve_tests.cpp
    audio/audio_attenuation_model_uve_tests.cpp
    audio/null_audio_device_uve_tests.cpp
    audio/audio_system_uve_tests.cpp
    audio/audio_source_system_uve_tests.cpp
    save/save_game_system_uve_tests.cpp
    save/checkpoint_manager_uve_tests.cpp
    window/null_window_manager_uve_tests.cpp
    window/window_manager_uve_tests.cpp
    render/gl_render_device_uve_tests.cpp
    render/shader/shader_preprocessor_uve_tests.cpp
    render/shader/shader_diagnostics_parser_uve_tests.cpp
    render/shader/shader_binary_cache_uve_tests.cpp
    render/shader/shader_program_uve_tests.cpp
    render/shader/shader_manager_uve_tests.cpp
    scene/entity_manager_uve_tests.cpp
    scene/scene_graph_uve_tests.cpp
    scene/scene_serializer_uve_tests.cpp
    scene/prefab_system_uve_tests.cpp
    core/engine_state_uve_tests.cpp
    core/engine_core_uve_tests.cpp
    core/engine_services_uve_tests.cpp
    core/version_uve_tests.cpp
    core/type_metadata_uve_tests.cpp
    core/time_pose_contract_uve_tests.cpp
    core/animation_clip_uve_tests.cpp
    core/animation_tree_uve_tests.cpp
    core/animation_state_machine_uve_tests.cpp
    core/control_rig_uve_tests.cpp
    core/animation_retargeting_uve_tests.cpp
    core/motion_query_uve_tests.cpp
    core/motion_query_feature_channels_uve_tests.cpp
    core/motion_query_history_uve_tests.cpp
    core/motion_query_database_contract_uve_tests.cpp
    core/frame_scheduler_uve_tests.cpp
    core/profiler_diagnostics_uve_tests.cpp
    editor/editor_tool_session_uve_tests.cpp
    editor/editor_bridge_stdio_uve_tests.cpp
    editor/editor_bridge_uve_tests.cpp
    editor/regression_harness_uve_tests.cpp
    editor/developer_console_uve_tests.cpp
    editor/developer_console_bridge_uve_tests.cpp
    editor/data_table_registry_bridge_uve_tests.cpp
    editor/editor_uve_tests.cpp
    editor/inspector_drawer_registry_uve_tests.cpp
    scripting/script_graph_uve_tests.cpp
    scripting/script_source_mapping_uve_tests.cpp
    plugins/plugin_registry_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_plugin_module_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_asset_sampling_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_asset_registry_cache_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_asset_ingestion_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_runtime_integration_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_schema_compatibility_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_lod_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_continuity_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_transition_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_runtime_telemetry_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_search_index_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_animation_node_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_interaction_runtime_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_editor_authoring_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_debugging_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_trace_replay_uve_tests.cpp
    ../engine/plugins/motion_query/Tests/motion_query_trace_replay_session_uve_tests.cpp
    project_check/project_checker_uve_tests.cpp
)
target_link_libraries(uve_tests PRIVATE
    nlohmann_json::nlohmann_json
    uve_platform
    uve_debug
    uve_utilities
    uve_events
    uve_memory
    uve_threading
    uve_commandline
    uve_config
    uve_math
    uve_asset
    uve_scene
    uve_physics
    uve_render
    uve_input
    uve_audio
    uve_save
    uve_window
    uve_core
    uve_editor
    uve_scripting
    uve_plugins
    uve_project_check_lib
    GTest::gtest
    GTest::gtest_main
)
target_compile_features(uve_tests PRIVATE cxx_std_20)
uve_set_warnings(uve_tests)

# render/shader/shader_preprocessor_uve_tests.cpp and shader_diagnostics_parser_uve_tests.cpp and
# shader_binary_cache_uve_tests.cpp unit-test module-private Detail:: helpers directly (each is
# pure text/data logic with no GL or threading involved, so each one's own doc comment says it is
# "independently unit-testable") - this include path exposes only those module-private headers,
# never anything from engine/render/shader/src as part of uve_render's own public interface.
target_include_directories(uve_tests PRIVATE ${CMAKE_SOURCE_DIR}/engine/render/shader/src)

include(GoogleTest)
gtest_discover_tests(uve_tests)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(
    NAME ContractInventoryUVE.CheckGeneratedReference
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_contract_inventory.py
            --root ${CMAKE_SOURCE_DIR}
            --inventory docs/CONTRACT_INVENTORY.md
            --generated docs/GENERATED_CONTRACT_REFERENCE.md
            --check
)
set_tests_properties(ContractInventoryUVE.CheckGeneratedReference
    PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
