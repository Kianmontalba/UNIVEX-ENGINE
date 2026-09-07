// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/viewport/editor_viewport_camera_uve.h"

#include <cmath>
#include <numbers>

#include "univex/camera/OrbitCamera.h"
#include "univex/math/Mat4.h"
#include "univex/math/Vec.h"

namespace UVE::Editor {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;

[[nodiscard]] univex::math::Vec3 ToUnivexVec3UVE(const Math::Vector3UVE& v) noexcept {
    return univex::math::Vec3{v.x, v.y, v.z};
}

[[nodiscard]] Math::Vector3UVE ToEngineVector3UVE(const univex::math::Vec3& v) noexcept {
    return Math::Vector3UVE{v.x, v.y, v.z};
}

/// `univex::math::Mat4` is column-major (`m[c*4+r]`, i.e. `At(row, col)`); `Math::Matrix4x4UVE` is
/// row-major (`m[row][col]`). Both represent exactly the same matrix - this is a storage-layout
/// transcription, not a re-derivation of any value.
[[nodiscard]] Math::Matrix4x4UVE ToEngineMatrixUVE(const univex::math::Mat4& m) noexcept {
    Math::Matrix4x4UVE result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = m.At(row, col);
        }
    }
    return result;
}

[[nodiscard]] univex::camera::OrbitCameraSettings ToUnivexSettingsUVE(
    const EditorViewportCameraSettingsUVE& settings) noexcept {
    univex::camera::OrbitCameraSettings result{};
    result.fovYRadians = settings.fieldOfViewYRadians;
    result.pitchMin = settings.pitchMinRadians;
    result.pitchMax = settings.pitchMaxRadians;
    result.distanceMin = settings.distanceMin;
    result.distanceMax = settings.distanceMax;
    result.orbitRadiansPerPixel = settings.orbitRadiansPerPixel;
    result.dollyPerWheelNotch = settings.dollyPerWheelNotch;
    result.snapDurationMs = settings.snapDurationMilliseconds;
    result.nearPlaneScale = settings.nearPlaneScale;
    result.farPlaneScale = settings.farPlaneScale;
    result.nearPlaneMin = settings.nearPlaneMin;
    return result;
}

} // namespace

struct EditorViewportCameraUVE::CameraImplUVE {
    univex::camera::OrbitCamera camera;
};

EditorViewportCameraUVE::EditorViewportCameraUVE()
    : EditorViewportCameraUVE(EditorViewportCameraSettingsUVE{}) {}

EditorViewportCameraUVE::EditorViewportCameraUVE(const EditorViewportCameraSettingsUVE& settings) noexcept
    : m_settings(settings), m_camera(std::make_unique<CameraImplUVE>(CameraImplUVE{
                                 univex::camera::OrbitCamera(ToUnivexSettingsUVE(settings))})) {}

EditorViewportCameraUVE::~EditorViewportCameraUVE() = default;
EditorViewportCameraUVE::EditorViewportCameraUVE(EditorViewportCameraUVE&&) noexcept = default;
EditorViewportCameraUVE& EditorViewportCameraUVE::operator=(EditorViewportCameraUVE&&) noexcept = default;

void EditorViewportCameraUVE::OrbitUVE(const float deltaXPixels, const float deltaYPixels) noexcept {
    if (!std::isfinite(deltaXPixels) || !std::isfinite(deltaYPixels)) {
        return;
    }
    m_camera->camera.Orbit(deltaXPixels, deltaYPixels);
}

void EditorViewportCameraUVE::PanUVE(const float deltaXPixels, const float deltaYPixels,
                                     const int viewportHeightPixels) noexcept {
    if (!std::isfinite(deltaXPixels) || !std::isfinite(deltaYPixels)) {
        return;
    }
    m_camera->camera.Pan(deltaXPixels, deltaYPixels, viewportHeightPixels);
}

void EditorViewportCameraUVE::DollyUVE(const float notches) noexcept {
    if (!std::isfinite(notches)) {
        return;
    }
    m_camera->camera.Dolly(notches);
}

void EditorViewportCameraUVE::SetTargetUVE(const Math::Vector3UVE& target) noexcept {
    m_camera->camera.SetTarget(ToUnivexVec3UVE(target));
}

void EditorViewportCameraUVE::SetDistanceUVE(const float distance) noexcept {
    if (std::isnan(distance)) {
        return;
    }
    m_camera->camera.SetDistance(distance);
}

void EditorViewportCameraUVE::SetYawPitchUVE(const float yawRadians, const float pitchRadians) noexcept {
    if (!std::isfinite(yawRadians) || !std::isfinite(pitchRadians)) {
        return;
    }
    m_camera->camera.SetYawPitch(yawRadians, pitchRadians);
}

Math::Vector3UVE EditorViewportCameraUVE::GetTargetUVE() const noexcept {
    return ToEngineVector3UVE(m_camera->camera.Target());
}

