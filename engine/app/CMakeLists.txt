# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


add_executable(uve_runtime src/main.cpp)
target_link_libraries(uve_runtime PRIVATE uve_core)
target_compile_features(uve_runtime PRIVATE cxx_std_20)
uve_set_warnings(uve_runtime)
