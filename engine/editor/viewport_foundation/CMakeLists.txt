# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#
# Vendored, largely unmodified, from the univex_viewport_gl foundation package
# (camera/grid/gizmo core built for this project). Only the build glue below
# is UVE-specific: the app/tools/demo targets from the original package are
# not built here (the engine has its own window and main loop; EditorUVE
# drives this module directly instead), and GLEW is fetched instead of
# requiring a system install, matching engine/window's own FetchContent
# pattern for GLFW.

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/infinite_grid.vert UNIVEX_GRID_VERT_SOURCE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/infinite_grid.frag UNIVEX_GRID_FRAG_SOURCE)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GridShaderSources.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/univex/render/GridShaderSources.h
    @ONLY)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_line.vert  UNIVEX_GIZMO_LINE_VERT)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_line.frag  UNIVEX_GIZMO_LINE_FRAG)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_solid.vert UNIVEX_GIZMO_SOLID_VERT)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_solid.frag UNIVEX_GIZMO_SOLID_FRAG)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GizmoShaderSources.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/univex/render/GizmoShaderSources.h
    @ONLY)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/infinite_grid.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/infinite_grid.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_line.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_line.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_solid.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gizmo_solid.frag)

# Pure-CPU core: math + camera + gizmo geometry. No GL, no windowing.
add_library(univex_viewport_core STATIC
    src/Mat4.cpp
    src/OrbitCamera.cpp
    src/GizmoGeometry.cpp
    src/NavGizmo.cpp)
target_include_directories(univex_viewport_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(univex_viewport_core PUBLIC cxx_std_20)

# GL renderer classes. GLEW is fetched (not system-required) so this module
# never depends on distro packages, matching engine/window's GLFW FetchContent.
include(FetchContent)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG glew-cmake-2.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glew)

add_library(univex_viewport_gl STATIC
    src/ShaderProgram.cpp
    src/InfiniteGridRenderer.cpp
    src/GizmoRenderer.cpp)
target_include_directories(univex_viewport_gl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(univex_viewport_gl PUBLIC univex_viewport_core libglew_static)
target_compile_features(univex_viewport_gl PUBLIC cxx_std_20)

if(UVE_BUILD_TESTS)
    # Not registered via add_test() here: this file is processed by add_subdirectory(engine),
    # which the root CMakeLists.txt runs *before* enable_testing() (that happens right before
    # add_subdirectory(tests)), so a ctest registration made from inside engine/ would silently
    # never take effect. The executable is still built - see tests/CMakeLists.txt, which adds the
    # actual ctest entry from a scope where enable_testing() is already active.
    add_executable(univex_viewport_tests tests/ViewportTests.cpp)
    target_link_libraries(univex_viewport_tests PRIVATE univex_viewport_core)
    target_include_directories(univex_viewport_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(univex_viewport_tests PRIVATE cxx_std_20)
endif()
