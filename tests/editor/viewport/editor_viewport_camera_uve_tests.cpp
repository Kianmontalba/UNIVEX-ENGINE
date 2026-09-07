// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "uve/editor/viewport/editor_viewport_camera_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

namespace {

/// Projects a world point through `viewProjection` into normalized device coordinates, matching
/// what the GPU does: multiply, then perspective divide. NDC is Y-up (+1 is the top of the
/// screen), so a point moving "down the screen" means its `y` decreases.
struct NdcPointUVE final {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 0.0F;
};

[[nodiscard]] NdcPointUVE ProjectUVE(const Math::Matrix4x4UVE& viewProjection,
                                     const Math::Vector3UVE& point) {
    const float clipX = (viewProjection.m[0][0] * point.x) + (viewProjection.m[0][1] * point.y) +
                        (viewProjection.m[0][2] * point.z) + viewProjection.m[0][3];
    const float clipY = (viewProjection.m[1][0] * point.x) + (viewProjection.m[1][1] * point.y) +
                        (viewProjection.m[1][2] * point.z) + viewProjection.m[1][3];
    const float clipZ = (viewProjection.m[2][0] * point.x) + (viewProjection.m[2][1] * point.y) +
                        (viewProjection.m[2][2] * point.z) + viewProjection.m[2][3];
    const float clipW = (viewProjection.m[3][0] * point.x) + (viewProjection.m[3][1] * point.y) +
                        (viewProjection.m[3][2] * point.z) + viewProjection.m[3][3];
    if (std::fabs(clipW) < 1e-8F) {
        return NdcPointUVE{0.0F, 0.0F, 0.0F, clipW};
    }
    return NdcPointUVE{clipX / clipW, clipY / clipW, clipZ / clipW, clipW};
}

constexpr float kAspectUVE = 16.0F / 9.0F;

} // namespace

TEST(EditorViewportCameraUVETest, DefaultFramingMatchesTheViewportFoundationAndIsPerspective) {
    const EditorViewportCameraUVE camera;

    // The foundation's opening three-quarter view, preserved exactly. These are deliberately not
    // recomputed or auto-framed at startup.
    EXPECT_NEAR(camera.GetYawUVE(), -0.7553F, 1e-6F);
    EXPECT_NEAR(camera.GetPitchUVE(), 0.4561F, 1e-6F);
    EXPECT_NEAR(camera.GetDistanceUVE(), 11.26F, 1e-6F);
    EXPECT_FALSE(camera.IsOrthographicUVE());
    EXPECT_EQ(camera.GetTargetUVE(), (Math::Vector3UVE{0.0F, 0.0F, 0.0F}));

    // Positive pitch looks down at the ground, so the default eye sits above the pivot and all
    // three axes stay visible.
    EXPECT_GT(camera.GetEyeUVE().y, 0.0F);
}

TEST(EditorViewportCameraUVETest, OrbitUVE_DraggingRightSweepsSceneContentRightNotInverted) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F);

    // A world point out along +X. At yaw 0 the camera sits on +X looking back at the origin, so
    // this point starts centred horizontally.
    const Math::Vector3UVE probe{1.0F, 0.0F, 0.0F};
    const float before = ProjectUVE(camera.GetViewProjectionUVE(kAspectUVE), probe).x;
    EXPECT_NEAR(before, 0.0F, 1e-5F);

    camera.OrbitUVE(40.0F, 0.0F); // drag right
    const float after = ProjectUVE(camera.GetViewProjectionUVE(kAspectUVE), probe).x;

    // Scene content follows the cursor: dragging right sweeps the point right across the screen.
    // A negative value here is the classic "inverted viewport" this convention exists to prevent.
    EXPECT_GT(after, before);
}

TEST(EditorViewportCameraUVETest, OrbitUVE_DraggingDownSweepsSceneContentDownNotInverted) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F);

    // A world point straight above the pivot.
    const Math::Vector3UVE probe{0.0F, 1.0F, 0.0F};
    const float before = ProjectUVE(camera.GetViewProjectionUVE(kAspectUVE), probe).y;
    EXPECT_GT(before, 0.0F);

    camera.OrbitUVE(0.0F, 40.0F); // drag down (screen Y grows downward)
    const float after = ProjectUVE(camera.GetViewProjectionUVE(kAspectUVE), probe).y;

    // The camera rises and looks further down, so the point above the pivot sweeps down-screen
    // with the cursor. NDC is Y-up, so "down" means a smaller y.
    EXPECT_LT(after, before);
    EXPECT_GT(camera.GetEyeUVE().y, 0.0F);
}

