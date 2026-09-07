// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/viewport/editor_viewport_camera_uve.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace UVE::Editor {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;
constexpr Math::Vector3UVE kWorldUpUVE{0.0F, 1.0F, 0.0F};

/// Shortest signed angular distance, wrapped into `(-pi, pi]`, so a snap always turns the short
/// way round instead of unwinding past 180 degrees.
[[nodiscard]] float WrapAngleDeltaUVE(const float delta) noexcept {
    float wrapped = std::fmod(delta + kPiUVE, 2.0F * kPiUVE);
    if (wrapped < 0.0F) {
        wrapped += 2.0F * kPiUVE;
    }
    return wrapped - kPiUVE;
}

[[nodiscard]] float EaseOutCubicUVE(const float t) noexcept {
    const float inverse = 1.0F - t;
    return 1.0F - (inverse * inverse * inverse);
}

} // namespace

void EditorViewportCameraUVE::OrbitUVE(const float deltaXPixels, const float deltaYPixels) noexcept {
    if (!std::isfinite(deltaXPixels) || !std::isfinite(deltaYPixels)) {
        return;
    }
    // A manual drag always wins over an in-flight snap.
    m_animating = false;
    m_yawRadians += deltaXPixels * m_settings.orbitRadiansPerPixel;
    m_pitchRadians = std::clamp(m_pitchRadians + (deltaYPixels * m_settings.orbitRadiansPerPixel),
                                m_settings.pitchMinRadians, m_settings.pitchMaxRadians);
}

void EditorViewportCameraUVE::PanUVE(const float deltaXPixels, const float deltaYPixels,
                                     const int viewportHeightPixels) noexcept {
    if (viewportHeightPixels <= 0 || !std::isfinite(deltaXPixels) || !std::isfinite(deltaYPixels)) {
        return;
    }

    // World units covered by one pixel at the pivot's depth.
    const float worldPerPixel =
        (2.0F * m_distance * std::tan(m_settings.fieldOfViewYRadians * 0.5F)) /
        static_cast<float>(viewportHeightPixels);

    const Math::Vector3UVE forward = Math::NormalizeUVE(m_target - GetEyeUVE());
    const Math::Vector3UVE right = Math::NormalizeUVE(Math::CrossUVE(forward, kWorldUpUVE));
    const Math::Vector3UVE up = Math::CrossUVE(right, forward);

    m_target += right * (-deltaXPixels * worldPerPixel);
    m_target += up * (deltaYPixels * worldPerPixel);
}

void EditorViewportCameraUVE::DollyUVE(const float notches) noexcept {
    if (!std::isfinite(notches)) {
        return;
    }
    SetDistanceUVE(m_distance * std::exp(-notches * m_settings.dollyPerWheelNotch));
}

void EditorViewportCameraUVE::SetDistanceUVE(const float distance) noexcept {
    // Only NaN is rejected. An infinity is a meaningful request here - it is what a very large
    // dolly-out produces once `exp()` overflows - and clamping it to the range end is exactly the
    // right answer, whereas ignoring it would silently strand the camera at the opposite bound.
    if (std::isnan(distance)) {
        return;
    }
    m_distance = std::clamp(distance, m_settings.distanceMin, m_settings.distanceMax);
}

void EditorViewportCameraUVE::SetYawPitchUVE(const float yawRadians, const float pitchRadians) noexcept {
    if (!std::isfinite(yawRadians) || !std::isfinite(pitchRadians)) {
        return;
    }
    m_yawRadians = yawRadians;
    m_pitchRadians = std::clamp(pitchRadians, m_settings.pitchMinRadians, m_settings.pitchMaxRadians);
}

Math::Vector3UVE EditorViewportCameraUVE::GetEyeUVE() const noexcept {
    const float cosPitch = std::cos(m_pitchRadians);
    const Math::Vector3UVE offset{
        std::cos(m_yawRadians) * cosPitch,
        std::sin(m_pitchRadians),
        std::sin(m_yawRadians) * cosPitch,
    };
    return m_target + (offset * m_distance);
}

float EditorViewportCameraUVE::GetNearPlaneUVE() const noexcept {
    return std::max(m_settings.nearPlaneMin, m_distance * m_settings.nearPlaneScale);
}

float EditorViewportCameraUVE::GetFarPlaneUVE() const noexcept {
    return m_distance * m_settings.farPlaneScale;
}

float EditorViewportCameraUVE::GetOrthographicHalfHeightUVE() const noexcept {
    return m_distance * std::tan(m_settings.fieldOfViewYRadians * 0.5F);
}

