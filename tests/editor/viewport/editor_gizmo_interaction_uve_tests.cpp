// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <gtest/gtest.h>

#include <cmath>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/editor/viewport/editor_gizmo_interaction_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/scene/components/transform_component_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kViewportWidthUVE = 800.0F;
constexpr float kViewportHeightUVE = 600.0F;
/// One gizmo unit is one world unit here, which keeps every expected screen position in the tests
/// below computable straight from the geometry constants.
constexpr float kUnitScaleUVE = 1.0F;

[[nodiscard]] Core::EngineConfigUVE MakeGizmoTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_gizmo_tests.log";
    config.settingsFilePath = "uve_gizmo_tests_settings.json";
    config.assetDatabaseFilePath = "uve_gizmo_tests_assets.json";
    config.saveDirectoryPath = "uve_gizmo_tests_saves";
    config.shaderCachePath = "uve_gizmo_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

/// The viewport projection for an editor's current camera, filling an 800x600 region at the origin.
[[nodiscard]] EditorViewportProjectionUVE MakeProjectionUVE(const EditorUVE& editor) {
    EditorViewportProjectionUVE projection{};
    projection.viewProjection = editor.GetViewportCameraControllerUVE().GetViewProjectionUVE(
        kViewportWidthUVE / kViewportHeightUVE);
    projection.origin = Math::Vector2UVE{0.0F, 0.0F};
    projection.size = Math::Vector2UVE{kViewportWidthUVE, kViewportHeightUVE};
    return projection;
}

/// Creates a cube at the world origin and selects it, which is the state every drag test starts in.
[[nodiscard]] Scene::EntityUVE CreateSelectedCubeUVE(EditorUVE& editor, Core::EngineCoreUVE& engine) {
    const Scene::EntityUVE cube = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
    engine.GetServicesUVE().GetSceneGraphUVE().UpdateUVE(engine.GetServicesUVE().GetEntityManagerUVE());
    return cube;
}

[[nodiscard]] Scene::TransformComponentUVE TransformOfUVE(Core::EngineCoreUVE& engine,
                                                          const Scene::EntityUVE entity) {
    return engine.GetServicesUVE().GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(
        entity);
}

} // namespace