TEST(EditorViewportCameraUVETest, OrbitUVE_ClampsPitchAtBothPolesAndCancelsAnimation) {
    EditorViewportCameraUVE camera;

    camera.OrbitUVE(0.0F, 100000.0F);
    EXPECT_NEAR(camera.GetPitchUVE(), camera.GetSettingsUVE().pitchMaxRadians, 1e-6F);

    camera.OrbitUVE(0.0F, -100000.0F);
    EXPECT_NEAR(camera.GetPitchUVE(), camera.GetSettingsUVE().pitchMinRadians, 1e-6F);

    // A manual drag always wins over an in-flight snap.
    camera.SnapToDirectionUVE(Math::Vector3UVE{0.0F, 1.0F, 0.0F});
    ASSERT_TRUE(camera.IsAnimatingUVE());
    camera.OrbitUVE(1.0F, 0.0F);
    EXPECT_FALSE(camera.IsAnimatingUVE());
}

TEST(EditorViewportCameraUVETest, OrbitUVE_RejectsNonFiniteDeltasWithoutMutation) {
    EditorViewportCameraUVE camera;
    const float yawBefore = camera.GetYawUVE();
    const float pitchBefore = camera.GetPitchUVE();

    camera.OrbitUVE(std::numeric_limits<float>::quiet_NaN(), 0.0F);
    camera.OrbitUVE(0.0F, std::numeric_limits<float>::infinity());

    EXPECT_EQ(camera.GetYawUVE(), yawBefore);
    EXPECT_EQ(camera.GetPitchUVE(), pitchBefore);
}

TEST(EditorViewportCameraUVETest, DollyUVE_MovesCloserOnPositiveNotchesAndClampsToRange) {
    EditorViewportCameraUVE camera;
    const float start = camera.GetDistanceUVE();

    camera.DollyUVE(1.0F);
    EXPECT_LT(camera.GetDistanceUVE(), start);

    camera.DollyUVE(-1.0F);
    EXPECT_NEAR(camera.GetDistanceUVE(), start, 1e-3F);

    camera.DollyUVE(100000.0F);
    EXPECT_NEAR(camera.GetDistanceUVE(), camera.GetSettingsUVE().distanceMin, 1e-6F);

    camera.DollyUVE(-100000.0F);
    EXPECT_NEAR(camera.GetDistanceUVE(), camera.GetSettingsUVE().distanceMax, 1e-6F);
}

TEST(EditorViewportCameraUVETest, PanUVE_SlidesPivotSoContentFollowsTheCursor) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F); // camera on +X looking toward -X; screen right is -Z
    const Math::Vector3UVE before = camera.GetTargetUVE();

    camera.PanUVE(10.0F, 0.0F, 900);
    const Math::Vector3UVE afterHorizontal = camera.GetTargetUVE();
    // Dragging right slides the pivot left in camera space, which is what makes the content under
    // the cursor travel with it.
    EXPECT_NE(afterHorizontal, before);

    camera.SetTargetUVE(before);
    camera.PanUVE(0.0F, 10.0F, 900);
    // Dragging down lifts the pivot, so the scene slides down the screen with the pointer.
    EXPECT_GT(camera.GetTargetUVE().y, before.y);

    // A degenerate viewport height is a no-op rather than a divide by zero.
    camera.SetTargetUVE(before);
    camera.PanUVE(10.0F, 10.0F, 0);
    EXPECT_EQ(camera.GetTargetUVE(), before);
}

TEST(EditorViewportCameraUVETest, GetRotationUVE_AgreesWithTheOrbitBasisSoTheEcsCameraMatches) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(-0.9F, 0.35F);

    const Math::Vector3UVE eye = camera.GetEyeUVE();
    const Math::Vector3UVE expectedForward = Math::NormalizeUVE(camera.GetTargetUVE() - eye);
    const Math::Vector3UVE rotatedForward =
        Math::RotateVectorUVE(camera.GetRotationUVE(), Math::Vector3UVE{0.0F, 0.0F, -1.0F});

    // This is the exact contract ViewFromPositionAndRotationUVE and WorldTransformComponentUVE
    // use, so writing GetRotationUVE() onto the ECS camera entity makes the engine's own scene
    // render agree with this camera's matrices.
    EXPECT_NEAR(rotatedForward.x, expectedForward.x, 1e-5F);
    EXPECT_NEAR(rotatedForward.y, expectedForward.y, 1e-5F);
    EXPECT_NEAR(rotatedForward.z, expectedForward.z, 1e-5F);

    // The rotation keeps the horizon level: the camera's right axis has no world-Y component.
    const Math::Vector3UVE right =
        Math::RotateVectorUVE(camera.GetRotationUVE(), Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    EXPECT_NEAR(right.y, 0.0F, 1e-5F);
}

