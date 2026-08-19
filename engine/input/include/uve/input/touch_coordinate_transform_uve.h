// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include "uve/math/vector2_uve.h"
namespace UVE::Input {
struct TouchCoordinateViewportUVE final {
    float width = 0.0F;
    float height = 0.0F;
    float safeLeft = 0.0F;
    float safeTop = 0.0F;
    float safeRight = 0.0F;
    float safeBottom = 0.0F;
};
/// Maps finite pixel coordinates into safe-area normalized [0,1] coordinates.
/// Value-only contract; it does not poll a platform, own lifecycle, or mutate input snapshots.
[[nodiscard]] bool NormalizeTouchCoordinateUVE(Math::Vector2UVE pixelPosition,
                                               const TouchCoordinateViewportUVE& viewport,
                                               Math::Vector2UVE& outNormalized) noexcept;
} // namespace UVE::Input