TEST(EditorGizmoInteractionUVETest, PickGizmoHandleUVE_FindsTheAxisShaftUnderThePointer) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_pick.uvescene");
        editor.InitUVE();
        static_cast<void>(CreateSelectedCubeUVE(editor, engine));
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Move);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};

        // Press exactly on the middle of the +X shaft.
        const float shaftMiddle = (style.moveShaftStart + style.moveShaftEnd) * 0.5F;
        Math::Vector2UVE onShaft{};
        ASSERT_TRUE(ProjectWorldPointUVE(
            projection, pivot + (Math::Vector3UVE{1.0F, 0.0F, 0.0F} * (shaftMiddle * kUnitScaleUVE)),
            onShaft));

        const EditorGizmoHandleHitUVE hit = PickGizmoHandleUVE(
            EditorGizmoModeUVE::Move, style, projection, pivot, kUnitScaleUVE, onShaft);
        ASSERT_TRUE(hit.IsHitUVE());
        EXPECT_EQ(hit.kind, EditorGizmoHandleKindUVE::TranslateAxis);
        EXPECT_EQ(hit.axis, EditorTransformAxisUVE::X);

        // Far from every handle is a clean miss, so a click in empty space is not swallowed.
        const EditorGizmoHandleHitUVE miss = PickGizmoHandleUVE(
            EditorGizmoModeUVE::Move, style, projection, pivot, kUnitScaleUVE,
            Math::Vector2UVE{5.0F, 5.0F});
        EXPECT_FALSE(miss.IsHitUVE());

        // Select mode draws no handles, so it can never report one.
        const EditorGizmoHandleHitUVE selectMode = PickGizmoHandleUVE(
            EditorGizmoModeUVE::Select, style, projection, pivot, kUnitScaleUVE, onShaft);
        EXPECT_FALSE(selectMode.IsHitUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, ComputeAxisDragDistanceUVE_RefusesAnAxisPointingAtTheCamera) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_edge_on.uvescene");
        editor.InitUVE();
        // Look straight down the +X axis: it projects to almost nothing on screen.
        editor.GetViewportCameraControllerUVE().SetYawPitchUVE(0.0F, 0.0F);
        editor.GetViewportCameraControllerUVE().SetTargetUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F});

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        float distance = 0.0F;
        const bool resolved = ComputeAxisDragDistanceUVE(
            projection, Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F},
            kUnitScaleUVE, Math::Vector2UVE{400.0F, 300.0F}, Math::Vector2UVE{460.0F, 300.0F}, distance);

        // Refusing is the point: resolving a drag onto a handle that is only a pixel or two long
        // would amplify pointer noise into an enormous world distance and fling the object away.
        EXPECT_FALSE(resolved);

        // The same drag on an axis that is broadside to the camera resolves normally.
        float sideDistance = 0.0F;
        EXPECT_TRUE(ComputeAxisDragDistanceUVE(
            projection, Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F},
            kUnitScaleUVE, Math::Vector2UVE{400.0F, 300.0F}, Math::Vector2UVE{460.0F, 300.0F},
            sideDistance));
        EXPECT_NE(sideDistance, 0.0F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, DraggingATranslateHandleMovesTheEntityAndIsReversible) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_translate.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE cube = CreateSelectedCubeUVE(editor, engine);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Move);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};
        const Math::Vector3UVE axisX{1.0F, 0.0F, 0.0F};

        Math::Vector2UVE pivotScreen{};
        Math::Vector2UVE axisTipScreen{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot, pivotScreen));
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * kUnitScaleUVE), axisTipScreen));

        const float shaftMiddle = (style.moveShaftStart + style.moveShaftEnd) * 0.5F;
        Math::Vector2UVE grabPoint{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * (shaftMiddle * kUnitScaleUVE)),
                                         grabPoint));

        ASSERT_TRUE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, grabPoint));
        EXPECT_TRUE(editor.IsGizmoDraggingUVE());

        // Drag one projected axis-length further along the axis's own screen direction, which is
        // one world unit by construction.
        const Math::Vector2UVE dragTo{grabPoint.x + (axisTipScreen.x - pivotScreen.x),
                                      grabPoint.y + (axisTipScreen.y - pivotScreen.y)};
        editor.UpdateGizmoDragUVE(projection, kUnitScaleUVE, dragTo);
        EXPECT_NEAR(TransformOfUVE(engine, cube).localPosition.x, 1.0F, 0.05F);

        // Returning the pointer to where it started returns the object exactly, because every
        // update is recomputed from the gesture's starting transform rather than accumulated.
        editor.UpdateGizmoDragUVE(projection, kUnitScaleUVE, grabPoint);
        EXPECT_NEAR(TransformOfUVE(engine, cube).localPosition.x, 0.0F, 1e-5F);

        editor.UpdateGizmoDragUVE(projection, kUnitScaleUVE, dragTo);
        editor.CommitGizmoDragUVE();
        EXPECT_FALSE(editor.IsGizmoDraggingUVE());
        EXPECT_NEAR(TransformOfUVE(engine, cube).localPosition.x, 1.0F, 0.05F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, AWholeDragIsOneUndoStepNotOnePerFrame) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_undo.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE cube = CreateSelectedCubeUVE(editor, engine);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Move);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};
        const Math::Vector3UVE axisX{1.0F, 0.0F, 0.0F};
        const Math::Vector3UVE startPosition = TransformOfUVE(engine, cube).localPosition;

        Math::Vector2UVE pivotScreen{};
        Math::Vector2UVE axisTipScreen{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot, pivotScreen));
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * kUnitScaleUVE), axisTipScreen));
        const float shaftMiddle = (style.moveShaftStart + style.moveShaftEnd) * 0.5F;
        Math::Vector2UVE grabPoint{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * (shaftMiddle * kUnitScaleUVE)),
                                         grabPoint));

        ASSERT_TRUE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, grabPoint));
        // Twenty intermediate updates, as a real drag would produce one per frame.
        for (int step = 1; step <= 20; ++step) {
            const float t = static_cast<float>(step) / 20.0F;
            editor.UpdateGizmoDragUVE(
                projection, kUnitScaleUVE,
                Math::Vector2UVE{grabPoint.x + ((axisTipScreen.x - pivotScreen.x) * t),
                                 grabPoint.y + ((axisTipScreen.y - pivotScreen.y) * t)});
        }
        editor.CommitGizmoDragUVE();

        const Math::Vector3UVE draggedPosition = TransformOfUVE(engine, cube).localPosition;
        EXPECT_GT(std::fabs(draggedPosition.x - startPosition.x), 0.5F);

        // One undo returns the whole gesture, not one twentieth of it.
        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        const Math::Vector3UVE undonePosition = TransformOfUVE(engine, cube).localPosition;
        EXPECT_NEAR(undonePosition.x, startPosition.x, 1e-4F);

        // And redo puts it back where the drag left it.
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NEAR(TransformOfUVE(engine, cube).localPosition.x, draggedPosition.x, 1e-4F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, CancelGizmoDragUVE_RestoresTheTransformAndRecordsNothing) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_cancel.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE cube = CreateSelectedCubeUVE(editor, engine);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Move);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};
        const Math::Vector3UVE axisX{1.0F, 0.0F, 0.0F};
        const Math::Vector3UVE startPosition = TransformOfUVE(engine, cube).localPosition;

        Math::Vector2UVE pivotScreen{};
        Math::Vector2UVE axisTipScreen{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot, pivotScreen));
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * kUnitScaleUVE), axisTipScreen));
        const float shaftMiddle = (style.moveShaftStart + style.moveShaftEnd) * 0.5F;
        Math::Vector2UVE grabPoint{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * (shaftMiddle * kUnitScaleUVE)),
                                         grabPoint));

        const bool couldUndoBefore = editor.CanUndoUVE();
        ASSERT_TRUE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, grabPoint));
        editor.UpdateGizmoDragUVE(
            projection, kUnitScaleUVE,
            Math::Vector2UVE{grabPoint.x + (axisTipScreen.x - pivotScreen.x),
                             grabPoint.y + (axisTipScreen.y - pivotScreen.y)});
        EXPECT_GT(std::fabs(TransformOfUVE(engine, cube).localPosition.x - startPosition.x), 0.5F);

        editor.CancelGizmoDragUVE();
        EXPECT_FALSE(editor.IsGizmoDraggingUVE());
        EXPECT_NEAR(TransformOfUVE(engine, cube).localPosition.x, startPosition.x, 1e-5F);
        EXPECT_EQ(editor.CanUndoUVE(), couldUndoBefore) << "a cancelled drag leaves no history";

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, DraggingARotateRingTurnsTheEntity) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_rotate.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE cube = CreateSelectedCubeUVE(editor, engine);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Rotate);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};
        const Math::QuaternionUVE startRotation = TransformOfUVE(engine, cube).localRotation;

        // Grab the Y ring where it crosses the +X direction, then sweep the pointer around the
        // pivot.
        Math::Vector2UVE grabPoint{};
        ASSERT_TRUE(ProjectWorldPointUVE(
            projection, pivot + (Math::Vector3UVE{1.0F, 0.0F, 0.0F} * (style.ringRadius * kUnitScaleUVE)),
            grabPoint));
        Math::Vector2UVE pivotScreen{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot, pivotScreen));

        ASSERT_TRUE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, grabPoint));
        // Rotate the grab point a quarter turn around the pivot on screen.
        const float armX = grabPoint.x - pivotScreen.x;
        const float armY = grabPoint.y - pivotScreen.y;
        editor.UpdateGizmoDragUVE(projection, kUnitScaleUVE,
                                  Math::Vector2UVE{pivotScreen.x - armY, pivotScreen.y + armX});
        editor.CommitGizmoDragUVE();

        const Math::QuaternionUVE endRotation = TransformOfUVE(engine, cube).localRotation;
        EXPECT_NE(endRotation, startRotation) << "sweeping a ring must actually turn the object";

        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(TransformOfUVE(engine, cube).localRotation, startRotation);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, DraggingAScaleHandleResizesTheEntityAndNeverThroughZero) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_scale.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE cube = CreateSelectedCubeUVE(editor, engine);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Scale);

        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);
        const EditorGizmoStyleUVE style{};
        const Math::Vector3UVE pivot{0.0F, 0.0F, 0.0F};
        const Math::Vector3UVE axisX{1.0F, 0.0F, 0.0F};

        Math::Vector2UVE pivotScreen{};
        Math::Vector2UVE axisTipScreen{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot, pivotScreen));
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * kUnitScaleUVE), axisTipScreen));
        const float shaftMiddle = (style.scaleShaftStart + style.scaleShaftEnd) * 0.5F;
        Math::Vector2UVE grabPoint{};
        ASSERT_TRUE(ProjectWorldPointUVE(projection, pivot + (axisX * (shaftMiddle * kUnitScaleUVE)),
                                         grabPoint));

        ASSERT_TRUE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, grabPoint));
        editor.UpdateGizmoDragUVE(
            projection, kUnitScaleUVE,
            Math::Vector2UVE{grabPoint.x + (axisTipScreen.x - pivotScreen.x),
                             grabPoint.y + (axisTipScreen.y - pivotScreen.y)});
        EXPECT_GT(TransformOfUVE(engine, cube).localScale.x, 1.5F);

        // Dragging far the other way floors the scale instead of passing through zero into a
        // mirrored object.
        editor.UpdateGizmoDragUVE(
            projection, kUnitScaleUVE,
            Math::Vector2UVE{grabPoint.x - ((axisTipScreen.x - pivotScreen.x) * 50.0F),
                             grabPoint.y - ((axisTipScreen.y - pivotScreen.y) * 50.0F)});
        const Math::Vector3UVE floored = TransformOfUVE(engine, cube).localScale;
        EXPECT_GT(floored.x, 0.0F);
        EXPECT_LT(floored.x, 0.01F);
        // The untouched axes are left exactly alone.
        EXPECT_NEAR(floored.y, 1.0F, 1e-5F);
        EXPECT_NEAR(floored.z, 1.0F, 1e-5F);

        editor.CommitGizmoDragUVE();
        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorGizmoInteractionUVETest, BeginGizmoDragUVE_RefusesWithoutASingleLiveSelection) {
    Core::EngineCoreUVE engine(MakeGizmoTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_gizmo_tests_guard.uvescene");
        editor.InitUVE();
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Move);
        const EditorViewportProjectionUVE projection = MakeProjectionUVE(editor);

        // Nothing selected: there is no pivot, so there is nothing to grab.
        EXPECT_FALSE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, Math::Vector2UVE{400.0F, 300.0F}));
        EXPECT_FALSE(editor.IsGizmoDraggingUVE());

        static_cast<void>(CreateSelectedCubeUVE(editor, engine));
        // Selected, but the pointer is nowhere near a handle.
        EXPECT_FALSE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, Math::Vector2UVE{5.0F, 5.0F}));

        // Select mode draws no handles at all.
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Select);
        const EditorGizmoStyleUVE style{};
        Math::Vector2UVE onShaft{};
        const float shaftMiddle = (style.moveShaftStart + style.moveShaftEnd) * 0.5F;
        ASSERT_TRUE(ProjectWorldPointUVE(
            projection,
            Math::Vector3UVE{0.0F, 0.0F, 0.0F} +
                (Math::Vector3UVE{1.0F, 0.0F, 0.0F} * (shaftMiddle * kUnitScaleUVE)),
            onShaft));
        EXPECT_FALSE(editor.BeginGizmoDragUVE(projection, kUnitScaleUVE, onShaft));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

} // namespace UVE::Editor
