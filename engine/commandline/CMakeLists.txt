# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_library(uve_commandline STATIC
    src/command_line_uve.cpp
)
target_include_directories(uve_commandline PUBLIC include)
target_link_libraries(uve_commandline PUBLIC uve_platform)
target_compile_features(uve_commandline PUBLIC cxx_std_20)
uve_set_warnings(uve_commandline)
