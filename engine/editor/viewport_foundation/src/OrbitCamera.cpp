#include "univex/camera/OrbitCamera.h"

#include <algorithm>
#include <cmath>

namespace univex::camera {

using univex::math::Cross;
using univex::math::Normalize;

namespace {
constexpr Vec3 kWorldUp{0.f, 1.f, 0.f};
} // namespace

namespace {

// Shortest signed angular distance, wrapped into (-pi, pi], so a snap
// always turns the short way round instead of unwinding past 180 degrees.
float WrapAngleDelta(float delta) {
    constexpr float kPi = 3.14159265358979323846f;
    float wrapped = std::fmod(delta + kPi, 2.f * kPi);
    if (wrapped < 0.f) wrapped += 2.f * kPi;
    return wrapped - kPi;
}

float EaseOutCubic(float t) {
    const float inverse = 1.f - t;
    return 1.f - inverse * inverse * inverse;
}

} // namespace

void OrbitCamera::Orbit(float dxPixels, float dyPixels) {
    animating_ = false; // a manual drag always wins over an in-flight snap
    yaw_ += dxPixels * settings_.orbitRadiansPerPixel;
    pitch_ = std::clamp(pitch_ + dyPixels * settings_.orbitRadiansPerPixel,
                        settings_.pitchMin, settings_.pitchMax);
}

void OrbitCamera::Pan(float dxPixels, float dyPixels, int viewportHeightPixels) {
    if (viewportHeightPixels <= 0) return;

    // World units covered by the full viewport height at the pivot's depth.
    const float worldPerPixel =
        (2.f * distance_ * std::tan(settings_.fovYRadians * 0.5f)) / static_cast<float>(viewportHeightPixels);

    const Vec3 forward = Normalize(target_ - Eye());
    const Vec3 right = Normalize(Cross(forward, kWorldUp));
    const Vec3 up = Cross(right, forward);

    target_ += right * (-dxPixels * worldPerPixel);
    target_ += up * (dyPixels * worldPerPixel);
}

void OrbitCamera::Dolly(float notches) {
    SetDistance(distance_ * std::exp(-notches * settings_.dollyPerWheelNotch));
}

void OrbitCamera::SetDistance(float distance) {
    distance_ = std::clamp(distance, settings_.distanceMin, settings_.distanceMax);
}

void OrbitCamera::SetYawPitch(float yaw, float pitch) {
    yaw_ = yaw;
    pitch_ = std::clamp(pitch, settings_.pitchMin, settings_.pitchMax);
}

Vec3 OrbitCamera::Eye() const {
    const float cosPitch = std::cos(pitch_);
    const Vec3 offset{
        std::cos(yaw_) * cosPitch,
        std::sin(pitch_),
        std::sin(yaw_) * cosPitch,
    };
    return target_ + offset * distance_;
}

float OrbitCamera::NearPlane() const {
    return std::max(settings_.nearPlaneMin, distance_ * settings_.nearPlaneScale);
}

float OrbitCamera::FarPlane() const {
    return distance_ * settings_.farPlaneScale;
}

Mat4 OrbitCamera::ViewMatrix() const {
    return Mat4::LookAt(Eye(), target_, kWorldUp);
}

float OrbitCamera::OrthographicHalfHeight() const {
    return distance_ * std::tan(settings_.fovYRadians * 0.5f);
}

void OrbitCamera::SnapToDirection(const Vec3& worldDirection) {
    const Vec3 direction = Normalize(worldDirection);
    const float horizontal = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    const float targetYaw = std::atan2(direction.z, direction.x);
    // atan2(y, horizontal) is +-pi/2 at the poles; the clamp keeps `up` from
    // degenerating exactly on-axis for Top and Bottom.
    const float targetPitch = std::clamp(std::atan2(direction.y, horizontal),
                                         settings_.pitchMin, settings_.pitchMax);
    SnapToYawPitch(targetYaw, targetPitch);
}

void OrbitCamera::SnapToYawPitch(float yaw, float pitch) {
    fromYaw_ = yaw_;
    fromPitch_ = pitch_;
    deltaYaw_ = WrapAngleDelta(yaw - yaw_);
    deltaPitch_ = std::clamp(pitch, settings_.pitchMin, settings_.pitchMax) - pitch_;
    elapsedSeconds_ = 0.f;
    durationSeconds_ = std::max(0.001f, static_cast<float>(settings_.snapDurationMs) / 1000.f);
    animating_ = true;
}

bool OrbitCamera::Update(float deltaSeconds) {
    if (!animating_) return false;
    elapsedSeconds_ += deltaSeconds;
    const float t = std::min(1.f, elapsedSeconds_ / durationSeconds_);
    const float eased = EaseOutCubic(t);
    yaw_ = fromYaw_ + deltaYaw_ * eased;
    pitch_ = std::clamp(fromPitch_ + deltaPitch_ * eased, settings_.pitchMin, settings_.pitchMax);
    if (t >= 1.f) animating_ = false;
    return animating_;
}

void OrbitCamera::Focus(const Vec3& point, float radius) {
    target_ = point;
    if (radius > 0.f) {
        // Pull back far enough that a sphere of `radius` fits the vertical FOV.
        SetDistance(radius / std::max(0.05f, std::sin(settings_.fovYRadians * 0.5f)));
    }
}

Mat4 OrbitCamera::ProjectionMatrix(float aspect) const {
    if (orthographic_) {
        // The ortho volume is centred on the pivot, so half the depth range
        // has to sit behind the camera for anything nearer than the pivot to
        // survive clipping.
        const float halfDepth = std::max(distance_ * 4.f, FarPlane() * 0.5f);
        return Mat4::Orthographic(OrthographicHalfHeight(), aspect, -halfDepth, halfDepth);
    }
    return Mat4::Perspective(settings_.fovYRadians, aspect, NearPlane(), FarPlane());
}

Mat4 OrbitCamera::ViewProjection(float aspect) const {
    return Mat4::Multiply(ProjectionMatrix(aspect), ViewMatrix());
}

Mat4 OrbitCamera::InverseViewProjection(float aspect) const {
    const auto inverse = Mat4::Inverse(ViewProjection(aspect));
    return inverse.value_or(Mat4::Identity());
}

} // namespace univex::camera
