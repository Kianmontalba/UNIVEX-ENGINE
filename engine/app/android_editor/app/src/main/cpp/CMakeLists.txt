cmake_minimum_required(VERSION 3.24)
project(uve_android_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(UVE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." uve_engine)

add_library(native_app_glue STATIC
    "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
)
target_include_directories(native_app_glue PUBLIC
    "${ANDROID_NDK}/sources/android/native_app_glue"
)

add_library(uve_android_engine SHARED
    uve_android_engine.cpp
)
target_compile_options(uve_android_engine PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
)
target_include_directories(uve_android_engine PRIVATE
    "${ANDROID_NDK}/sources/android/native_app_glue"
)
target_link_libraries(uve_android_engine PRIVATE
    native_app_glue
    uve_core
    android
    log
    EGL
    GLESv3
)