float EditorViewportCameraUVE::GetYawUVE() const noexcept { return m_camera->camera.Yaw(); }
float EditorViewportCameraUVE::GetPitchUVE() const noexcept { return m_camera->camera.Pitch(); }
float EditorViewportCameraUVE::GetDistanceUVE() const noexcept { return m_camera->camera.Distance(); }

Math::Vector3UVE EditorViewportCameraUVE::GetEyeUVE() const noexcept {
    return ToEngineVector3UVE(m_camera->camera.Eye());
}

float EditorViewportCameraUVE::GetNearPlaneUVE() const noexcept { return m_camera->camera.NearPlane(); }
float EditorViewportCameraUVE::GetFarPlaneUVE() const noexcept { return m_camera->camera.FarPlane(); }

void EditorViewportCameraUVE::SetOrthographicUVE(const bool orthographic) noexcept {
    m_camera->camera.SetOrthographic(orthographic);
}

bool EditorViewportCameraUVE::IsOrthographicUVE() const noexcept {
    return m_camera->camera.IsOrthographic();
}

float EditorViewportCameraUVE::GetOrthographicHalfHeightUVE() const noexcept {
    return m_camera->camera.OrthographicHalfHeight();
}

void EditorViewportCameraUVE::SnapToDirectionUVE(const Math::Vector3UVE& worldDirection) noexcept {
    if (Math::LengthSquaredUVE(worldDirection) <= 0.0F) {
        return;
    }
    m_camera->camera.SnapToDirection(ToUnivexVec3UVE(worldDirection));
}

void EditorViewportCameraUVE::SnapToYawPitchUVE(const float yawRadians, const float pitchRadians) noexcept {
    if (!std::isfinite(yawRadians) || !std::isfinite(pitchRadians)) {
        return;
    }
    m_camera->camera.SnapToYawPitch(yawRadians, pitchRadians);
}

bool EditorViewportCameraUVE::UpdateUVE(const float deltaSeconds) noexcept {
    if (!std::isfinite(deltaSeconds)) {
        return m_camera->camera.IsAnimating();
    }
    return m_camera->camera.Update(deltaSeconds);
}

void EditorViewportCameraUVE::CancelAnimationUVE() noexcept { m_camera->camera.CancelAnimation(); }
bool EditorViewportCameraUVE::IsAnimatingUVE() const noexcept { return m_camera->camera.IsAnimating(); }

void EditorViewportCameraUVE::FocusUVE(const Math::Vector3UVE& point, const float radius) noexcept {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
        return;
    }
    m_camera->camera.Focus(ToUnivexVec3UVE(point), radius);
}

Math::QuaternionUVE EditorViewportCameraUVE::GetRotationUVE() const noexcept {
    // univex::camera::OrbitCamera has no notion of a quaternion (LookAt needs none) - this engine's
    // ECS camera representation (Scene::TransformComponentUVE) is position+rotation, so this is the
    // one place that needs one. Solving `R * (0,0,-1) == forward` for this orbit's forward
    // direction gives R = Ry(pi/2 - yaw) * Rx(-pitch); this is hand-verified to reproduce
    // OrbitCamera::Eye()/ViewMatrix()'s own forward vector exactly for every yaw/pitch, so pushing
    // this quaternion onto the ECS transform reproduces this camera's actual orientation exactly.
    const float yaw = m_camera->camera.Yaw();
    const float pitch = m_camera->camera.Pitch();
    Math::QuaternionUVE yawRotation{};
    Math::QuaternionUVE pitchRotation{};
    if (!Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 1.0F, 0.0F}, (kPiUVE * 0.5F) - yaw, yawRotation) ||
        !Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F}, -pitch, pitchRotation)) {
        return Math::QuaternionUVE{};
    }
    return Math::MultiplyUVE(yawRotation, pitchRotation);
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetViewMatrixUVE() const noexcept {
    return ToEngineMatrixUVE(m_camera->camera.ViewMatrix());
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetProjectionMatrixUVE(const float aspectRatio) const noexcept {
    const float safeAspect = (std::isfinite(aspectRatio) && aspectRatio > 0.0F) ? aspectRatio : 1.0F;
    return ToEngineMatrixUVE(m_camera->camera.ProjectionMatrix(safeAspect));
}

Math::Matrix4x4UVE EditorViewportCameraUVE::GetViewProjectionUVE(const float aspectRatio) const noexcept {
    const float safeAspect = (std::isfinite(aspectRatio) && aspectRatio > 0.0F) ? aspectRatio : 1.0F;
    return ToEngineMatrixUVE(m_camera->camera.ViewProjection(safeAspect));
}

bool EditorViewportCameraUVE::TryGetInverseViewProjectionUVE(
    const float aspectRatio, Math::Matrix4x4UVE& outInverse) const noexcept {
    const float safeAspect = (std::isfinite(aspectRatio) && aspectRatio > 0.0F) ? aspectRatio : 1.0F;
    const auto inverse = univex::math::Mat4::Inverse(m_camera->camera.ViewProjection(safeAspect));
    if (!inverse.has_value()) {
        return false;
    }
    outInverse = ToEngineMatrixUVE(*inverse);
    return true;
}

} // namespace UVE::Editor
