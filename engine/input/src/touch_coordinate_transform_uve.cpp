// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/input/touch_coordinate_transform_uve.h"
#include <algorithm>
#include <cmath>
namespace UVE::Input {
bool ApplyTouchOrientationUVE(const Math::Vector2UVE normalizedPosition,
                              const TouchOrientationUVE orientation,
                              Math::Vector2UVE& outOriented) noexcept {
    if (!std::isfinite(normalizedPosition.x) || !std::isfinite(normalizedPosition.y) ||
        normalizedPosition.x < 0.0F || normalizedPosition.x > 1.0F || normalizedPosition.y < 0.0F ||
        normalizedPosition.y > 1.0F) {
        return false;
    }
    Math::Vector2UVE oriented;
    switch (orientation) {
        case TouchOrientationUVE::Deg0:
            oriented = normalizedPosition;
            break;
        case TouchOrientationUVE::Deg90:
            oriented = Math::Vector2UVE{1.0F - normalizedPosition.y, normalizedPosition.x};
            break;
        case TouchOrientationUVE::Deg180:
            oriented = Math::Vector2UVE{1.0F - normalizedPosition.x, 1.0F - normalizedPosition.y};
            break;
        case TouchOrientationUVE::Deg270:
            oriented = Math::Vector2UVE{normalizedPosition.y, 1.0F - normalizedPosition.x};
            break;
        default:
            return false;
    }
    outOriented = oriented;
    return true;
}

bool NormalizeTouchCoordinateUVE(const Math::Vector2UVE pixelPosition,
                                 const TouchCoordinateViewportUVE& viewport,
                                 Math::Vector2UVE& outNormalized) noexcept {
    if (!std::isfinite(pixelPosition.x) || !std::isfinite(pixelPosition.y) ||
        !std::isfinite(viewport.width) || !std::isfinite(viewport.height) || viewport.width <= 0.0F ||
        viewport.height <= 0.0F || !std::isfinite(viewport.safeLeft) || !std::isfinite(viewport.safeTop) ||
        !std::isfinite(viewport.safeRight) || !std::isfinite(viewport.safeBottom) || viewport.safeLeft < 0.0F ||
        viewport.safeTop < 0.0F || viewport.safeRight < 0.0F || viewport.safeBottom < 0.0F ||
        viewport.safeLeft + viewport.safeRight >= viewport.width ||
        viewport.safeTop + viewport.safeBottom >= viewport.height) {
        return false;
    }
    const float usableWidth = viewport.width - viewport.safeLeft - viewport.safeRight;
    const float usableHeight = viewport.height - viewport.safeTop - viewport.safeBottom;
    outNormalized.x = std::clamp((pixelPosition.x - viewport.safeLeft) / usableWidth, 0.0F, 1.0F);
    outNormalized.y = std::clamp((pixelPosition.y - viewport.safeTop) / usableHeight, 0.0F, 1.0F);
    return true;
}
} // namespace UVE::Input
