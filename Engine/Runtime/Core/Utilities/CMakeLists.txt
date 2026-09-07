# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_library(uve_utilities STATIC
    src/timer_uve.cpp
)
target_include_directories(uve_utilities PUBLIC include)
target_link_libraries(uve_utilities PUBLIC uve_platform)
target_compile_features(uve_utilities PUBLIC cxx_std_20)
uve_set_warnings(uve_utilities)
