// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/vector2_uve.h"

namespace UVE::Math {

std::string ToStringUVE(const Vector2UVE& vector) {
    return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ")";
}

} // namespace UVE::Math
