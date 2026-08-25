# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

add_library(uve_platform STATIC
    src/editor_project_package_uve.cpp
)
target_include_directories(uve_platform PUBLIC include)
target_link_libraries(uve_platform PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(uve_platform PUBLIC cxx_std_20)
uve_set_warnings(uve_platform)
