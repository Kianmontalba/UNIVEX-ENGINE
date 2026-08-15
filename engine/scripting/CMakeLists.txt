# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
add_library(uve_scripting
    src/script_graph_uve.cpp
    src/script_compiler_ir_uve.cpp
    src/script_bytecode_uve.cpp
    src/script_vm_uve.cpp
    src/script_runtime_uve.cpp
    src/script_graph_persistence_uve.cpp
    src/script_graph_canvas_persistence_uve.cpp
    src/script_graph_editor_backend_uve.cpp
    src/script_graph_canvas_uve.cpp
    src/script_debugger_uve.cpp
    src/script_hot_reload_uve.cpp
)
target_include_directories(uve_scripting PUBLIC include)
target_compile_features(uve_scripting PUBLIC cxx_std_20)
uve_set_warnings(uve_scripting)
target_link_libraries(uve_scripting PUBLIC uve_scene PRIVATE nlohmann_json::nlohmann_json)
