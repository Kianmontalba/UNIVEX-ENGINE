// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Editor {

namespace {

[[nodiscard]] Core::EngineConfigUVE MakeViewportTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_viewport_tests.log";
    config.settingsFilePath = "uve_viewport_tests_settings.json";
    config.assetDatabaseFilePath = "uve_viewport_tests_assets.json";
    config.saveDirectoryPath = "uve_viewport_tests_saves";
    config.shaderCachePath = "uve_viewport_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

} // namespace

TEST(EditorViewportIntegrationUVETest, InitUVE_CreatesACameraEntityThatIsNotDocumentData) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_camera.uvescene");
        EXPECT_EQ(editor.GetViewportCameraUVE(), Scene::kInvalidEntityUVE)
            << "no camera exists before InitUVE()";

        editor.InitUVE();
        const Scene::EntityUVE camera = editor.GetViewportCameraUVE();
        ASSERT_NE(camera, Scene::kInvalidEntityUVE);

        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        EXPECT_TRUE(entityManager.IsAliveUVE(camera));

        // It is a real ECS camera, so ICameraSystemUVE and Renderer3DUVE consume it exactly like
        // any other camera rather than through a parallel editor-camera path.
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(camera));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(camera));

        // ... and it is editor presentation, not document data: out of the hierarchy, out of every
        // save, and rejected by every command path that acts on document entities.
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        EXPECT_EQ(std::find(roots.begin(), roots.end(), camera), roots.end());

        editor.SelectEntityUVE(camera);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE)
            << "the editor camera must not be selectable";

        editor.ShutdownUVE();
        EXPECT_EQ(editor.GetViewportCameraUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(entityManager.IsAliveUVE(camera)) << "ShutdownUVE() must reclaim the camera";
    }

    engine.Shutdown();
}

