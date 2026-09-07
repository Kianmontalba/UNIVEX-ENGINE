// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Tunables of the editor viewport's orbit camera. Mirrors
/// `univex::camera::OrbitCameraSettings` field-for-field (see editor_viewport_camera_uve.cpp) -
/// kept as this engine's own `Math::` type so no public header here has to include the vendored
/// module's headers.
struct EditorViewportCameraSettingsUVE final {
    float fieldOfViewYRadians = 0.8726646F; // 50 degrees

    /// Just short of the pole, so the view basis never degenerates against world up.
    float pitchMinRadians = -1.5533F;
    float pitchMaxRadians = 1.5533F;

    float distanceMin = 0.02F;
    float distanceMax = 50000.0F;

    float orbitRadiansPerPixel = 0.0055F;
    /// Exponential dolly: `distance *= exp(-notches * dollyPerWheelNotch)`.
    float dollyPerWheelNotch = 0.12F;

    int snapDurationMilliseconds = 280;

    float nearPlaneScale = 0.002F;
    float farPlaneScale = 2000.0F;
    float nearPlaneMin = 0.001F;
};

/// The editor viewport's orbit camera. This class is a thin adapter: every field of camera state
/// and every piece of orbit/pan/dolly/snap/projection math lives in and is computed by
/// `univex::camera::OrbitCamera` (engine/editor/viewport_foundation/include/univex/camera/
/// OrbitCamera.h) - the camera from the univex_viewport_gl foundation package, vendored
/// essentially unmodified. This class exists only to translate at the boundary: `Math::Vector3UVE`
/// <-> `univex::math::Vec3`, `Math::Matrix4x4UVE` (row-major, `[0, 1]` clip-space z) <->
/// `univex::math::Mat4` (column-major - the projection z-row is UVE-adapted for the `[0, 1]`
/// convention in Mat4.cpp, everything else in that module is untouched), and one
/// yaw/pitch-to-quaternion conversion for `GetRotationUVE()`, needed only because this engine's
/// ECS camera representation is transform-based (position + rotation) rather than raw-matrix-based
/// - `univex::camera::OrbitCamera` itself has no notion of a quaternion.
///
/// ORBIT CONVENTION: a turntable orbit in which the scene follows the cursor. Pointer deltas are
/// passed in screen pixels, with `+dy` meaning the pointer moved DOWN the screen, matching
/// ImGui/GLFW.
///   - Drag right (`+dx`): world points sweep RIGHT across the screen; the camera orbits left.
///   - Drag down  (`+dy`): the camera rises and looks further down onto the scene, so a point
///     above the pivot sweeps DOWN the screen.
///
/// The camera is editor-only presentation state: it owns no document entity, is never serialized
/// into a `.uvescene`, and never enters the undo/redo history. `GetRotationUVE()` exists so the
/// editor can push this orientation onto the ECS camera entity the engine renders the scene from -
/// the one bridge between this class and the scene.
class EditorViewportCameraUVE final {
public:
    EditorViewportCameraUVE();
    explicit EditorViewportCameraUVE(const EditorViewportCameraSettingsUVE& settings) noexcept;
    ~EditorViewportCameraUVE();

    EditorViewportCameraUVE(const EditorViewportCameraUVE&) = delete;
    EditorViewportCameraUVE& operator=(const EditorViewportCameraUVE&) = delete;
    EditorViewportCameraUVE(EditorViewportCameraUVE&&) noexcept;
    EditorViewportCameraUVE& operator=(EditorViewportCameraUVE&&) noexcept;

    // ---- input ---------------------------------------------------------------------------------

    /// Orbits by a pointer drag in pixels. Pitch is clamped to the settings range. A manual drag
    /// always cancels an in-flight snap animation.
    void OrbitUVE(float deltaXPixels, float deltaYPixels) noexcept;

    /// Slides the pivot in the camera's own right/up plane. The world distance per pixel is
    /// derived from the orbit distance and vertical FOV, so a drag moves whatever is under the
    /// cursor by roughly that many pixels at any zoom level.
    void PanUVE(float deltaXPixels, float deltaYPixels, int viewportHeightPixels) noexcept;

    /// Exponential dolly; `notches` is positive to move closer.
    void DollyUVE(float notches) noexcept;

    void SetTargetUVE(const Math::Vector3UVE& target) noexcept;
    void SetDistanceUVE(float distance) noexcept;
    void SetYawPitchUVE(float yawRadians, float pitchRadians) noexcept;

    // ---- state ---------------------------------------------------------------------------------

