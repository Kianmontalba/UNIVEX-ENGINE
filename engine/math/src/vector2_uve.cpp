//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/vector2_uve.h"

namespace UVE::Math {

std::string ToStringUVE(const Vector2UVE& vector) {
    return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ")";
}

} // namespace UVE::Math
