# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_library(uve_debug STATIC
    src/log_sink_uve.cpp
    src/logger_uve.cpp
)
target_include_directories(uve_debug PUBLIC include)
target_link_libraries(uve_debug PUBLIC uve_platform)
target_compile_features(uve_debug PUBLIC cxx_std_20)
uve_set_warnings(uve_debug)
uve_link_format_backend(uve_debug)
