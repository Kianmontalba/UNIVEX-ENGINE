# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# WindowManagerUVE is the first module to pull in GLFW3, fetched here exactly like
# nlohmann_json in engine/config/CMakeLists.txt (git-based FetchContent works in this sandbox
# even though raw HTTPS to github.com does not — confirmed by direct testing). GLFW's own headers
# are confined to window_manager_uve.cpp (PRIVATE link) — no public header in this module ever
# includes <GLFW/glfw3.h>. Building GLFW3's own X11 backend additionally requires the system
# X11/OpenGL *development* headers (libgl1-mesa-dev, libglfw3-dev, libxrandr-dev,
# libxinerama-dev, libxcursor-dev, libxi-dev, libxkbcommon-dev) to already be installed — an
# environment prerequisite, not something FetchContent can provide (see docs/CODING_STANDARDS.md,
# mirroring engine/threading's find_package(Threads) precedent).

include(FetchContent)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw3)

add_library(uve_window STATIC
    src/window_manager_uve.cpp
    src/null_window_manager_uve.cpp
)
target_include_directories(uve_window PUBLIC include)
target_link_libraries(uve_window
    PUBLIC uve_platform uve_debug uve_events
    PRIVATE glfw
)
target_compile_features(uve_window PUBLIC cxx_std_20)
uve_set_warnings(uve_window)