void EditorViewportCameraUVE::SnapToDirectionUVE(const Math::Vector3UVE& worldDirection) noexcept {
    const Math::Vector3UVE direction = Math::NormalizeUVE(worldDirection);
    if (Math::LengthSquaredUVE(direction) <= 0.0F) {
        return;
    }
    const float horizontal =
        std::sqrt((direction.x * direction.x) + (direction.z * direction.z));
    const float targetYaw = std::atan2(direction.z, direction.x);
    // atan2(y, horizontal) is +-pi/2 at the poles; the clamp keeps the view basis from
    // degenerating exactly on-axis for Top and Bottom.
    const float targetPitch = std::clamp(std::atan2(direction.y, horizontal),
                                         m_settings.pitchMinRadians, m_settings.pitchMaxRadians);
    SnapToYawPitchUVE(targetYaw, targetPitch);
}

void EditorViewportCameraUVE::SnapToYawPitchUVE(const float yawRadians,
                                                const float pitchRadians) noexcept {
    if (!std::isfinite(yawRadians) || !std::isfinite(pitchRadians)) {
        return;
    }
    m_fromYawRadians = m_yawRadians;
    m_fromPitchRadians = m_pitchRadians;
    m_deltaYawRadians = WrapAngleDeltaUVE(yawRadians - m_yawRadians);
    m_deltaPitchRadians =
        std::clamp(pitchRadians, m_settings.pitchMinRadians, m_settings.pitchMaxRadians) -
        m_pitchRadians;
    m_elapsedSeconds = 0.0F;
    m_durationSeconds =
        std::max(0.001F, static_cast<float>(m_settings.snapDurationMilliseconds) / 1000.0F);
    m_animating = true;
}

bool EditorViewportCameraUVE::UpdateUVE(const float deltaSeconds) noexcept {
    if (!m_animating || !std::isfinite(deltaSeconds)) {
        return m_animating;
    }
    m_elapsedSeconds += deltaSeconds;
    const float t = std::min(1.0F, m_elapsedSeconds / m_durationSeconds);
    const float eased = EaseOutCubicUVE(t);
    m_yawRadians = m_fromYawRadians + (m_deltaYawRadians * eased);
    m_pitchRadians = std::clamp(m_fromPitchRadians + (m_deltaPitchRadians * eased),
                                m_settings.pitchMinRadians, m_settings.pitchMaxRadians);
    if (t >= 1.0F) {
        m_animating = false;
    }
    return m_animating;
}

void EditorViewportCameraUVE::FocusUVE(const Math::Vector3UVE& point, const float radius) noexcept {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
        return;
    }
    m_target = point;
    if (radius > 0.0F) {
        // Pull back far enough that a sphere of `radius` fits the vertical FOV.
        SetDistanceUVE(radius / std::max(0.05F, std::sin(m_settings.fieldOfViewYRadians * 0.5F)));
    }
}

Math::QuaternionUVE EditorViewportCameraUVE::GetRotationUVE() const noexcept {
    // The view basis is a turntable: yaw about world +Y, then pitch about the camera's own right
    // axis. Solving `R * (0,0,-1) == forward` for the orbit forward direction gives
    // R = Ry(pi/2 - yaw) * Rx(-pitch), which is what is composed here. Expressed as a rotation so
    // it can be written straight onto the ECS camera entity's transform.
    Math::QuaternionUVE yawRotation{};
    Math::QuaternionUVE pitchRotation{};
    if (!Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 1.0F, 0.0F},
                                   (kPiUVE * 0.5F) - m_yawRadians, yawRotation) ||
        !Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F}, -m_pitchRadians,
                                   pitchRotation)) {
        return Math::QuaternionUVE{};
    }
    return Math::MultiplyUVE(yawRotation, pitchRotation);
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetViewMatrixUVE() const noexcept {
    return Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(GetEyeUVE(), GetRotationUVE());
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetProjectionMatrixUVE(const float aspectRatio) const noexcept {
    const float safeAspect = (std::isfinite(aspectRatio) && aspectRatio > 0.0F) ? aspectRatio : 1.0F;
    if (m_orthographic) {
        const float halfHeight = GetOrthographicHalfHeightUVE();
        const float halfWidth = halfHeight * safeAspect;
        // The ortho volume is centred on the pivot, so half the depth range has to sit behind the
        // camera for anything nearer than the pivot to survive clipping.
        const float halfDepth = std::max(m_distance * 4.0F, GetFarPlaneUVE() * 0.5F);
        return Math::Matrix4x4UVE::OrthographicUVE(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                                    -halfDepth, halfDepth);
    }
    return Math::Matrix4x4UVE::PerspectiveUVE(m_settings.fieldOfViewYRadians, safeAspect,
                                               GetNearPlaneUVE(), GetFarPlaneUVE());
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetViewProjectionUVE(const float aspectRatio) const noexcept {
    return GetProjectionMatrixUVE(aspectRatio) * GetViewMatrixUVE();
}

bool EditorViewportCameraUVE::TryGetInverseViewProjectionUVE(
    const float aspectRatio, Math::Matrix4x4UVE& outInverse) const noexcept {
    return Math::TryInverseUVE(GetViewProjectionUVE(aspectRatio), outInverse);
}

} // namespace UVE::Editor
