# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


# Desktop builds use system OpenGL. Android supplies EGL/GLES through the NDK/platform.
if(NOT ANDROID)
    find_package(OpenGL REQUIRED)
endif()

add_library(uve_render STATIC
    src/null_command_buffer_uve.cpp
    src/null_render_device_uve.cpp
    src/render_system_uve.cpp
    src/render_graph_uve.cpp
    src/camera_system_uve.cpp
    src/render_queue_uve.cpp
    src/particle_render_bridge_uve.cpp
    src/particle_draw_command_uve.cpp
    src/mesh_renderer_uve.cpp
    src/mesh_render_eligibility_uve.cpp
    src/primitive_geometry_uve.cpp
    src/light_system_uve.cpp
    src/renderer_3d_uve.cpp
    src/gl_functions_uve.cpp
    src/gl_command_buffer_uve.cpp
    src/gl_render_device_uve.cpp
    src/shader_data_type_uve.cpp
    shader/src/shader_preprocessor_uve.cpp
    shader/src/shader_diagnostics_parser_uve.cpp
    shader/src/shader_binary_cache_uve.cpp
    shader/src/shader_manager_uve.cpp
    shader/src/shader_program_uve.cpp
    shader/src/built_in_shaders_uve.cpp
)
target_include_directories(uve_render PUBLIC include shader/include)
# uve_asset: IMeshRendererUVE/RenderItemUVE reference Asset::IAssetManagerUVE/IAssetDatabaseUVE/
# AssetHandleUVE<T> directly (Increment 13) — the first time engine/render depends on engine/asset.
# engine/asset must never link uve_render back (see engine/asset/CMakeLists.txt).
# uve_window: GlRenderDeviceUVE's public constructor references Window::IWindowManagerUVE&
# (Increment 20) — the first time engine/render depends on engine/window. engine/window must
# never link uve_render back (see engine/window/CMakeLists.txt).
# uve_threading/uve_events: Shader::ShaderManagerUVE's public constructor references
# Threading::IThreadPoolUVE&/Events::IEventSystemUVE& directly (Increment 21) — already transitively
# available via uve_asset's own PUBLIC link, listed explicitly here since uve_render now names
# both types directly in its own public headers too.
# glfw/OpenGL::GL are PRIVATE — confined to gl_render_device_uve.cpp (glfwGetProcAddress) and
# gl_functions_uve.cpp/gl_command_buffer_uve.cpp (raw GL calls) respectively; no public header
# under include/ ever includes a GLFW or GL header.
target_link_libraries(uve_render
    PUBLIC uve_platform uve_debug uve_math uve_scene uve_asset uve_window uve_threading uve_events
)
if(ANDROID)
    target_link_libraries(uve_render PRIVATE android EGL GLESv3)
else()
    target_link_libraries(uve_render PRIVATE glfw OpenGL::GL)
endif()
target_compile_features(uve_render PUBLIC cxx_std_20)
uve_set_warnings(uve_render)
