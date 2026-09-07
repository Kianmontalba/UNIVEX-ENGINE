// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>

namespace UVE::Scene {

inline constexpr float kMinimumCameraFieldOfViewDegreesUVE = 0.1F;

/// One of the master spec's named built-in components (Part 7.3). It contains only the
/// universally-understood perspective-camera basics; projection matrices and render-target policy
/// remain owned by CameraSystemUVE (Part 7.2).
struct CameraComponentUVE final {
    float fieldOfViewDegrees = 60.0F;
    float nearPlane = 0.1F;
    float farPlane = 1000.0F;

    /// When true, CameraSystemUVE::ComputeProjectionMatrixUVE() builds an orthographic projection
    /// instead of a perspective one - fieldOfViewDegrees is then unused for the projection itself
    /// (it still gates validity below) and orthographicHalfHeight determines the view volume.
    bool orthographic = false;
    /// Half the vertical extent an orthographic view covers, in world units. Ignored when
    /// `orthographic` is false.
    float orthographicHalfHeight = 5.0F;
};

/// Camera values are validated before scene persistence and runtime projection use. The strict
/// open FOV interval avoids tan(FOV/2) singularities, and the positive ordered clip planes keep
/// perspective depth math finite and meaningful.
[[nodiscard]] inline bool IsCameraComponentValidUVE(const CameraComponentUVE& camera) noexcept {
    return std::isfinite(camera.fieldOfViewDegrees) &&
           camera.fieldOfViewDegrees >= kMinimumCameraFieldOfViewDegreesUVE &&
           camera.fieldOfViewDegrees < 180.0F && std::isfinite(camera.nearPlane) && camera.nearPlane > 0.0F &&
           std::isfinite(camera.farPlane) && camera.farPlane > camera.nearPlane &&
           std::isfinite(camera.orthographicHalfHeight) && camera.orthographicHalfHeight > 0.0F;
}

} // namespace UVE::Scene
