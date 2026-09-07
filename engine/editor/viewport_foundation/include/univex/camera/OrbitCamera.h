// univex/camera/OrbitCamera.h
// -----------------------------------------------------------------------
// A real editor-viewport orbit camera: perspective projection, look-at
// view matrix, orbit / pan / dolly, and the inverse view-projection the
// infinite grid needs to rebuild a world ray per pixel.
//
// Dynamic clip planes: near and far are derived from the current orbit
// distance rather than fixed. A fixed near=0.05 / far=20000 pair looks
// fine at one zoom level and falls apart at the others — at 4 km out the
// depth buffer has almost no precision left, and at 5 cm in the near
// plane clips through everything. Scaling both with distance keeps the
// near:far ratio (and therefore depth precision) roughly constant across
// the whole zoom range, which is what makes a grid that spans centimetres
// to kilometres actually usable.
// -----------------------------------------------------------------------
#pragma once

#include "univex/math/Mat4.h"
#include "univex/math/Vec.h"

namespace univex::camera {

using univex::math::Mat4;
using univex::math::Vec3;

struct OrbitCameraSettings {
    float fovYRadians = 50.f * 3.14159265358979323846f / 180.f;

    float pitchMin = -1.5533f; // ~ -89 degrees; stops short of the pole so `up` never degenerates
    float pitchMax = 1.5533f;

    float distanceMin = 0.02f;
    float distanceMax = 50000.f;

    float orbitRadiansPerPixel = 0.0055f;
    float dollyPerWheelNotch = 0.12f;  // exponential: distance *= exp(-notches * this)

    // Orthographic half-height is derived from the orbit distance so that
    // toggling projection does not change how big anything looks — the
    // ortho view is framed to match what the perspective view had at the
    // pivot's depth.
    int snapDurationMs = 280;

    // Dynamic clip planes, as multiples of the current orbit distance.
    float nearPlaneScale = 0.002f;
    float farPlaneScale = 2000.f;
    float nearPlaneMin = 0.001f;
};

class OrbitCamera {
public:
    OrbitCamera() = default;
    explicit OrbitCamera(const OrbitCameraSettings& settings) : settings_(settings) {}

    // ---- input ----------------------------------------------------------
    // Pixel deltas from a drag. Pitch is clamped to the settings range.
    void Orbit(float dxPixels, float dyPixels);

    // Slides the pivot in the camera's own right/up plane. The world
    // distance per pixel is derived from the orbit distance and vertical
    // FOV, so a drag moves whatever is under the cursor by roughly that
    // many pixels at any zoom level.
    void Pan(float dxPixels, float dyPixels, int viewportHeightPixels);

    // Exponential dolly; `notches` is positive to move closer.
    void Dolly(float notches);

    void SetTarget(const Vec3& target) { target_ = target; }
    void SetDistance(float distance);
    void SetYawPitch(float yaw, float pitch);

    // ---- state ----------------------------------------------------------
    [[nodiscard]] Vec3 Target() const { return target_; }
    [[nodiscard]] float Yaw() const { return yaw_; }
    [[nodiscard]] float Pitch() const { return pitch_; }
    [[nodiscard]] float Distance() const { return distance_; }
    [[nodiscard]] const OrbitCameraSettings& Settings() const { return settings_; }

    // World-space camera position.
    [[nodiscard]] Vec3 Eye() const;

    [[nodiscard]] float NearPlane() const;
    [[nodiscard]] float FarPlane() const;

    // ---- projection mode ------------------------------------------------
    void SetOrthographic(bool orthographic) { orthographic_ = orthographic; }
    [[nodiscard]] bool IsOrthographic() const { return orthographic_; }

    // Half the vertical extent the orthographic view covers, chosen so a
    // projection toggle does not change the apparent size of anything at
    // the pivot's depth.
    [[nodiscard]] float OrthographicHalfHeight() const;

    // ---- animated snap ---------------------------------------------------
    // Eases to the yaw/pitch that looks at the pivot from along
    // `worldDirection` — what a nav-gizmo click or a standard-view shortcut
    // triggers. Non-blocking; drive it with Update().
    void SnapToDirection(const Vec3& worldDirection);
    void SnapToYawPitch(float yaw, float pitch);

    // Advances any in-flight snap. Returns true while still animating.
    bool Update(float deltaSeconds);
    void CancelAnimation() { animating_ = false; }
    [[nodiscard]] bool IsAnimating() const { return animating_; }

    // Frames a point: keeps the current angles, moves the pivot there, and
    // pulls the distance in to `radius`-ish framing.
    void Focus(const Vec3& point, float radius);

    // ---- matrices -------------------------------------------------------
    [[nodiscard]] Mat4 ViewMatrix() const;
    [[nodiscard]] Mat4 ProjectionMatrix(float aspect) const;
    [[nodiscard]] Mat4 ViewProjection(float aspect) const;

    // Inverse of ViewProjection(). Falls back to the identity if the
    // matrix is singular (which shouldn't happen for a sane camera, but a
    // NaN uniform would blow up the whole frame, so it's guarded).
    [[nodiscard]] Mat4 InverseViewProjection(float aspect) const;

private:
    OrbitCameraSettings settings_{};
    Vec3 target_{0.f, 0.f, 0.f};
    // Default framing, matched to the reference viewport's opening view: a
    // three-quarter from above where all three axes are visible and none is
    // foreshortened into another. The reference is a Z-up editor sitting at
    // about (7.36, -6.93, 4.96) looking at the origin; mapped into this
    // engine's Y-up basis that is the yaw/pitch/distance below.
    float yaw_ = -0.7553f;   // radians, around world +Y  (-43.3 degrees)
    float pitch_ = 0.4561f;  // radians, positive looks down at the ground (26.1 degrees)
    float distance_ = 11.26f;
    bool orthographic_ = false;

    bool animating_ = false;
    float fromYaw_ = 0.f, fromPitch_ = 0.f;
    float deltaYaw_ = 0.f, deltaPitch_ = 0.f;
    float elapsedSeconds_ = 0.f, durationSeconds_ = 0.f;
};

} // namespace univex::camera
