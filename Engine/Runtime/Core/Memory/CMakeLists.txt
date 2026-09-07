# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_library(uve_memory STATIC
    src/heap_allocator_uve.cpp
    src/pool_allocator_uve.cpp
    src/stack_allocator_uve.cpp
    src/memory_manager_uve.cpp
)
target_include_directories(uve_memory PUBLIC include)
target_link_libraries(uve_memory PUBLIC uve_platform uve_debug)
target_compile_features(uve_memory PUBLIC cxx_std_20)
uve_set_warnings(uve_memory)
