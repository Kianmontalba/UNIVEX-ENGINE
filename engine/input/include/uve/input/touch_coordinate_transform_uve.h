// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include "uve/math/vector2_uve.h"
#include <cmath>
#include <cstdint>
namespace UVE::Input {
enum class TouchOrientationUVE : std::uint8_t { Deg0, Deg90, Deg180, Deg270 };
/// Safe-area fields are non-negative inset margins from the corresponding viewport edges.
struct TouchCoordinateViewportUVE final {
    float width = 0.0F;
    float height = 0.0F;
    float safeLeft = 0.0F;
    float safeTop = 0.0F;
    float safeRight = 0.0F;
    float safeBottom = 0.0F;
};

/// Validates a copied mobile viewport/safe-area snapshot whose safe-area fields are inset margins,
/// without acquiring native metrics or selecting a platform coordinate backend.
[[nodiscard]] inline bool ValidateTouchCoordinateViewportUVE(
    const TouchCoordinateViewportUVE& viewport) noexcept {
    return std::isfinite(viewport.width) && std::isfinite(viewport.height) &&
           std::isfinite(viewport.safeLeft) && std::isfinite(viewport.safeTop) &&
           std::isfinite(viewport.safeRight) && std::isfinite(viewport.safeBottom) &&
           viewport.width > 0.0F && viewport.height > 0.0F && viewport.safeLeft >= 0.0F &&
           viewport.safeTop >= 0.0F && viewport.safeRight >= 0.0F && viewport.safeBottom >= 0.0F &&
           viewport.safeLeft < viewport.width && viewport.safeTop < viewport.height &&
           viewport.safeRight < viewport.width - viewport.safeLeft &&
           viewport.safeBottom < viewport.height - viewport.safeTop;
}
/// Applies a finite cardinal orientation to normalized [0,1] touch coordinates.
[[nodiscard]] bool ApplyTouchOrientationUVE(Math::Vector2UVE normalizedPosition,
                                             TouchOrientationUVE orientation,
                                             Math::Vector2UVE& outOriented) noexcept;

/// Maps finite pixel coordinates into safe-area normalized [0,1] coordinates.
/// Value-only contract; it does not poll a platform, own lifecycle, or mutate input snapshots.
[[nodiscard]] bool NormalizeTouchCoordinateUVE(Math::Vector2UVE pixelPosition,
                                               const TouchCoordinateViewportUVE& viewport,
                                               Math::Vector2UVE& outNormalized) noexcept;
} // namespace UVE::Input
