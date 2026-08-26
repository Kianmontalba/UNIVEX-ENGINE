# Editor Foundation v1 is intentionally an upper-layer module: engine/editor may compose core,
# scene, render, and window services, but no lower engine module may depend on it.
find_package(OpenGL REQUIRED)
include(FetchContent)

# Pinned Dear ImGui source, following the repository's established GLFW/GoogleTest FetchContent
# precedent. v1.91.5 resolves to f401021d5a5d56fe2304056c391e78f81c8d4b8f; ImGui has no target
# suitable for this engine's restricted backend selection, so the exact required core + GLFW/OpenGL
# implementation files are assembled below as an editor-private static target.
FetchContent_Declare(
    uve_imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(uve_imgui)
if(NOT uve_imgui_POPULATED)
    FetchContent_Populate(uve_imgui)
endif()

add_library(uve_editor_imgui STATIC
    ${uve_imgui_SOURCE_DIR}/imgui.cpp
    ${uve_imgui_SOURCE_DIR}/imgui_draw.cpp
    ${uve_imgui_SOURCE_DIR}/imgui_tables.cpp
    ${uve_imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${uve_imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${uve_imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(uve_editor_imgui SYSTEM PUBLIC
    ${uve_imgui_SOURCE_DIR}
    ${uve_imgui_SOURCE_DIR}/backends
)
target_link_libraries(uve_editor_imgui PRIVATE glfw OpenGL::GL)
target_compile_features(uve_editor_imgui PUBLIC cxx_std_20)
# ImGui is third-party source with its own deliberate warning profile. Engine warning-as-error
# policy remains active for every UVE source target, while this pinned dependency is compiled
# without project-specific diagnostics.
target_compile_options(uve_editor_imgui PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-w>)

# Editor-private orientation widget. The widget is built against UVE's existing pinned Dear ImGui
# target; its repository's bundled ImGui submodule is intentionally not fetched.
FetchContent_Declare(
    uve_imguizmo
    GIT_REPOSITORY https://github.com/BrutPitt/imGuIZMO.quat.git
    GIT_TAG 23c8e689ec6b7d9bbfe5231a8d33ae946e2e65be
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(uve_imguizmo)
if(NOT uve_imguizmo_POPULATED)
    FetchContent_Populate(uve_imguizmo)
endif()
add_library(uve_editor_imguizmo STATIC
    ${uve_imguizmo_SOURCE_DIR}/imguizmo_quat/imGuIZMOquat.cpp
    src/editor_imguizmo_orientation_uve.cpp
)
target_include_directories(uve_editor_imguizmo SYSTEM PUBLIC
    ${uve_imguizmo_SOURCE_DIR}/imguizmo_quat
)
target_include_directories(uve_editor_imguizmo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(uve_editor_imguizmo PRIVATE IMGUIZMO_IMGUI_FOLDER=)
target_compile_features(uve_editor_imguizmo PUBLIC cxx_std_20)
target_compile_options(uve_editor_imguizmo PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-w>)
target_link_libraries(uve_editor_imguizmo PRIVATE uve_editor_imgui)

add_library(uve_editor STATIC
    src/editor_bridge_stdio_uve.cpp
    src/editor_bridge_uve.cpp
    src/developer_console_uve.cpp
    src/editor_tool_session_uve.cpp
    src/editor_theme_uve.cpp
    src/editor_ui_assets_uve.cpp
    src/editor_uve.cpp
    src/inspector_drawer_registry_uve.cpp
    src/regression_harness_uve.cpp
)
target_include_directories(uve_editor PUBLIC include)
target_compile_features(uve_editor PUBLIC cxx_std_20)
target_link_libraries(uve_editor
    PUBLIC uve_core uve_scripting uve_plugins
    PRIVATE uve_editor_imgui uve_editor_imguizmo nlohmann_json::nlohmann_json
)
uve_set_warnings(uve_editor)
