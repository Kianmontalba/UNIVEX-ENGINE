#------------------------------------------------------------------------------
# UniVex Engine (UVE) — Proprietary Game Engine
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
# Unauthorized copying, modification, distribution, or use of this code
# in whole or in part is strictly prohibited without express written
# permission from UniVex Studios.
# Violators will be prosecuted to the fullest extent of the law.
#------------------------------------------------------------------------------

add_library(uve_math STATIC
    src/vector3_uve.cpp
    src/quaternion_uve.cpp
    src/matrix4x4_uve.cpp
    src/aabb_uve.cpp
    src/frustum_uve.cpp
)
target_include_directories(uve_math PUBLIC include)
target_link_libraries(uve_math PUBLIC uve_platform)
target_compile_features(uve_math PUBLIC cxx_std_20)
uve_set_warnings(uve_math)