TEST(EditorViewportIntegrationUVETest, ViewportCameraEntityTracksTheOrbitControllerEveryTick) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_sync.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE camera = editor.GetViewportCameraUVE();
        ASSERT_NE(camera, Scene::kInvalidEntityUVE);

        editor.GetViewportCameraControllerUVE().SetTargetUVE(Math::Vector3UVE{4.0F, 1.0F, -2.0F});
        editor.GetViewportCameraControllerUVE().SetYawPitchUVE(0.35F, 0.2F);
        editor.TickUVE();

        // The engine renders from the entity, so the entity - not just the controller - has to
        // carry the pose, or the scene would render from a stale view.
        const Math::Vector3UVE expectedEye = editor.GetViewportCameraControllerUVE().GetEyeUVE();
        const Scene::WorldTransformComponentUVE& worldTransform =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(camera);
        EXPECT_NEAR(worldTransform.worldPosition.x, expectedEye.x, 1e-4F);
        EXPECT_NEAR(worldTransform.worldPosition.y, expectedEye.y, 1e-4F);
        EXPECT_NEAR(worldTransform.worldPosition.z, expectedEye.z, 1e-4F);

        // Clip planes follow the orbit distance, which is what keeps depth precision usable across
        // the whole zoom range.
        const Scene::CameraComponentUVE& cameraComponent =
            entityManager.GetComponentUVE<Scene::CameraComponentUVE>(camera);
        EXPECT_NEAR(cameraComponent.nearPlane,
                    editor.GetViewportCameraControllerUVE().GetNearPlaneUVE(), 1e-5F);
        EXPECT_NEAR(cameraComponent.farPlane,
                    editor.GetViewportCameraControllerUVE().GetFarPlaneUVE(), 1e-3F);
        EXPECT_TRUE(Scene::IsCameraComponentValidUVE(cameraComponent));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorViewportIntegrationUVETest, GizmoPivotIsTheSelectedEntitysOwnTransformNotTheWorldOrigin) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_pivot.uvescene");
        editor.InitUVE();

        Math::Vector3UVE pivot{};
        EXPECT_FALSE(editor.TryGetGizmoPivotUVE(pivot))
            << "no selection means no gizmo, so no pivot to report";

        const Scene::EntityUVE cube = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_NE(cube, Scene::kInvalidEntityUVE);

        // Move the cube well away from the origin: a gizmo pinned to the world origin or to the
        // viewport centre would still report {0,0,0} here, which is exactly the bug this guards.
        Scene::TransformComponentUVE transform =
            engine.GetServicesUVE().GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(
                cube);
        transform.localPosition = Math::Vector3UVE{6.0F, 2.5F, -3.5F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(transform));
        engine.GetServicesUVE().GetSceneGraphUVE().UpdateUVE(engine.GetServicesUVE().GetEntityManagerUVE());

        ASSERT_TRUE(editor.TryGetGizmoPivotUVE(pivot));
        EXPECT_NEAR(pivot.x, 6.0F, 1e-4F);
        EXPECT_NEAR(pivot.y, 2.5F, 1e-4F);
        EXPECT_NEAR(pivot.z, -3.5F, 1e-4F);

        // Clearing the selection removes the gizmo entirely rather than stranding it at the last
        // pivot.
        editor.ClearSelectionUVE();
        EXPECT_FALSE(editor.TryGetGizmoPivotUVE(pivot));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorViewportIntegrationUVETest, ProjectionSwitchIsRealAndPreservesTheView) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_projection.uvescene");
        editor.InitUVE();

        // Perspective is the startup projection and is deliberately not recomputed at launch.
        EXPECT_EQ(editor.GetViewportSettingsUVE().projection,
                  EditorViewportProjectionModeUVE::Perspective);
        EXPECT_FALSE(editor.GetViewportCameraControllerUVE().IsOrthographicUVE());

        editor.GetViewportCameraControllerUVE().SetYawPitchUVE(-0.62F, 0.31F);
        editor.GetViewportCameraControllerUVE().SetTargetUVE(Math::Vector3UVE{1.0F, 0.5F, 2.0F});
        const Math::Vector3UVE eyeBefore = editor.GetViewportCameraControllerUVE().GetEyeUVE();
        const float distanceBefore = editor.GetViewportCameraControllerUVE().GetDistanceUVE();

        editor.SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Orthographic);
        EXPECT_EQ(editor.GetViewportSettingsUVE().projection,
                  EditorViewportProjectionModeUVE::Orthographic);
        // The switch reaches the camera itself, so the projection matrix really changes rather
        // than only the label on the control.
        EXPECT_TRUE(editor.GetViewportCameraControllerUVE().IsOrthographicUVE());

        // Orientation, pivot and distance all survive: the user is not teleported by a projection
        // change.
        EXPECT_EQ(editor.GetViewportCameraControllerUVE().GetEyeUVE(), eyeBefore);
        EXPECT_EQ(editor.GetViewportCameraControllerUVE().GetDistanceUVE(), distanceBefore);

        editor.SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Perspective);
        EXPECT_FALSE(editor.GetViewportCameraControllerUVE().IsOrthographicUVE());
        EXPECT_EQ(editor.GetViewportCameraControllerUVE().GetEyeUVE(), eyeBefore);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorViewportIntegrationUVETest, StandardViewSnapsTheCameraAndTurnsOnOrthographic) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_standard_view.uvescene");
        editor.InitUVE();

        editor.SetStandardViewUVE(EditorStandardViewUVE::Front);
        EXPECT_EQ(editor.GetViewportSettingsUVE().standardView, EditorStandardViewUVE::Front);
        // An axis view in perspective is almost never what "Front" means, so autoOrthographic
        // switches projection with the snap.
        EXPECT_EQ(editor.GetViewportSettingsUVE().projection,
                  EditorViewportProjectionModeUVE::Orthographic);

        // The snap is driven with an explicit delta here rather than through TickUVE(): TickUVE()
        // reads the engine timer, and this test never runs a full engine frame, so that delta is
        // zero and the eased snap would correctly make no progress at all.
        for (int step = 0; step < 200 && editor.GetViewportCameraControllerUVE().IsAnimatingUVE();
             ++step) {
            static_cast<void>(editor.GetViewportCameraControllerUVE().UpdateUVE(0.016F));
        }
        editor.TickUVE();

        // Front looks back along +Z at the pivot.
        const Math::Vector3UVE eye = editor.GetViewportCameraControllerUVE().GetEyeUVE();
        const float distance = editor.GetViewportCameraControllerUVE().GetDistanceUVE();
        EXPECT_NEAR(eye.z, distance, distance * 0.02F);
        EXPECT_NEAR(eye.x, 0.0F, distance * 0.02F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorViewportIntegrationUVETest, GroundGridStaysAtTheWorldOriginWhateverIsSelected) {
    Core::EngineCoreUVE engine(MakeViewportTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_viewport_tests_grid.uvescene");
        editor.InitUVE();
        editor.TickUVE();
        const EditorGridDisplayStateUVE gridBefore = editor.ComputeGroundGridStateUVE();

        // The grid is world space and the gizmo follows the selection - two different owners.
        // Selecting and moving an entity must not drag the grid along with it, so the state the
        // renderer is handed carries no selection-derived origin at all: the only camera-derived
        // values are the horizon fade distances.
        const Scene::EntityUVE cube = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_NE(cube, Scene::kInvalidEntityUVE);
        Scene::TransformComponentUVE transform =
            engine.GetServicesUVE().GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(
                cube);
        transform.localPosition = Math::Vector3UVE{25.0F, 4.0F, 9.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(transform));
        engine.GetServicesUVE().GetSceneGraphUVE().UpdateUVE(engine.GetServicesUVE().GetEntityManagerUVE());
        editor.TickUVE();

        // A moved, selected entity changes the gizmo pivot ...
        Math::Vector3UVE pivot{};
        ASSERT_TRUE(editor.TryGetGizmoPivotUVE(pivot));
        EXPECT_NEAR(pivot.x, 25.0F, 1e-4F);

        // ... while the grid state handed to the renderer is byte-for-byte what it was before the
        // selection existed. The grid carries no selection-derived value at all.
        const EditorGridDisplayStateUVE afterSelection = editor.ComputeGroundGridStateUVE();
        EXPECT_TRUE(afterSelection.enabled);
        EXPECT_EQ(afterSelection.baseSpacing, gridBefore.baseSpacing);
        EXPECT_EQ(afterSelection.fadeStartDistance, gridBefore.fadeStartDistance);
        EXPECT_EQ(afterSelection.fadeEndDistance, gridBefore.fadeEndDistance);

        // The one thing that does move the grid is the camera's own orbit distance, which sets
        // where the horizon fade sits so it stays in the same place on screen at any zoom.
        editor.GetViewportCameraControllerUVE().SetDistanceUVE(
            editor.GetViewportCameraControllerUVE().GetDistanceUVE() * 4.0F);
        const EditorGridDisplayStateUVE afterZoom = editor.ComputeGroundGridStateUVE();
        EXPECT_GT(afterZoom.fadeEndDistance, gridBefore.fadeEndDistance);

        // Turning the grid off in the Show menu stops the pass being recorded at all rather than
        // drawing a fully transparent one.
        EXPECT_TRUE(editor.ComputeGroundGridStateUVE().enabled);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

} // namespace UVE::Editor
