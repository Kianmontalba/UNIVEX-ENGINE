#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

add_library(uve_render STATIC
    src/null_command_buffer_uve.cpp
    src/null_render_device_uve.cpp
    src/render_system_uve.cpp
)
target_include_directories(uve_render PUBLIC include)
target_link_libraries(uve_render PUBLIC uve_platform uve_debug)
target_compile_features(uve_render PUBLIC cxx_std_20)
uve_set_warnings(uve_render)
