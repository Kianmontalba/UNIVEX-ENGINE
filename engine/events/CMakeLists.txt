#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

add_library(uve_events STATIC
    src/event_system_uve.cpp
)
target_include_directories(uve_events PUBLIC include)
target_link_libraries(uve_events PUBLIC uve_platform)
target_compile_features(uve_events PUBLIC cxx_std_20)
uve_set_warnings(uve_events)
