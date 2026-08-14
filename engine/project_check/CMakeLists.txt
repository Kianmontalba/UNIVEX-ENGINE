# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

add_library(uve_project_check_lib
    src/project_checker_uve.cpp
)

target_include_directories(uve_project_check_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(uve_project_check_lib
    PUBLIC
        uve_asset
)

target_compile_features(uve_project_check_lib PUBLIC cxx_std_20)
uve_set_warnings(uve_project_check_lib)
