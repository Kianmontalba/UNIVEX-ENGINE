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

add_library(uve_editor STATIC
    src/editor_bridge_uve.cpp
    src/editor_tool_session_uve.cpp
    src/editor_uve.cpp
    src/inspector_drawer_registry_uve.cpp
)
target_include_directories(uve_editor PUBLIC include)
target_compile_features(uve_editor PUBLIC cxx_std_20)
target_link_libraries(uve_editor
    PUBLIC uve_core
    PRIVATE uve_editor_imgui
)
uve_set_warnings(uve_editor)
