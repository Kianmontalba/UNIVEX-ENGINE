# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

if(ANDROID)
    add_library(uve_window STATIC
        src/android_window_manager_uve.cpp
        src/null_window_manager_uve.cpp
        src/window_desc_validation_uve.cpp
        src/monitor_info_validation_uve.cpp
        src/display_mode_validation_uve.cpp
        src/framebuffer_size_validation_uve.cpp
    )
    target_include_directories(uve_window PUBLIC include)
    target_link_libraries(uve_window
        PUBLIC uve_platform uve_debug uve_events uve_input
        PRIVATE android EGL GLESv3
    )
else()
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
        src/window_desc_validation_uve.cpp
        src/monitor_info_validation_uve.cpp
        src/display_mode_validation_uve.cpp
        src/framebuffer_size_validation_uve.cpp
    )
    target_include_directories(uve_window PUBLIC include)
    target_link_libraries(uve_window
        PUBLIC uve_platform uve_debug uve_events uve_input
        PRIVATE glfw
    )
endif()

target_compile_features(uve_window PUBLIC cxx_std_20)
uve_set_warnings(uve_window)