TEST(EditorViewportCameraUVETest, GetViewMatrixUVE_PlacesTheEyeAtTheViewSpaceOrigin) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.42F, -0.2F);

    const Math::Matrix4x4UVE view = camera.GetViewMatrixUVE();
    const Math::Vector3UVE eyeInView = Math::TransformPointUVE(view, camera.GetEyeUVE());
    EXPECT_NEAR(eyeInView.x, 0.0F, 1e-4F);
    EXPECT_NEAR(eyeInView.y, 0.0F, 1e-4F);
    EXPECT_NEAR(eyeInView.z, 0.0F, 1e-4F);

    // The pivot sits straight down the camera's forward axis, one orbit distance away. The camera
    // looks down -Z in view space.
    const Math::Vector3UVE targetInView = Math::TransformPointUVE(view, camera.GetTargetUVE());
    EXPECT_NEAR(targetInView.x, 0.0F, 1e-4F);
    EXPECT_NEAR(targetInView.y, 0.0F, 1e-4F);
    EXPECT_NEAR(targetInView.z, -camera.GetDistanceUVE(), 1e-3F);
}

TEST(EditorViewportCameraUVETest, ProjectionUVE_UsesTheEngineZeroToOneDepthRangeInBothModes) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F);

    // Perspective: the near plane maps to depth 0 and the far plane to depth 1, matching
    // Matrix4x4UVE's documented convention - which is what lets the grid and gizmo overlays share
    // a depth buffer with the engine's scene render.
    const Math::Matrix4x4UVE perspective = camera.GetProjectionMatrixUVE(kAspectUVE);
    const Math::Vector3UVE atNear{0.0F, 0.0F, -camera.GetNearPlaneUVE()};
    const Math::Vector3UVE atFar{0.0F, 0.0F, -camera.GetFarPlaneUVE()};
    EXPECT_NEAR(ProjectUVE(perspective, atNear).z, 0.0F, 1e-4F);
    EXPECT_NEAR(ProjectUVE(perspective, atFar).z, 1.0F, 1e-4F);

    camera.SetOrthographicUVE(true);
    const Math::Matrix4x4UVE orthographic = camera.GetProjectionMatrixUVE(kAspectUVE);
    // Orthographic has no perspective divide at all: w stays 1 for every point.
    EXPECT_NEAR(ProjectUVE(orthographic, Math::Vector3UVE{0.0F, 0.0F, -5.0F}).w, 1.0F, 1e-5F);
    EXPECT_NEAR(ProjectUVE(orthographic, Math::Vector3UVE{0.0F, 0.0F, -50.0F}).w, 1.0F, 1e-5F);
}

TEST(EditorViewportCameraUVETest, ProjectionUVE_OrthographicDropsPerspectiveConvergence) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F);
    camera.SetTargetUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F});

    // Two points the same height above the ground but at genuinely different depths from the
    // camera. At yaw 0 the camera sits out along +X looking back down -X, so depth runs along X.
    const Math::Vector3UVE nearPoint{2.0F, 1.0F, 0.0F};
    const Math::Vector3UVE farPoint{-2.0F, 1.0F, 0.0F};

    const Math::Matrix4x4UVE perspective = camera.GetViewProjectionUVE(kAspectUVE);
    const float perspectiveNearY = ProjectUVE(perspective, nearPoint).y;
    const float perspectiveFarY = ProjectUVE(perspective, farPoint).y;
    // Under perspective the nearer point is magnified, so the two disagree.
    EXPECT_GT(std::fabs(perspectiveNearY - perspectiveFarY), 1e-3F);

    camera.SetOrthographicUVE(true);
    const Math::Matrix4x4UVE orthographic = camera.GetViewProjectionUVE(kAspectUVE);
    const float orthographicNearY = ProjectUVE(orthographic, nearPoint).y;
    const float orthographicFarY = ProjectUVE(orthographic, farPoint).y;
    // Under a real orthographic projection depth cannot change apparent size: parallel lines stay
    // parallel and the two land on exactly the same scanline.
    EXPECT_NEAR(orthographicNearY, orthographicFarY, 1e-5F);
}

