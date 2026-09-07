// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include "uve/editor/viewport/editor_nav_gizmo_uve.h"
#include "uve/editor/viewport/editor_viewport_camera_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kNavSizePxUVE = 154.0F;

/// Where a nav handle lands inside the widget, in pixels from its top-left - the same projection
/// PickNavGizmoUVE performs, so a test can press exactly on a ball.
struct NavScreenPointUVE final {
    float x = 0.0F;
    float y = 0.0F;
};

[[nodiscard]] NavScreenPointUVE ProjectHandleUVE(const EditorGizmoStyleUVE& style,
                                                 const Math::Matrix4x4UVE& viewRotation,
                                                 const Math::Vector3UVE& direction,
                                                 const float viewportSizePx) {
    const float pixelsPerUnit = (viewportSizePx * 0.5F) / GetNavViewHalfExtentUVE(style);
    const float centerPx = viewportSizePx * 0.5F;
    const Math::Vector3UVE viewSpace = TransformDirectionUVE(viewRotation, direction);
    return NavScreenPointUVE{centerPx + (viewSpace.x * pixelsPerUnit),
                             centerPx - (viewSpace.y * pixelsPerUnit)};
}

} // namespace

TEST(EditorNavGizmoUVETest, HandlesCoverBothEndsOfAllThreeAxesInAStableOrder) {
    const EditorGizmoStyleUVE style{};
    const std::array<EditorNavHandleUVE, 6> handles = GetNavHandlesUVE(style);

    EXPECT_EQ(handles[0].direction, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
    EXPECT_EQ(handles[1].direction, (Math::Vector3UVE{-1.0F, 0.0F, 0.0F}));
    EXPECT_EQ(handles[2].direction, (Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    EXPECT_EQ(handles[3].direction, (Math::Vector3UVE{0.0F, -1.0F, 0.0F}));
    EXPECT_EQ(handles[4].direction, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(handles[5].direction, (Math::Vector3UVE{0.0F, 0.0F, -1.0F}));

    EXPECT_EQ(handles[0].axisLabel, 'X');
    EXPECT_EQ(handles[2].axisLabel, 'Y');
    EXPECT_EQ(handles[4].axisLabel, 'Z');

    // Positive ends are solid and labelled; negative ends are hollow rings.
    EXPECT_TRUE(handles[0].positive);
    EXPECT_FALSE(handles[1].positive);

    // Each axis pair shares one colour, so an axis reads as one axis.
    EXPECT_EQ(handles[0].color, handles[1].color);
    EXPECT_EQ(handles[2].color, handles[3].color);
    EXPECT_EQ(handles[4].color, handles[5].color);
}

TEST(EditorNavGizmoUVETest, BuildNavGizmoMeshesUVE_SplitsStubsUnderTheBallsAndLabelsOver) {
    const EditorGizmoStyleUVE style{};
    const EditorNavGizmoMeshesUVE meshes =
        BuildNavGizmoMeshesUVE(style, Math::Vector3UVE{0.0F, 0.0F, -1.0F});

    // One full-length stub per axis, drawn under the balls.
    EXPECT_EQ(meshes.underlay.lines.size(), 3U);
    EXPECT_TRUE(meshes.underlay.triangles.empty());

    // Balls are discs of real triangles; the three positive ones carry letter strokes over them.
    EXPECT_FALSE(meshes.overlay.triangles.empty());
    EXPECT_FALSE(meshes.overlay.lines.empty());

    // X is two strokes, Y is three, Z is three - eight letter strokes in all.
    EXPECT_EQ(meshes.overlay.lines.size(), 8U);
}

TEST(EditorNavGizmoUVETest, PickNavGizmoUVE_HitsEachBallAtItsOwnProjectedPosition) {
    const EditorGizmoStyleUVE style{};
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(-0.7553F, 0.4561F); // the default three-quarter view
    const Math::Matrix4x4UVE viewRotation = camera.GetViewMatrixUVE();

    for (const EditorNavHandleUVE& handle : GetNavHandlesUVE(style)) {
        const NavScreenPointUVE point =
            ProjectHandleUVE(style, viewRotation, handle.direction, kNavSizePxUVE);
        const EditorNavPickResultUVE result =
            PickNavGizmoUVE(style, viewRotation, point.x, point.y, kNavSizePxUVE);

        ASSERT_TRUE(result.hit) << "ball " << handle.axisLabel << (handle.positive ? '+' : '-');
        EXPECT_EQ(result.direction, handle.direction);
        EXPECT_EQ(result.axisLabel, handle.axisLabel);
        EXPECT_EQ(result.positive, handle.positive);
    }
}

TEST(EditorNavGizmoUVETest, PickNavGizmoUVE_PressingEmptySpaceInsideTheWidgetPicksNothing) {
    const EditorGizmoStyleUVE style{};
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(-0.7553F, 0.4561F);
    const Math::Matrix4x4UVE viewRotation = camera.GetViewMatrixUVE();

    // A corner of the square widget: inside the viewport, but well clear of every ball.
    const EditorNavPickResultUVE corner = PickNavGizmoUVE(style, viewRotation, 2.0F, 2.0F, kNavSizePxUVE);
    EXPECT_FALSE(corner.hit);

    // A degenerate widget size is a miss, not a divide by zero.
    const EditorNavPickResultUVE degenerate =
        PickNavGizmoUVE(style, viewRotation, 10.0F, 10.0F, 0.0F);
    EXPECT_FALSE(degenerate.hit);
}

TEST(EditorNavGizmoUVETest, PickNavGizmoUVE_PrefersTheNearerBallWhenTwoProjectOnTopOfEachOther) {
    const EditorGizmoStyleUVE style{};
    EditorViewportCameraUVE camera;
    // Looking as close to straight down +Y as the pitch clamp allows: the +Y and -Y balls project
    // almost exactly on top of each other, and the near one must win.
    camera.SetYawPitchUVE(0.0F, camera.GetSettingsUVE().pitchMaxRadians);
    const Math::Matrix4x4UVE viewRotation = camera.GetViewMatrixUVE();

    const NavScreenPointUVE point =
        ProjectHandleUVE(style, viewRotation, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, kNavSizePxUVE);
    const EditorNavPickResultUVE result =
        PickNavGizmoUVE(style, viewRotation, point.x, point.y, kNavSizePxUVE);

    ASSERT_TRUE(result.hit);
    EXPECT_EQ(result.axisLabel, 'Y');
    EXPECT_TRUE(result.positive) << "the camera is above the pivot, so +Y is the nearer ball";
}

TEST(EditorNavGizmoUVETest, ClickingABallSnapsTheCameraOntoThatAxis) {
    const EditorGizmoStyleUVE style{};
    EditorViewportCameraUVE camera;
    camera.SetYawPitchUVE(-0.7553F, 0.4561F);

    const NavScreenPointUVE point = ProjectHandleUVE(
        style, camera.GetViewMatrixUVE(), Math::Vector3UVE{1.0F, 0.0F, 0.0F}, kNavSizePxUVE);
    const EditorNavPickResultUVE result =
        PickNavGizmoUVE(style, camera.GetViewMatrixUVE(), point.x, point.y, kNavSizePxUVE);
    ASSERT_TRUE(result.hit);

    camera.SnapToDirectionUVE(result.direction);
    for (int step = 0; step < 200 && camera.IsAnimatingUVE(); ++step) {
        static_cast<void>(camera.UpdateUVE(0.016F));
    }

    // The camera ends up sitting out along +X, looking back at the pivot.
    const Math::Vector3UVE eye = camera.GetEyeUVE();
    EXPECT_NEAR(eye.x, camera.GetDistanceUVE(), camera.GetDistanceUVE() * 0.01F);
    EXPECT_NEAR(eye.y, 0.0F, camera.GetDistanceUVE() * 0.01F);
    EXPECT_NEAR(eye.z, 0.0F, camera.GetDistanceUVE() * 0.01F);
}

TEST(EditorNavGizmoUVETest, ViewHalfExtentLeavesRoomSoBallsNeverClipTheWidgetEdge) {
    const EditorGizmoStyleUVE style{};
    const float halfExtent = GetNavViewHalfExtentUVE(style);

    // One unit out to the ball centre plus its radius has to fit, with air to spare.
    EXPECT_GT(halfExtent, 1.0F + style.navBallRadius);
}

} // namespace UVE::Editor
