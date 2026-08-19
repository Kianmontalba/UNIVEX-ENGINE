# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
add_library(uve_network STATIC
    src/reliable_packet_window_uve.cpp
)
target_include_directories(uve_network PUBLIC include)
target_compile_features(uve_network PUBLIC cxx_std_20)
uve_set_warnings(uve_network)