TEST(EditorViewportCameraUVETest, TryGetInverseViewProjectionUVE_RoundTripsAProjectedPoint) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(-0.7F, 0.3F);

    Math::Matrix4x4UVE inverse{};
    ASSERT_TRUE(camera.TryGetInverseViewProjectionUVE(kAspectUVE, inverse));

    const Math::Vector3UVE world{1.5F, 0.75F, -2.25F};
    const NdcPointUVE ndc = ProjectUVE(camera.GetViewProjectionUVE(kAspectUVE), world);

    // Unproject: the inverse takes the homogeneous clip point back to world space. This is the
    // exact operation the infinite grid shader performs per pixel to rebuild its view ray.
    const float clipX = ndc.x * ndc.w;
    const float clipY = ndc.y * ndc.w;
    const float clipZ = ndc.z * ndc.w;
    const float clipW = ndc.w;
    const float worldX = (inverse.m[0][0] * clipX) + (inverse.m[0][1] * clipY) +
                         (inverse.m[0][2] * clipZ) + (inverse.m[0][3] * clipW);
    const float worldY = (inverse.m[1][0] * clipX) + (inverse.m[1][1] * clipY) +
                         (inverse.m[1][2] * clipZ) + (inverse.m[1][3] * clipW);
    const float worldZ = (inverse.m[2][0] * clipX) + (inverse.m[2][1] * clipY) +
                         (inverse.m[2][2] * clipZ) + (inverse.m[2][3] * clipW);
    const float worldW = (inverse.m[3][0] * clipX) + (inverse.m[3][1] * clipY) +
                         (inverse.m[3][2] * clipZ) + (inverse.m[3][3] * clipW);
    ASSERT_GT(std::fabs(worldW), 1e-6F);

    EXPECT_NEAR(worldX / worldW, world.x, 1e-3F);
    EXPECT_NEAR(worldY / worldW, world.y, 1e-3F);
    EXPECT_NEAR(worldZ / worldW, world.z, 1e-3F);
}

TEST(EditorViewportCameraUVETest, SnapToDirectionUVE_EasesToTheAxisAndTerminates) {
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(0.0F, 0.0F);

    camera.SnapToDirectionUVE(Math::Vector3UVE{0.0F, 1.0F, 0.0F}); // Top
    EXPECT_TRUE(camera.IsAnimatingUVE());

    // Drive it to completion in small steps, as the editor tick does.
    for (int step = 0; step < 200 && camera.IsAnimatingUVE(); ++step) {
        static_cast<void>(camera.UpdateUVE(0.016F));
    }
    EXPECT_FALSE(camera.IsAnimatingUVE());

    // Top clamps just short of the pole so the view basis never degenerates.
    EXPECT_NEAR(camera.GetPitchUVE(), camera.GetSettingsUVE().pitchMaxRadians, 1e-5F);
    EXPECT_GT(camera.GetEyeUVE().y, camera.GetDistanceUVE() * 0.99F);
}

TEST(EditorViewportCameraUVETest, FocusUVE_MovesThePivotAndFitsTheRadiusWithoutChangingAngles) {
    EditorViewportCameraUVE camera;
    const float yawBefore = camera.GetYawUVE();
    const float pitchBefore = camera.GetPitchUVE();

    camera.FocusUVE(Math::Vector3UVE{5.0F, 2.0F, -3.0F}, 4.0F);

    EXPECT_EQ(camera.GetTargetUVE(), (Math::Vector3UVE{5.0F, 2.0F, -3.0F}));
    EXPECT_EQ(camera.GetYawUVE(), yawBefore);
    EXPECT_EQ(camera.GetPitchUVE(), pitchBefore);
    // Pulled back far enough that a sphere of that radius fits the vertical FOV.
    EXPECT_GT(camera.GetDistanceUVE(), 4.0F);

    // A non-positive radius moves the pivot only.
    const float distanceBefore = camera.GetDistanceUVE();
    camera.FocusUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, 0.0F);
    EXPECT_EQ(camera.GetDistanceUVE(), distanceBefore);
}

TEST(EditorViewportCameraUVETest, ClipPlanesScaleWithDistanceSoDepthPrecisionSurvivesTheZoomRange) {
    EditorViewportCameraUVE camera;

    camera.SetDistanceUVE(1.0F);
    const float nearClose = camera.GetNearPlaneUVE();
    const float farClose = camera.GetFarPlaneUVE();

    camera.SetDistanceUVE(1000.0F);
    const float nearFar = camera.GetNearPlaneUVE();
    const float farFar = camera.GetFarPlaneUVE();

    EXPECT_GT(nearFar, nearClose);
    EXPECT_GT(farFar, farClose);
    // The ratio is what actually governs depth precision, and it stays constant across the range.
    EXPECT_NEAR(farClose / nearClose, farFar / nearFar, (farClose / nearClose) * 0.001F);
}

} // namespace UVE::Editor