    [[nodiscard]] Math::Vector3UVE GetTargetUVE() const noexcept;
    [[nodiscard]] float GetYawUVE() const noexcept;
    [[nodiscard]] float GetPitchUVE() const noexcept;
    [[nodiscard]] float GetDistanceUVE() const noexcept;
    [[nodiscard]] const EditorViewportCameraSettingsUVE& GetSettingsUVE() const noexcept {
        return m_settings;
    }

    /// World-space camera position.
    [[nodiscard]] Math::Vector3UVE GetEyeUVE() const noexcept;

    [[nodiscard]] float GetNearPlaneUVE() const noexcept;
    [[nodiscard]] float GetFarPlaneUVE() const noexcept;

    // ---- projection ----------------------------------------------------------------------------

    void SetOrthographicUVE(bool orthographic) noexcept;
    [[nodiscard]] bool IsOrthographicUVE() const noexcept;

    /// Half the vertical extent the orthographic view covers, chosen so a projection toggle does
    /// not change the apparent size of anything at the pivot's depth.
    [[nodiscard]] float GetOrthographicHalfHeightUVE() const noexcept;

    // ---- animated snap -------------------------------------------------------------------------

    /// Eases to the yaw/pitch that looks at the pivot from along `worldDirection` - what a
    /// nav-gizmo click or a standard-view shortcut triggers. Non-blocking; drive it with
    /// UpdateUVE().
    void SnapToDirectionUVE(const Math::Vector3UVE& worldDirection) noexcept;
    void SnapToYawPitchUVE(float yawRadians, float pitchRadians) noexcept;

    /// Advances any in-flight snap. Returns true while still animating.
    bool UpdateUVE(float deltaSeconds) noexcept;
    void CancelAnimationUVE() noexcept;
    [[nodiscard]] bool IsAnimatingUVE() const noexcept;

    /// Frames a point: keeps the current angles, moves the pivot there, and pulls the distance in
    /// to fit a sphere of `radius`. A non-positive radius moves the pivot only.
    void FocusUVE(const Math::Vector3UVE& point, float radius) noexcept;

    // ---- matrices ------------------------------------------------------------------------------

    /// The camera orientation as a rotation, in exactly the convention
    /// `Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE` and `Scene::WorldTransformComponentUVE`
    /// use: it maps `(0,0,-1)` onto the camera forward direction. Pushing this plus GetEyeUVE()
    /// onto the ECS camera entity makes the engine's own renderer agree with this class's
    /// matrices exactly.
    [[nodiscard]] Math::QuaternionUVE GetRotationUVE() const noexcept;

    /// World-to-view, converted directly from `univex::camera::OrbitCamera::ViewMatrix()` (its own
    /// `LookAt`), not re-derived from GetRotationUVE() - the two are algebraically equivalent for
    /// this orbit formula, but this keeps the actual rendered value tied to the vendored camera's
    /// own math with no intermediate step.
    [[nodiscard]] Math::Matrix4x4UVE GetViewMatrixUVE() const noexcept;

    /// View-to-clip, converted directly from `univex::camera::OrbitCamera::ProjectionMatrix()`. A
    /// genuinely different matrix per projection mode - perspective uses the settings FOV with
    /// dynamic clip planes, orthographic uses GetOrthographicHalfHeightUVE() with a symmetric depth
    /// range centred on the pivot, so geometry nearer than the pivot survives clipping.
    [[nodiscard]] Math::Matrix4x4UVE GetProjectionMatrixUVE(float aspectRatio) const noexcept;

    [[nodiscard]] Math::Matrix4x4UVE GetViewProjectionUVE(float aspectRatio) const noexcept;

    /// The inverse view-projection the infinite grid needs to rebuild a world ray per pixel.
    /// Returns false (leaving `outInverse` untouched) for a singular matrix rather than letting a
    /// non-finite uniform reach the driver.
    [[nodiscard]] bool TryGetInverseViewProjectionUVE(float aspectRatio,
                                                      Math::Matrix4x4UVE& outInverse) const noexcept;

private:
    EditorViewportCameraSettingsUVE m_settings{};

    // The vendored univex::camera::OrbitCamera, PIMPL'd so this public header never has to include
    // engine/editor/viewport_foundation/include/univex/camera/OrbitCamera.h - matching this
    // module's existing third-party-header confinement discipline (see editor/CMakeLists.txt's
    // ImGui precedent).
    struct CameraImplUVE;
    std::unique_ptr<CameraImplUVE> m_camera;
};

} // namespace UVE::Editor
