// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Tunables of the editor viewport's orbit camera.
///
/// Dynamic clip planes: near and far are derived from the current orbit distance rather than
/// fixed. A fixed `near=0.05 / far=20000` pair looks fine at one zoom level and falls apart at
/// the others - at 4 km out the depth buffer has almost no precision left, and at 5 cm in the
/// near plane clips through everything. Scaling both with distance keeps the near:far ratio (and
/// therefore depth precision) roughly constant across the whole zoom range, which is what makes a
/// viewport spanning centimetres to kilometres usable.
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

/// The editor viewport's orbit camera: yaw/pitch/distance around a pivot, with a real projection
/// switch, dynamic clip planes, and eased axis snapping.
///
/// ORBIT CONVENTION (the fix for the previously inverted viewport, asserted by
/// `tests/editor/viewport/editor_viewport_camera_uve_tests.cpp`): this is a turntable orbit in
/// which the scene follows the cursor. Pointer deltas are passed in screen pixels, with `+dy`
/// meaning the pointer moved DOWN the screen, matching ImGui/GLFW.
///   - Drag right (`+dx`): world points sweep RIGHT across the screen; the camera orbits left.
///   - Drag down  (`+dy`): the camera rises and looks further down onto the scene, so a point
///     above the pivot sweeps DOWN the screen.
/// Both axes therefore move scene content the same way the pointer moves, which is the standard
/// editor tumble. A caller must not pre-negate its deltas.
///
/// The camera is editor-only presentation state: it owns no document entity, is never serialized
/// into a `.uvescene`, and never enters the undo/redo history. `GetRotationUVE()` exists so the
/// editor can push this orientation onto the ECS camera entity the engine renders the scene from
/// - the one bridge between this class and the scene.
class EditorViewportCameraUVE final {
public:
    EditorViewportCameraUVE() = default;
    explicit EditorViewportCameraUVE(const EditorViewportCameraSettingsUVE& settings) noexcept
        : m_settings(settings) {}

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

    void SetTargetUVE(const Math::Vector3UVE& target) noexcept { m_target = target; }
    void SetDistanceUVE(float distance) noexcept;
    void SetYawPitchUVE(float yawRadians, float pitchRadians) noexcept;

    // ---- state ---------------------------------------------------------------------------------

    [[nodiscard]] Math::Vector3UVE GetTargetUVE() const noexcept { return m_target; }
    [[nodiscard]] float GetYawUVE() const noexcept { return m_yawRadians; }
    [[nodiscard]] float GetPitchUVE() const noexcept { return m_pitchRadians; }
    [[nodiscard]] float GetDistanceUVE() const noexcept { return m_distance; }
    [[nodiscard]] const EditorViewportCameraSettingsUVE& GetSettingsUVE() const noexcept {
        return m_settings;
    }

    /// World-space camera position.
    [[nodiscard]] Math::Vector3UVE GetEyeUVE() const noexcept;

    [[nodiscard]] float GetNearPlaneUVE() const noexcept;
    [[nodiscard]] float GetFarPlaneUVE() const noexcept;

    // ---- projection ----------------------------------------------------------------------------

    void SetOrthographicUVE(const bool orthographic) noexcept { m_orthographic = orthographic; }
    [[nodiscard]] bool IsOrthographicUVE() const noexcept { return m_orthographic; }

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
    void CancelAnimationUVE() noexcept { m_animating = false; }
    [[nodiscard]] bool IsAnimatingUVE() const noexcept { return m_animating; }

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

    /// World-to-view. Built through ViewFromPositionAndRotationUVE(GetEyeUVE(), GetRotationUVE()),
    /// so the grid/gizmo overlays and the engine's scene render cannot drift apart.
    [[nodiscard]] Math::Matrix4x4UVE GetViewMatrixUVE() const noexcept;

    /// View-to-clip. A genuinely different matrix per projection mode - perspective uses the
    /// settings FOV with dynamic clip planes, orthographic uses GetOrthographicHalfHeightUVE()
    /// with a symmetric depth range centred on the pivot, so geometry nearer than the pivot
    /// survives clipping. Both are the engine's Y-up, `[0, 1]` depth convention.
    [[nodiscard]] Math::Matrix4x4UVE GetProjectionMatrixUVE(float aspectRatio) const noexcept;

    [[nodiscard]] Math::Matrix4x4UVE GetViewProjectionUVE(float aspectRatio) const noexcept;

    /// The inverse view-projection the infinite grid needs to rebuild a world ray per pixel.
    /// Returns false (leaving `outInverse` untouched) for a singular matrix rather than letting a
    /// non-finite uniform reach the driver.
    [[nodiscard]] bool TryGetInverseViewProjectionUVE(float aspectRatio,
                                                      Math::Matrix4x4UVE& outInverse) const noexcept;

private:
    EditorViewportCameraSettingsUVE m_settings{};
    Math::Vector3UVE m_target{0.0F, 0.0F, 0.0F};

    /// Default framing, preserved exactly from the viewport foundation this camera was ported
    /// from: a three-quarter view from above where all three axes are visible and none is
    /// foreshortened into another. Deliberately not recomputed or auto-framed at startup.
    float m_yawRadians = -0.7553F;
    float m_pitchRadians = 0.4561F;
    float m_distance = 11.26F;
    bool m_orthographic = false;

    bool m_animating = false;
    float m_fromYawRadians = 0.0F;
    float m_fromPitchRadians = 0.0F;
    float m_deltaYawRadians = 0.0F;
    float m_deltaPitchRadians = 0.0F;
    float m_elapsedSeconds = 0.0F;
    float m_durationSeconds = 0.0F;
};

} // namespace UVE::Editor
