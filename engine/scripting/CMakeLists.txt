# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
add_library(uve_scripting
    src/script_graph_uve.cpp
)
target_include_directories(uve_scripting PUBLIC include)
target_compile_features(uve_scripting PUBLIC cxx_std_20)
uve_set_warnings(uve_scripting)
