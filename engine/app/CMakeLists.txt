# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_executable(uve_runtime src/main.cpp)
target_link_libraries(uve_runtime PRIVATE uve_core)
target_compile_features(uve_runtime PRIVATE cxx_std_20)
uve_set_warnings(uve_runtime)

add_executable(uve_editor_app src/editor/main.cpp)
set_target_properties(uve_editor_app PROPERTIES OUTPUT_NAME uve_editor)
target_link_libraries(uve_editor_app PRIVATE uve_editor)
target_compile_features(uve_editor_app PRIVATE cxx_std_20)
uve_set_warnings(uve_editor_app)

add_executable(uve_project_check src/project_check/main.cpp)
target_link_libraries(uve_project_check PRIVATE uve_project_check_lib)
target_compile_features(uve_project_check PRIVATE cxx_std_20)
uve_set_warnings(uve_project_check)
