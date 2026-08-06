# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# ThreadPoolUVE is the first module that spawns real std::thread's, so this is
# the first CMakeLists.txt in the project to explicitly find and link
# Threads::Threads rather than relying on implicit toolchain linkage.

find_package(Threads REQUIRED)

add_library(uve_threading STATIC
    src/job_counter_uve.cpp
    src/thread_pool_uve.cpp
)
target_include_directories(uve_threading PUBLIC include)
target_link_libraries(uve_threading PUBLIC uve_platform uve_debug Threads::Threads)
target_compile_features(uve_threading PUBLIC cxx_std_20)
uve_set_warnings(uve_threading)
