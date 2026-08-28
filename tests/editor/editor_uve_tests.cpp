// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/editor/gizmo_system_uve.h"
#include "uve/editor/viewport_nav_gizmo_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/nodes/scene_node_registry_uve.h"

namespace UVE::Editor::Tests {

struct EditorUVEAccessUVE final {
    [[nodiscard]] static std::string GetOutlinerTypeTagUVE(const EditorUVE& editor,
                                                            const Scene::EntityUVE entity) {
        return editor.GetOutlinerTypeTagUVE(entity);
    }

    [[nodiscard]] static std::vector<Scene::EntityUVE> GetDocumentAncestryUVE(const EditorUVE& editor,
                                                                                const Scene::EntityUVE entity) {
        return editor.GetDocumentAncestryUVE(entity);
    }

    [[nodiscard]] static std::vector<Scene::EntityUVE> GetEligibleReparentParentsUVE(EditorUVE& editor,
                                                                                        const Scene::EntityUVE entity) {
        return editor.GetEligibleReparentParentsUVE(entity);
    }

    [[nodiscard]] static bool HasInspectorDrawerUVE(const EditorUVE& editor, const std::string_view id) {
        return editor.m_inspectorDrawerRegistry.HasDrawerUVE(id);
    }

    [[nodiscard]] static std::size_t GetInspectorDrawerCountUVE(const EditorUVE& editor) {
        return editor.m_inspectorDrawerRegistry.GetDrawerCountUVE();
    }

    [[nodiscard]] static std::string GetContentBrowserItemTypeLabelUVE(const Asset::ProjectFileEntryUVE& entry) {
        return EditorUVE::GetContentBrowserItemTypeLabelUVE(EditorUVE::ClassifyContentBrowserEntryUVE(entry));
    }

    static void SelectContentBrowserMeshFocusUVE(EditorUVE& editor) {
        editor.m_contentBrowserTypeFocus = EditorUVE::ContentBrowserTypeFocusUVE::Mesh;
    }

    static void SelectContentBrowserRegisteredFocusUVE(EditorUVE& editor) {
        editor.m_contentBrowserTypeFocus = EditorUVE::ContentBrowserTypeFocusUVE::Registered;
    }

    static void SelectContentBrowserMotionQueryFocusUVE(EditorUVE& editor) {
        editor.m_contentBrowserTypeFocus = EditorUVE::ContentBrowserTypeFocusUVE::MotionQuery;
    }

    [[nodiscard]] static bool DoesContentBrowserEntryMatchFocusUVE(const EditorUVE& editor,
                                                                     const Asset::ProjectFileEntryUVE& entry) {
        return editor.DoesContentBrowserEntryMatchFocusUVE(entry);
    }

    static void SetContentBrowserDirectoryUVE(EditorUVE& editor, std::filesystem::path directory) {
        editor.m_contentBrowserDirectory = std::move(directory);
    }

    [[nodiscard]] static const std::filesystem::path& GetContentBrowserDirectoryUVE(const EditorUVE& editor) {
        return editor.m_contentBrowserDirectory;
    }

    static void SetAssetFilterUVE(EditorUVE& editor, std::string filter) { editor.m_assetFilter = std::move(filter); }

    [[nodiscard]] static std::string GetContentBrowserTypeFocusLabelUVE(const EditorUVE& editor) {
        return EditorUVE::GetContentBrowserFocusLabelUVE(editor.m_contentBrowserTypeFocus);
    }

    static void ReconcileContentBrowserDirectoryUVE(EditorUVE& editor, const Asset::ProjectFileSnapshotUVE& snapshot) {
        editor.ReconcileContentBrowserDirectoryUVE(snapshot);
    }

    static void LoadSessionSettingsUVE(EditorUVE& editor) { editor.LoadSessionSettingsUVE(); }
    [[nodiscard]] static bool SaveSessionSettingsUVE(EditorUVE& editor) { return editor.SaveSessionSettingsUVE(); }
    static void ApplyFocusViewportPresetUVE(EditorUVE& editor) {
        editor.ApplyLayoutPresetUVE(EditorUVE::EditorLayoutPresetUVE::FocusViewport);
    }
    static void ApplyDefaultLayoutPresetUVE(EditorUVE& editor) {
        editor.ApplyLayoutPresetUVE(EditorUVE::EditorLayoutPresetUVE::Default);
    }
    [[nodiscard]] static bool IsScenePanelVisibleUVE(const EditorUVE& editor) noexcept { return editor.m_scenePanelVisible; }
    [[nodiscard]] static bool IsInspectorPanelVisibleUVE(const EditorUVE& editor) noexcept { return editor.m_inspectorPanelVisible; }
    [[nodiscard]] static bool IsBottomDockVisibleUVE(const EditorUVE& editor) noexcept { return editor.m_bottomDockVisible; }

    [[nodiscard]] static bool ProjectWorldPointUVE(const EditorUVE& editor, const EditorViewportRectUVE& viewportRect,
                                                   const Math::Vector3UVE& worldPoint,
                                                   Math::Vector2UVE& outScreenPoint) {
        return editor.ProjectWorldPointUVE(viewportRect, worldPoint, outScreenPoint);
    }

    [[nodiscard]] static bool GetGizmoAxisWorldVectorUVE(const EditorUVE& editor, const Scene::EntityUVE entity,
                                                          const EditorTransformAxisUVE axis,
                                                          Math::Vector3UVE& outAxis) {
        return editor.GetGizmoAxisWorldVectorUVE(entity, axis, outAxis);
    }

    [[nodiscard]] static bool BeginGizmoDragUVE(EditorUVE& editor, const EditorViewportRectUVE& viewportRect,
                                                 const Math::Vector2UVE pointerPosition) {
        return editor.BeginGizmoDragUVE(viewportRect, pointerPosition);
    }

    static void UpdateGizmoDragUVE(EditorUVE& editor, const Math::Vector2UVE pointerPosition) {
        editor.UpdateGizmoDragUVE(pointerPosition);
    }

    static void CommitGizmoDragUVE(EditorUVE& editor) { editor.CommitGizmoDragUVE(); }

    [[nodiscard]] static EditorTransformAxisUVE GetGizmoDragAxisUVE(const EditorUVE& editor) noexcept {
        return editor.m_gizmoDrag.axis;
    }

    [[nodiscard]] static bool IsGizmoDragHandleKindUVE(const EditorUVE& editor,
                                                        const int expectedKind) noexcept {
        return static_cast<int>(editor.m_gizmoDrag.handleKind) == expectedKind;
    }

    [[nodiscard]] static bool ClickViewportNavigationGizmoUVE(EditorUVE& editor,
                                                               const EditorViewportRectUVE& viewportRect,
                                                               const Math::Vector2UVE pointerPosition) {
        return editor.HandleViewportNavigationGizmoClickUVE(viewportRect, pointerPosition);
    }

    [[nodiscard]] static float GetViewportYawUVE(const EditorUVE& editor) noexcept {
        return editor.m_viewportYawRadians;
    }

    static void CompileVisualScriptUVE(EditorUVE& editor) { editor.CompileVisualScriptUVE(); }

    [[nodiscard]] static bool IsVisualScriptCompileSuccessfulUVE(const EditorUVE& editor) noexcept {
        return editor.m_scriptCompileSucceeded;
    }

    [[nodiscard]] static std::uint64_t GetLastCompiledVisualScriptGraphRevisionUVE(
        const EditorUVE& editor) noexcept {
        return editor.m_scriptLastCompiledGraphRevision;
    }

    [[nodiscard]] static std::size_t GetVisualScriptCompileInstructionCountUVE(
        const EditorUVE& editor) noexcept {
        return editor.m_scriptCompileInstructionCount;
    }
};

namespace {

[[nodiscard]] Core::EngineConfigUVE MakeEditorTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_editor_tests.log";
    config.settingsFilePath = "uve_editor_tests_settings.json";
    config.assetDatabaseFilePath = "uve_editor_tests_assets.json";
    config.saveDirectoryPath = "uve_editor_tests_saves";
    config.shaderCachePath = "uve_editor_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

void AttachRootUVE(Core::EngineCoreUVE& engine, const Scene::EntityUVE entity,
                   const Scene::TransformComponentUVE& transform) {
    Core::EngineServicesUVE& services = engine.GetServicesUVE();
    services.GetSceneGraphUVE().AttachTransformUVE(services.GetEntityManagerUVE(), entity, transform);
}

struct UnregisteredEditorLifecycleComponentUVE final {
    int value = 0;
};

TEST(EditorUVETest, InitUVE_CreatesCameraOutsideDocumentRootsAndSupportsHeadlessLifecycle) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_lifecycle.uvescene");
        editor.InitUVE();

        EXPECT_EQ(editor.GetStateUVE(), EditorStateUVE::Running);
        EXPECT_TRUE(engine.GetServicesUVE().GetEntityManagerUVE().IsAliveUVE(editor.GetViewportCameraUVE()));
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());

        editor.ShutdownUVE();
        EXPECT_EQ(editor.GetStateUVE(), EditorStateUVE::Shutdown);
    }

    engine.Shutdown();
}

TEST(EditorUVETest, InitUVE_DoesNotCreateAutomaticPreviewLighting) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_no_preview_light.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        std::size_t lightCount = 0U;
        entityManager.ForEachUVE<Scene::LightComponentUVE>(
            [&lightCount](Scene::EntityUVE, Scene::LightComponentUVE&) { ++lightCount; });
        EXPECT_EQ(lightCount, 0U);
        std::size_t meshCount = 0U;
        entityManager.ForEachUVE<Scene::MeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::MeshComponentUVE&) { ++meshCount; });
        entityManager.ForEachUVE<Scene::PrimitiveMeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::PrimitiveMeshComponentUVE&) { ++meshCount; });
        EXPECT_EQ(meshCount, 0U);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.TickUVE();
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        lightCount = 0U;
        entityManager.ForEachUVE<Scene::LightComponentUVE>(
            [&lightCount](Scene::EntityUVE, Scene::LightComponentUVE&) { ++lightCount; });
        EXPECT_EQ(lightCount, 0U);
        meshCount = 0U;
        entityManager.ForEachUVE<Scene::MeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::MeshComponentUVE&) { ++meshCount; });
        entityManager.ForEachUVE<Scene::PrimitiveMeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::PrimitiveMeshComponentUVE&) { ++meshCount; });
        EXPECT_EQ(meshCount, 0U);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, PluginToolGatesUVE_AreEditorOnlyAndDoNotMutateScene) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_plugin_gates.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        EXPECT_FALSE(editor.IsControlRigPluginEnabledUVE());
        EXPECT_FALSE(editor.IsMotionQueryPluginEnabledUVE());
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.SetControlRigPluginEnabledUVE(true);
        editor.SetMotionQueryPluginEnabledUVE(true);
        EXPECT_TRUE(editor.IsControlRigPluginEnabledUVE());
        EXPECT_TRUE(editor.IsMotionQueryPluginEnabledUVE());
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        std::size_t lightCount = 0U;
        entityManager.ForEachUVE<Scene::LightComponentUVE>(
            [&lightCount](Scene::EntityUVE, Scene::LightComponentUVE&) { ++lightCount; });
        EXPECT_EQ(lightCount, 0U);
        std::size_t meshCount = 0U;
        entityManager.ForEachUVE<Scene::MeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::MeshComponentUVE&) { ++meshCount; });
        entityManager.ForEachUVE<Scene::PrimitiveMeshComponentUVE>(
            [&meshCount](Scene::EntityUVE, Scene::PrimitiveMeshComponentUVE&) { ++meshCount; });
        EXPECT_EQ(meshCount, 0U);

        editor.SetControlRigPluginEnabledUVE(false);
        editor.SetMotionQueryPluginEnabledUVE(false);
        EXPECT_FALSE(editor.IsControlRigPluginEnabledUVE());
        EXPECT_FALSE(editor.IsMotionQueryPluginEnabledUVE());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TwoDCanvasStateUVE_IsEditorOnlyAndValidated) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_2d_canvas.uvescene");
        editor.InitUVE();
        const Editor2DCanvasStateUVE initial = editor.Get2DCanvasStateUVE();
        EXPECT_FLOAT_EQ(initial.zoom, 0.36F);
        EXPECT_FLOAT_EQ(initial.pan.x, 0.0F);
        EXPECT_FLOAT_EQ(initial.pan.y, 0.0F);
        EXPECT_TRUE(initial.gridVisible);
        EXPECT_TRUE(initial.safeAreaVisible);
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        EXPECT_TRUE(editor.Set2DCanvasZoomUVE(1.25F));
        EXPECT_FLOAT_EQ(editor.Get2DCanvasStateUVE().zoom, 1.25F);
        EXPECT_FALSE(editor.Set2DCanvasZoomUVE(0.0F));
        EXPECT_FALSE(editor.Set2DCanvasZoomUVE(5.0F));
        EXPECT_FALSE(editor.Set2DCanvasZoomUVE(std::numeric_limits<float>::quiet_NaN()));
        EXPECT_FLOAT_EQ(editor.Get2DCanvasStateUVE().zoom, 1.25F);

        editor.Reset2DCanvasViewUVE();
        const Editor2DCanvasStateUVE reset = editor.Get2DCanvasStateUVE();
        EXPECT_FLOAT_EQ(reset.zoom, 0.36F);
        EXPECT_FLOAT_EQ(reset.pan.x, 0.0F);
        EXPECT_FLOAT_EQ(reset.pan.y, 0.0F);
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RenderOverlayUVE_HeadlessWorkspaceCompositionDoesNotMutateEditorState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_workspace_layout.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        editor.RenderOverlayUVE();

        EXPECT_EQ(editor.GetStateUVE(), EditorStateUVE::Running);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        ASSERT_EQ(editor.GetSelectedEntitiesUVE().size(), 1U);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE().front(), root);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, InspectorDrawerRegistrationUVE_IncludesStableHierarchyDrawer) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_hierarchy_drawer_registration.uvescene");
        EXPECT_EQ(EditorUVEAccessUVE::GetInspectorDrawerCountUVE(editor), 15U);
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "name"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "hierarchy"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "transform"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "primitive-mesh"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "camera"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "mesh"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "light"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "collider"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "rigid-body"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "audio-source"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "particle-emitter"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "script"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "animation-player"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "world-environment"));
        EXPECT_TRUE(EditorUVEAccessUVE::HasInspectorDrawerUVE(editor, "prefab-instance"));
        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, WorldEnvironmentComponentUVE_AttachEditUndoRedoThroughEditorPath) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_world_environment.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE entity =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Empty);
        ASSERT_NE(entity, Scene::kInvalidEntityUVE);
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity));

        Scene::WorldEnvironment3DNodeComponentUVE environment;
        environment.skyAssetPath = "environments/sunset.uvesky";
        environment.ambientColor = Math::Vector3UVE{0.15F, 0.25F, 0.40F};
        environment.ambientEnergy = 1.75F;
        environment.exposure = 1.25F;
        environment.fogColor = Math::Vector3UVE{0.30F, 0.35F, 0.45F};
        environment.fogDensity = 0.02F;
        environment.fogEnabled = true;
        environment.postProcessingEnabled = true;
        ASSERT_TRUE(Scene::IsWorldEnvironment3DNodeComponentValidUVE(environment));
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::WorldEnvironment, environment));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity).skyAssetPath,
                  environment.skyAssetPath);
        EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity).ambientEnergy,
                        environment.ambientEnergy);
        EXPECT_TRUE(editor.CanUndoUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity));
        ASSERT_TRUE(editor.RedoUVE());
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity).fogEnabled,
                  environment.fogEnabled);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, OutlinerContextUVE_UsesFixedSpecializedTagPriority) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_outliner_tags.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE plain = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, plain, Scene::TransformComponentUVE{});
        EXPECT_TRUE(EditorUVEAccessUVE::GetOutlinerTypeTagUVE(editor, plain).empty());

        const Scene::EntityUVE collider = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, collider, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(collider, Scene::ColliderComponentUVE{});
        EXPECT_EQ(EditorUVEAccessUVE::GetOutlinerTypeTagUVE(editor, collider), "Collision Box");

        const Scene::EntityUVE light = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, light, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::LightComponentUVE>(light, Scene::LightComponentUVE{});
        EXPECT_EQ(EditorUVEAccessUVE::GetOutlinerTypeTagUVE(editor, light), "Directional Light");

        const Scene::EntityUVE camera = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, camera, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::CameraComponentUVE>(camera, Scene::CameraComponentUVE{});
        EXPECT_EQ(EditorUVEAccessUVE::GetOutlinerTypeTagUVE(editor, camera), "Camera");

        const Scene::EntityUVE primitive = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, primitive, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(
            primitive, Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::Plane, {0.4F, 0.5F, 0.6F}});
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(primitive, Scene::ColliderComponentUVE{});
        entityManager.AddComponentUVE<Scene::CameraComponentUVE>(primitive, Scene::CameraComponentUVE{});
        EXPECT_EQ(EditorUVEAccessUVE::GetOutlinerTypeTagUVE(editor, primitive), "Plane");

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, OutlinerContextUVE_AncestryAndEligibleParentsExcludeSelectedSubtree) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_outliner_hierarchy_context.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE rootA = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, rootA, Scene::TransformComponentUVE{});
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE selected = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, selected, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, selected, parent);
        const Scene::EntityUVE descendant = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, descendant, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, descendant, selected);
        const Scene::EntityUVE rootB = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, rootB, Scene::TransformComponentUVE{});

        EXPECT_EQ(EditorUVEAccessUVE::GetDocumentAncestryUVE(editor, selected),
                  (std::vector<Scene::EntityUVE>{parent, selected}));
        EXPECT_EQ(EditorUVEAccessUVE::GetEligibleReparentParentsUVE(editor, selected),
                  (std::vector<Scene::EntityUVE>{rootA, parent, rootB}));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ContentBrowserWorkflowUVE_UsesPrimaryExtensionTagAndIndependentRegisteredFocus) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_content_browser_tags.uvescene");
        Asset::ProjectFileEntryUVE registeredMesh;
        registeredMesh.relativePath = "Characters/Hero.UVEMESH";
        registeredMesh.kind = Asset::ProjectFileEntryKindUVE::File;
        registeredMesh.registeredAssetGuid = Asset::AssetGuidUVE{42U};

        Asset::ProjectFileEntryUVE ordinaryFile;
        ordinaryFile.relativePath = "Notes/readme.txt";
        ordinaryFile.kind = Asset::ProjectFileEntryKindUVE::File;

        Asset::ProjectFileEntryUVE motionQuery;
        motionQuery.relativePath = "Queries/Locomotion.UVEMOTIONQUERY";
        motionQuery.kind = Asset::ProjectFileEntryKindUVE::File;
        motionQuery.registeredAssetGuid = Asset::AssetGuidUVE{84U};

        Asset::ProjectFileEntryUVE directory;
        directory.relativePath = "Characters";
        directory.kind = Asset::ProjectFileEntryKindUVE::Directory;

        // A registered file keeps its semantic extension tag; registration remains an independent badge/focus.
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserItemTypeLabelUVE(registeredMesh), "Mesh");
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserItemTypeLabelUVE(ordinaryFile), "File");
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserItemTypeLabelUVE(motionQuery), "Motion Query");
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserItemTypeLabelUVE(directory), "Folder");

        EditorUVEAccessUVE::SelectContentBrowserMotionQueryFocusUVE(editor);
        EXPECT_TRUE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, motionQuery));
        EXPECT_FALSE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, registeredMesh));
        EXPECT_FALSE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, ordinaryFile));

        EditorUVEAccessUVE::SelectContentBrowserMeshFocusUVE(editor);
        EXPECT_TRUE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, registeredMesh));
        EXPECT_FALSE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, ordinaryFile));

        EditorUVEAccessUVE::SelectContentBrowserRegisteredFocusUVE(editor);
        EXPECT_TRUE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, registeredMesh));
        EXPECT_FALSE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, ordinaryFile));
        EXPECT_FALSE(EditorUVEAccessUVE::DoesContentBrowserEntryMatchFocusUVE(editor, directory));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ContentBrowserWorkflowUVE_PersistsFiltersAndSafelyFallsBackWhenFolderDisappears) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_content_browser_navigation.uvescene");
        Asset::ProjectFileSnapshotUVE snapshot;
        snapshot.entries.push_back(
            Asset::ProjectFileEntryUVE{std::filesystem::path{"Scenes"}, Asset::ProjectFileEntryKindUVE::Directory, std::nullopt});
        snapshot.entries.push_back(Asset::ProjectFileEntryUVE{std::filesystem::path{"Scenes/City.uvescene"},
                                                               Asset::ProjectFileEntryKindUVE::File, std::nullopt});

        EditorUVEAccessUVE::SetAssetFilterUVE(editor, "city");
        EditorUVEAccessUVE::SelectContentBrowserMeshFocusUVE(editor);
        EditorUVEAccessUVE::SetContentBrowserDirectoryUVE(editor, "Scenes");
        EditorUVEAccessUVE::ReconcileContentBrowserDirectoryUVE(editor, snapshot);

        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserDirectoryUVE(editor), std::filesystem::path{"Scenes"});
        EXPECT_EQ(editor.GetAssetFilterUVE(), "city");
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserTypeFocusLabelUVE(editor), "Mesh");

        snapshot.entries.clear();
        EditorUVEAccessUVE::ReconcileContentBrowserDirectoryUVE(editor, snapshot);
        EXPECT_TRUE(EditorUVEAccessUVE::GetContentBrowserDirectoryUVE(editor).empty());
        EXPECT_EQ(editor.GetAssetFilterUVE(), "city");
        EXPECT_EQ(EditorUVEAccessUVE::GetContentBrowserTypeFocusLabelUVE(editor), "Mesh");

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ContentBrowserAutoRefreshUVE_RefreshesAfterEngineWatcherSequenceAndAcknowledgesIt) {
    const std::filesystem::path root = "uve_editor_tests_auto_refresh_content";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Scenes");
    {
        std::ofstream initialFile(root / "Scenes" / "Initial.txt", std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(initialFile.is_open());
        initialFile << "initial";
        ASSERT_TRUE(initialFile.good());
    }

    Core::EngineConfigUVE config = MakeEditorTestConfigUVE();
    config.projectContentRootUVE = root;
    config.projectChangeWatchPollIntervalSecondsUVE = 0.0;
    config.projectChangeJournalCapacityUVE = 16U;
    Core::EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_content_browser_auto_refresh.uvescene");
        editor.InitUVE();

        engine.TickFrameUVE();
        editor.TickUVE();
        Asset::IProjectFileIndexUVE& projectFileIndex = engine.GetServicesUVE().GetProjectFileIndexUVE();
        Asset::ProjectFileSnapshotUVE initialSnapshot = projectFileIndex.GetSnapshotUVE();
        ASSERT_EQ(initialSnapshot.refreshGeneration, 1U);
        ASSERT_TRUE(std::any_of(initialSnapshot.entries.begin(), initialSnapshot.entries.end(),
                                [](const Asset::ProjectFileEntryUVE& entry) {
                                    return entry.relativePath == std::filesystem::path{"Scenes/Initial.txt"};
                                }));
        EXPECT_TRUE(engine.GetServicesUVE().GetProjectChangeWatcherUVE().GetSnapshotUVE().changes.empty());

        {
            std::ofstream newFile(root / "Scenes" / "AutoRefresh.txt", std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(newFile.is_open());
            newFile << "created after baseline";
            ASSERT_TRUE(newFile.good());
        }

        engine.TickFrameUVE();
        editor.TickUVE();
        const Asset::ProjectFileSnapshotUVE refreshedSnapshot = projectFileIndex.GetSnapshotUVE();
        ASSERT_GT(refreshedSnapshot.refreshGeneration, initialSnapshot.refreshGeneration);
        EXPECT_TRUE(std::any_of(refreshedSnapshot.entries.begin(), refreshedSnapshot.entries.end(),
                                [](const Asset::ProjectFileEntryUVE& entry) {
                                    return entry.relativePath == std::filesystem::path{"Scenes/AutoRefresh.txt"};
                                }));
        const Asset::ProjectChangeSnapshotUVE changeSnapshot =
            engine.GetServicesUVE().GetProjectChangeWatcherUVE().GetSnapshotUVE();
        EXPECT_TRUE(changeSnapshot.changes.empty());
        EXPECT_FALSE(changeSnapshot.rescanRequired);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
    std::filesystem::remove_all(root);
}

TEST(EditorUVETest, SessionSettingsUVE_MigratesWithoutHiddenWriteAndPreservesDocumentState) {
    const Core::EngineConfigUVE config = MakeEditorTestConfigUVE();
    std::filesystem::remove(config.settingsFilePath);
    Core::EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Config::IConfigManagerUVE& settings = services.GetConfigManagerUVE();
        settings.SetIntUVE("editor.sessionSettingsVersion", 0);
        settings.SetBoolUVE("editor.panels.sceneVisible", false);
        settings.SetBoolUVE("editor.panels.inspectorVisible", false);
        settings.SetBoolUVE("editor.panels.bottomDockVisible", false);
        settings.SetBoolUVE("editor.viewport.snap.enabled", true);
        settings.SetDoubleUVE("editor.viewport.snap.translateStep", -1.0);
        settings.SetDoubleUVE("editor.viewport.snap.rotateStepDegrees", 45.0);
        settings.SetDoubleUVE("editor.viewport.snap.scaleStep", 0.25);

        EditorUVE editor(services, "uve_editor_tests_session_settings.uvescene");
        editor.InitUVE();
        EXPECT_FALSE(EditorUVEAccessUVE::IsScenePanelVisibleUVE(editor));
        EXPECT_FALSE(EditorUVEAccessUVE::IsInspectorPanelVisibleUVE(editor));
        EXPECT_FALSE(EditorUVEAccessUVE::IsBottomDockVisibleUVE(editor));
        EXPECT_FALSE(settings.HasKeyUVE("editor.workspace.active"));
        const EditorTransformSnappingSettingsUVE& snapping = editor.GetTransformSnappingSettingsUVE();
        EXPECT_TRUE(snapping.enabled);
        EXPECT_FLOAT_EQ(snapping.translateStep, 1.0F);
        EXPECT_FLOAT_EQ(snapping.rotateStepDegrees, 45.0F);
        EXPECT_FLOAT_EQ(snapping.scaleStep, 0.25F);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.GetSelectedEntitiesUVE().empty());

        EditorUVEAccessUVE::ApplyDefaultLayoutPresetUVE(editor);
        EXPECT_TRUE(EditorUVEAccessUVE::IsScenePanelVisibleUVE(editor));
        EXPECT_TRUE(EditorUVEAccessUVE::IsInspectorPanelVisibleUVE(editor));
        EXPECT_TRUE(EditorUVEAccessUVE::IsBottomDockVisibleUVE(editor));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.GetSelectedEntitiesUVE().empty());

        ASSERT_TRUE(EditorUVEAccessUVE::SaveSessionSettingsUVE(editor));
        EXPECT_EQ(settings.GetIntUVE("editor.sessionSettingsVersion", -1), 1);
        EXPECT_TRUE(settings.HasKeyUVE("editor.workspace.active"));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        editor.ShutdownUVE();
    }

    engine.Shutdown();
    std::filesystem::remove(config.settingsFilePath);
}

TEST(EditorUVETest, SelectionAndInspectorTransformEdit_ValidateLifetimeAndFiniteValues) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_selection.uvescene");
        editor.InitUVE();

        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        editor.SelectEntityUVE(root);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);

        Scene::TransformComponentUVE edited{};
        edited.localPosition = Math::Vector3UVE{3.0F, -2.0F, 7.0F};
        edited.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(edited));
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_EQ(services.GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  edited.localPosition);

        edited.localScale.x = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(editor.SetSelectedLocalTransformUVE(edited));

        services.GetEntityManagerUVE().DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, MultiSelectionUVE_ToggleMaintainsOrderActiveFallbackAndSingleCommandSafety) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_multi_selection.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE first = entityManager.CreateEntityUVE();
        const Scene::EntityUVE second = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, first, Scene::TransformComponentUVE{});
        AttachRootUVE(engine, second, Scene::TransformComponentUVE{});

        editor.SelectEntityUVE(first);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), std::vector<Scene::EntityUVE>{first});
        EXPECT_EQ(editor.GetSelectedEntityUVE(), first);
        EXPECT_TRUE(editor.HasSingleDocumentSelectionUVE());

        editor.ToggleEntitySelectionUVE(second);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), (std::vector<Scene::EntityUVE>{first, second}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), second);
        EXPECT_FALSE(editor.HasSingleDocumentSelectionUVE());

        const Scene::TransformComponentUVE before =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(second);
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 1.0F));
        EXPECT_FALSE(editor.DuplicateSelectedEntityUVE() != Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(second).localPosition,
                  before.localPosition);

        editor.ToggleEntitySelectionUVE(second);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), std::vector<Scene::EntityUVE>{first});
        EXPECT_EQ(editor.GetSelectedEntityUVE(), first);
        EXPECT_TRUE(editor.HasSingleDocumentSelectionUVE());

        editor.ToggleEntitySelectionUVE(second);
        editor.ToggleEntitySelectionUVE(first);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), std::vector<Scene::EntityUVE>{second});
        EXPECT_EQ(editor.GetSelectedEntityUVE(), second);

        editor.ToggleEntitySelectionUVE(second);
        EXPECT_TRUE(editor.GetSelectedEntitiesUVE().empty());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.HasSingleDocumentSelectionUVE());

        editor.ToggleEntitySelectionUVE(editor.GetViewportCameraUVE());
        EXPECT_TRUE(editor.GetSelectedEntitiesUVE().empty());
        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, MultiSelectionUVE_TickPrunesStaleEntitiesAndPromotesLastLiveSelection) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_multi_selection_stale.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE first = entityManager.CreateEntityUVE();
        const Scene::EntityUVE second = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, first, Scene::TransformComponentUVE{});
        AttachRootUVE(engine, second, Scene::TransformComponentUVE{});

        editor.SelectEntityUVE(first);
        editor.ToggleEntitySelectionUVE(second);
        entityManager.DestroyEntityUVE(second);
        editor.TickUVE();
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), std::vector<Scene::EntityUVE>{first});
        EXPECT_EQ(editor.GetSelectedEntityUVE(), first);
        EXPECT_TRUE(editor.HasSingleDocumentSelectionUVE());

        entityManager.DestroyEntityUVE(first);
        editor.TickUVE();
        EXPECT_TRUE(editor.GetSelectedEntitiesUVE().empty());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_CreatesSelectedDirtyRootArchetypes) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_entities.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE empty = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty);
        ASSERT_TRUE(entityManager.IsAliveUVE(empty));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::LightComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(empty));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(empty));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(empty).name, "Empty");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), empty);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE camera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        ASSERT_TRUE(entityManager.IsAliveUVE(camera));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(camera));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(camera));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(camera));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(camera).name, "Camera");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), camera);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE directionalLight =
            editor.CreateDocumentEntityUVE(EditorEntityKindUVE::DirectionalLight);
        ASSERT_TRUE(entityManager.IsAliveUVE(directionalLight));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(directionalLight));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::LightComponentUVE>(directionalLight));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::LightComponentUVE>(directionalLight).type,
                  Scene::LightTypeUVE::Directional);
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(directionalLight));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(directionalLight).name, "Directional Light");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), directionalLight);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE collisionBox = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::CollisionBox);
        ASSERT_TRUE(entityManager.IsAliveUVE(collisionBox));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(collisionBox));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(collisionBox));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(collisionBox));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(collisionBox).name, "Collision Box");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), collisionBox);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE cube = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_TRUE(entityManager.IsAliveUVE(cube));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(cube));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).kind,
                  Scene::PrimitiveMeshKindUVE::Cube);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(cube).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.5F, 0.5F}));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(cube).name, "Cube");

        const Scene::EntityUVE sphere = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::UVSphere);
        ASSERT_TRUE(entityManager.IsAliveUVE(sphere));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(sphere));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(sphere));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(sphere).kind,
                  Scene::PrimitiveMeshKindUVE::UVSphere);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(sphere).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.5F, 0.5F}));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(sphere).name, "UV Sphere");

        const Scene::EntityUVE plane = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Plane);
        ASSERT_TRUE(entityManager.IsAliveUVE(plane));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(plane));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(plane));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(plane).kind,
                  Scene::PrimitiveMeshKindUVE::Plane);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(plane).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.025F, 0.5F}));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(plane).name, "Plane");

        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 7U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), empty), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), camera), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), directionalLight), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), collisionBox), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), cube), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), sphere), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), plane), roots.end());
        EXPECT_EQ(std::find(roots.begin(), roots.end(), editor.GetViewportCameraUVE()), roots.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, PrimitiveAppearanceUVE_UpdatesColliderAndSupportsAtomicUndoRedo) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_primitive_appearance.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE cube = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_TRUE(entityManager.IsAliveUVE(cube));
        const Scene::PrimitiveMeshComponentUVE before =
            entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube);

        const Scene::PrimitiveMeshComponentUVE after{Scene::PrimitiveMeshKindUVE::Plane,
                                                     Math::Vector3UVE{0.1F, 0.4F, 0.9F}};
        ASSERT_TRUE(editor.SetSelectedPrimitiveMeshUVE(after));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).kind, after.kind);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).baseColor, after.baseColor);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(cube).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.025F, 0.5F}));

        const Scene::PrimitiveMeshComponentUVE invalid{static_cast<Scene::PrimitiveMeshKindUVE>(99),
                                                       Math::Vector3UVE{0.1F, 0.4F, 0.9F}};
        EXPECT_FALSE(editor.SetSelectedPrimitiveMeshUVE(invalid));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).kind, after.kind);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).baseColor, after.baseColor);

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).kind, before.kind);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).baseColor, before.baseColor);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(cube).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.5F, 0.5F}));
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).kind, after.kind);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(cube).baseColor, after.baseColor);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(cube).halfExtents,
                  (Math::Vector3UVE{0.5F, 0.025F, 0.5F}));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_RejectsInvalidKindsAndNonRunningStates) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_invalid.uvescene");
        EXPECT_EQ(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);

        editor.InitUVE();
        EXPECT_EQ(editor.CreateDocumentEntityUVE(static_cast<EditorEntityKindUVE>(999)),
                  Scene::kInvalidEntityUVE);
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
        EXPECT_EQ(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_AllocatesUniqueNamesAndKeepsEditorCameraOutOfNamespace) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_names.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE firstCamera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        const Scene::EntityUVE secondCamera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(firstCamera));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(secondCamera));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(firstCamera).name, "Camera");
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(secondCamera).name, "Camera 2");
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(editor.GetViewportCameraUVE()));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, SetSelectedEntityNameUVE_ValidatesInputAndMarksDocumentDirty) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rename.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Unselected"));
        editor.SelectEntityUVE(root);
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Gameplay Root"));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Gameplay Root");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Gameplay Root"));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE(""));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("   \t"));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE(std::string(97U, 'n')));

        entityManager.DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Destroyed"));
        editor.ShutdownUVE();
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Shutdown"));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_TransformUndoRedoRestoresSelectionAndDirtyState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_transform.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE moved{};
        moved.localPosition = Math::Vector3UVE{4.0F, -3.0F, 2.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(moved));
        EXPECT_TRUE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  Math::Vector3UVE{});
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_TRUE(editor.CanRedoUVE());

        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  moved.localPosition);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NameUndoRedoRestoresOptionalComponentState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_name.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Level Root"));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Level Root");
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Level Root");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_CreationUndoRedoRecreatesArchetypeAndName) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_create.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE created = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::CollisionBox);
        ASSERT_TRUE(entityManager.IsAliveUVE(created));
        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(created));
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.RedoUVE());
        const Scene::EntityUVE recreated = editor.GetSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(recreated));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(recreated));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(recreated));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(recreated));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(recreated).name, "Collision Box");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NewMutationClearsRedoAndCapacityDiscardsOldestCommand) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_capacity.uvescene", 1U);
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE first{};
        first.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(first));
        Scene::TransformComponentUVE second = first;
        second.localPosition = Math::Vector3UVE{2.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(second));
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  first.localPosition);
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_TRUE(editor.CanRedoUVE());

        Scene::TransformComponentUVE third = first;
        third.localPosition = Math::Vector3UVE{3.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(third));
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_FALSE(editor.SetSelectedLocalTransformUVE(third));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_StaleTargetsAndNonRunningStateFailWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_stale.uvescene");
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.RedoUVE());
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE moved{};
        moved.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(moved));
        entityManager.DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.RedoUVE());
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DuplicateSelectedEntityUVE_RootCreatesNamedSiblingWithCopiedComponents) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE sourceTransform{};
        sourceTransform.localPosition = Math::Vector3UVE{2.0F, 4.0F, 6.0F};
        AttachRootUVE(engine, source, sourceTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(source);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Lamp"});
        editor.SelectEntityUVE(source);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        EXPECT_NE(duplicate, source);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), duplicate);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(duplicate).name, "Lamp 2");
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(duplicate).localPosition,
                  sourceTransform.localPosition);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(duplicate));

        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 2U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), source), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), duplicate), roots.end());
        EXPECT_EQ(std::find(roots.begin(), roots.end(), editor.GetViewportCameraUVE()), roots.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DuplicateSelectedEntityUVE_ChildRestoresAsSiblingUnderSameParent) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_child.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(child, Scene::NameComponentUVE{"Child"});
        editor.SelectEntityUVE(child);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(duplicate).name, "Child 2");
        const std::vector<Scene::EntityUVE> children =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent);
        ASSERT_EQ(children.size(), 2U);
        EXPECT_NE(std::find(children.begin(), children.end(), child), children.end());
        EXPECT_NE(std::find(children.begin(), children.end(), duplicate), children.end());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), duplicate);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DeleteSelectedEntityUVE_DeletesSubtreeAndSelectsLiveParent) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_subtree.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        const Scene::EntityUVE grandchild = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, grandchild, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, grandchild, child);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(child));
        EXPECT_FALSE(entityManager.IsAliveUVE(grandchild));
        EXPECT_TRUE(entityManager.IsAliveUVE(parent));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.CanUndoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DeleteSelectedEntityUVE_RootClearsSelection) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_root.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(root));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DuplicateUndoRedoUsesFreshHandlesAndRestoresDirtySelection) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_history.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, source, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Actor"});
        editor.SelectEntityUVE(source);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(duplicate));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), source);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        const Scene::EntityUVE recreated = editor.GetSelectedEntityUVE();
        EXPECT_TRUE(entityManager.IsAliveUVE(recreated));
        EXPECT_NE(recreated, duplicate);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(recreated).name, "Actor 2");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DeleteUndoRedoRestoresSubtreeUnderOriginalParentWithFreshHandles) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_history.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(child, Scene::NameComponentUVE{"Deleted Child"});
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(child));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        ASSERT_TRUE(editor.UndoUVE());
        const Scene::EntityUVE restored = editor.GetSelectedEntityUVE();
        EXPECT_TRUE(entityManager.IsAliveUVE(restored));
        EXPECT_NE(restored, child);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(restored).name, "Deleted Child");
        const std::vector<Scene::EntityUVE> children = services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent);
        EXPECT_NE(std::find(children.begin(), children.end(), restored), children.end());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(restored));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DeleteUndoRejectsStaleParentAndClearsTimeline) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_stale_parent.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        entityManager.DestroyEntityUVE(parent);
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_EQ(editor.GetDocumentRootsUVE().size(), 0U);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NewMutationAfterDuplicateUndoInvalidatesRedo) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_redo_invalidation.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, source, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Source"});
        editor.SelectEntityUVE(source);

        ASSERT_NE(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.CanRedoUVE());
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Source Revised"));
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EntityLifecycleUVE_RejectsUnselectedCameraStaleNonRunningAndUnsupportedCapture) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_lifecycle_safety.uvescene");
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.InitUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.SelectEntityUVE(editor.GetViewportCameraUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());

        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE unsupported = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, unsupported, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<UnregisteredEditorLifecycleComponentUVE>(
            unsupported, UnregisteredEditorLifecycleComponentUVE{7});
        editor.SelectEntityUVE(unsupported);
        const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.IsAliveUVE(unsupported));
        EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        EXPECT_TRUE(entityManager.IsAliveUVE(unsupported));

        entityManager.DestroyEntityUVE(unsupported);
        editor.TickUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.ShutdownUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_RootMovesBelowTargetAndPreservesLocalTransform) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE target = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, target, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE localTransform{};
        localTransform.localPosition = Math::Vector3UVE{2.0F, -3.0F, 7.0F};
        AttachRootUVE(engine, moved, localTransform);
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(target));
        const std::vector<Scene::EntityUVE> targetChildren =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, target);
        EXPECT_NE(std::find(targetChildren.begin(), targetChildren.end(), moved), targetChildren.end());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(moved).localPosition,
                  localTransform.localPosition);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), moved);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.CanUndoUVE());
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 1U);
        EXPECT_EQ(roots.front(), target);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_ChildCanReturnToRootWithoutDetachingDescendants) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_root_detach.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        const Scene::EntityUVE grandchild = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, grandchild, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, grandchild, child);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 2U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), parent), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), child), roots.end());
        EXPECT_TRUE(services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent).empty());
        const std::vector<Scene::EntityUVE> childChildren =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, child);
        EXPECT_NE(std::find(childChildren.begin(), childChildren.end(), grandchild), childChildren.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_ReparentUndoRedoRestoresParentsSelectionAndDirtyState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_history.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE oldParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, oldParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE newParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, newParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, moved, oldParent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(moved, Scene::NameComponentUVE{"Moved"});
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(newParent));
        const std::vector<Scene::EntityUVE> childrenAfterReparent =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, newParent);
        EXPECT_NE(std::find(childrenAfterReparent.begin(), childrenAfterReparent.end(), moved),
                  childrenAfterReparent.end());
        ASSERT_TRUE(editor.UndoUVE());
        const std::vector<Scene::EntityUVE> childrenAfterUndo =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, oldParent);
        EXPECT_NE(std::find(childrenAfterUndo.begin(), childrenAfterUndo.end(), moved), childrenAfterUndo.end());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), moved);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        const std::vector<Scene::EntityUVE> childrenAfterRedo =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, newParent);
        EXPECT_NE(std::find(childrenAfterRedo.begin(), childrenAfterRedo.end(), moved), childrenAfterRedo.end());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Moved Again"));
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_RejectsCyclesNoOpCameraStaleAndNonRunningStates) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_safety.uvescene");
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, root);
        editor.SelectEntityUVE(root);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(child));
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.SelectEntityUVE(child);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));
        editor.SelectEntityUVE(editor.GetViewportCameraUVE());
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));

        const Scene::EntityUVE staleTarget = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, staleTarget, Scene::TransformComponentUVE{});
        entityManager.DestroyEntityUVE(staleTarget);
        editor.SelectEntityUVE(root);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(staleTarget));

        const Scene::EntityUVE malformed = entityManager.CreateEntityUVE();
        editor.SelectEntityUVE(malformed);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.ShutdownUVE();
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_ReparentUndoRejectsStalePriorParentAndClearsTimeline) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_stale_parent.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE oldParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, oldParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE newParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, newParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, moved, oldParent);
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(newParent));
        entityManager.DestroyEntityUVE(oldParent);
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, KeepWorldReparentUVE_PreservesCompatibleWorldTrsAndHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_keep_world_reparent.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localPosition = Math::Vector3UVE{10.0F, -2.0F, 5.0F};
        parentTransform.localScale = Math::Vector3UVE{2.0F, 2.0F, 2.0F};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 1.0F, 0.0F},
                                              std::numbers::pi_v<float> * 0.5F,
                                              parentTransform.localRotation));
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, parentTransform);
        Scene::TransformComponentUVE movedTransform{};
        movedTransform.localPosition = Math::Vector3UVE{4.0F, 3.0F, -2.0F};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                              std::numbers::pi_v<float> * 0.25F,
                                              movedTransform.localRotation));
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, movedTransform);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        const Scene::WorldTransformComponentUVE worldBefore =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(moved);
        editor.SelectEntityUVE(moved);
        ASSERT_TRUE(editor.SetReparentTransformModeUVE(EditorReparentTransformModeUVE::KeepWorld));
        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(parent));
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        const Scene::WorldTransformComponentUVE& worldAfter =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(moved);
        EXPECT_NEAR(worldAfter.worldPosition.x, worldBefore.worldPosition.x, 0.0001F);
        EXPECT_NEAR(worldAfter.worldPosition.y, worldBefore.worldPosition.y, 0.0001F);
        EXPECT_NEAR(worldAfter.worldPosition.z, worldBefore.worldPosition.z, 0.0001F);
        EXPECT_NEAR(worldAfter.worldScale.x, worldBefore.worldScale.x, 0.0001F);
        EXPECT_NEAR(worldAfter.worldScale.y, worldBefore.worldScale.y, 0.0001F);
        EXPECT_NEAR(worldAfter.worldScale.z, worldBefore.worldScale.z, 0.0001F);
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(moved).localPosition,
                  movedTransform.localPosition);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NE(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(moved).localPosition,
                  movedTransform.localPosition);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, KeepWorldReparentUVE_RejectsShearProneAndNearZeroScaleParents) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_keep_world_reject.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        Scene::TransformComponentUVE shearParentTransform{};
        shearParentTransform.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 0.0F, 1.0F},
                                              std::numbers::pi_v<float> * 0.25F,
                                              shearParentTransform.localRotation));
        const Scene::EntityUVE shearParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, shearParent, shearParentTransform);
        Scene::TransformComponentUVE movedTransform{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                              std::numbers::pi_v<float> * 0.25F,
                                              movedTransform.localRotation));
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, movedTransform);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(moved);
        ASSERT_TRUE(editor.SetReparentTransformModeUVE(EditorReparentTransformModeUVE::KeepWorld));
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(shearParent));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(moved).localPosition,
                  movedTransform.localPosition);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        Scene::TransformComponentUVE tinyParentTransform{};
        tinyParentTransform.localScale = Math::Vector3UVE{0.0001F, 1.0F, 1.0F};
        const Scene::EntityUVE tinyParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, tinyParent, tinyParentTransform);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(tinyParent));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, SaveThenLoadScene_RoundTripsDocumentRootsWithoutSerializingEditorCamera) {
    const std::filesystem::path scenePath = "uve_editor_tests_round_trip.uvescene";
    std::filesystem::remove(scenePath);
    std::filesystem::remove(scenePath.string() + ".editor-recovery");

    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), scenePath);
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();

        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        Scene::TransformComponentUVE rootTransform{};
        rootTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        const Scene::EntityUVE child = services.GetEntityManagerUVE().CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{4.0F, 5.0F, 6.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(services.GetEntityManagerUVE(), child, root);

        editor.SelectEntityUVE(root);
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(rootTransform));
        ASSERT_TRUE(editor.SaveSceneUVE());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(std::filesystem::exists(scenePath));

        Scene::TransformComponentUVE modified = rootTransform;
        modified.localPosition = Math::Vector3UVE{9.0F, 9.0F, 9.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(modified));
        ASSERT_TRUE(editor.LoadSceneUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        const std::vector<Scene::EntityUVE> loadedRoots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(loadedRoots.size(), 1U);
        EXPECT_NE(loadedRoots.front(), editor.GetViewportCameraUVE());
        const Scene::TransformComponentUVE& loadedTransform =
            services.GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(loadedRoots.front());
        EXPECT_EQ(loadedTransform.localPosition, rootTransform.localPosition);
        EXPECT_EQ(services.GetSceneGraphUVE().GetChildrenUVE(services.GetEntityManagerUVE(), loadedRoots.front()).size(),
                  1U);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
    std::filesystem::remove(scenePath);
    std::filesystem::remove(scenePath.string() + ".editor-recovery");
}

TEST(EditorUVETest, ViewportNavigationUVE_FocusOrbitPanZoomPreserveDocumentAndHistoryState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE rootTransform{};
        rootTransform.localPosition = Math::Vector3UVE{3.0F, 2.0F, -4.0F};
        AttachRootUVE(engine, root, rootTransform);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(root);

        const Scene::TransformComponentUVE documentBefore =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root);
        const Scene::TransformComponentUVE cameraBefore =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(editor.GetViewportCameraUVE());
        ASSERT_TRUE(editor.FocusSelectedEntityUVE());
        EXPECT_EQ(editor.GetViewportFocusPointUVE(), rootTransform.localPosition);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        ASSERT_TRUE(editor.OrbitViewportUVE(0.5F, 0.25F));
        const Scene::TransformComponentUVE cameraAfterOrbit =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(editor.GetViewportCameraUVE());
        EXPECT_NE(cameraAfterOrbit.localPosition, cameraBefore.localPosition);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  documentBefore.localPosition);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());

        const EditorViewportRectUVE viewportRect{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        const Math::Vector3UVE focusBeforePan = editor.GetViewportFocusPointUVE();
        ASSERT_TRUE(editor.PanViewportUVE(Math::Vector2UVE{120.0F, -40.0F}, viewportRect));
        EXPECT_NE(editor.GetViewportFocusPointUVE(), focusBeforePan);
        const float distanceBeforeZoom = editor.GetViewportDistanceUVE();
        ASSERT_TRUE(editor.ZoomViewportUVE(2.0F));
        EXPECT_LT(editor.GetViewportDistanceUVE(), distanceBeforeZoom);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_EQ(editor.GetViewportNavigationModeUVE(), EditorViewportNavigationModeUVE::None);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ViewportNavigationUVE_ValidatesSelectionInputAndDistanceLimits) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation_invalid.uvescene");
        EXPECT_FALSE(editor.FocusSelectedEntityUVE());
        EXPECT_FALSE(editor.OrbitViewportUVE(0.0F, 0.0F));
        editor.InitUVE();
        const EditorViewportRectUVE validViewport{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        EXPECT_FALSE(editor.FocusSelectedEntityUVE());
        EXPECT_FALSE(editor.OrbitViewportUVE(std::numeric_limits<float>::infinity(), 0.0F));
        EXPECT_FALSE(editor.PanViewportUVE(Math::Vector2UVE{0.0F, 1.0F},
                                           EditorViewportRectUVE{Math::Vector2UVE{}, Math::Vector2UVE{1.0F, 1.0F}}));
        EXPECT_FALSE(editor.ZoomViewportUVE(std::numeric_limits<float>::quiet_NaN()));

        ASSERT_TRUE(editor.ZoomViewportUVE(100.0F));
        EXPECT_GE(editor.GetViewportDistanceUVE(), 0.5F);
        EXPECT_LE(editor.GetViewportDistanceUVE(), 500.0F);
        EXPECT_FALSE(editor.ZoomViewportUVE(100.0F));
        ASSERT_TRUE(editor.ZoomViewportUVE(-100.0F));
        EXPECT_GE(editor.GetViewportDistanceUVE(), 0.5F);
        EXPECT_LE(editor.GetViewportDistanceUVE(), 500.0F);
        EXPECT_FALSE(editor.ZoomViewportUVE(-100.0F));
        EXPECT_TRUE(editor.PanViewportUVE(Math::Vector2UVE{0.0F, 0.0F}, validViewport));

        editor.ShutdownUVE();
        EXPECT_FALSE(editor.OrbitViewportUVE(0.1F, 0.1F));
        EXPECT_FALSE(editor.ZoomViewportUVE(1.0F));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ViewportNavigationUVE_DoesNotInterfereWithDocumentHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation_history.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE edited{};
        edited.localPosition = Math::Vector3UVE{2.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(edited));
        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.OrbitViewportUVE(0.3F, -0.2F));
        ASSERT_TRUE(editor.ZoomViewportUVE(1.0F));
        EXPECT_TRUE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  Math::Vector3UVE{});
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ViewportRayAndColliderPicking_SelectClosestDocumentEntityAndClearOnMiss) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_picking.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE nearEntity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE nearTransform{};
        nearTransform.localPosition = Math::Vector3UVE{0.0F, 1.5F, 2.0F};
        AttachRootUVE(engine, nearEntity, nearTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(nearEntity);

        const Scene::EntityUVE farEntity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE farTransform{};
        farTransform.localPosition = Math::Vector3UVE{0.0F, 1.5F, -2.0F};
        AttachRootUVE(engine, farEntity, farTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(farEntity);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        const EditorViewportRectUVE viewportRect{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        const std::optional<Math::RayUVE> centerRay =
            editor.MakeViewportRayUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F});
        ASSERT_TRUE(centerRay.has_value());
        EXPECT_NEAR(centerRay->origin.x, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->origin.y, 1.5F, 0.0001F);
        EXPECT_NEAR(centerRay->origin.z, 6.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.x, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.y, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.z, -1.0F, 0.0001F);
        EXPECT_FALSE(editor.MakeViewportRayUVE(viewportRect, Math::Vector2UVE{-1.0F, 300.0F}).has_value());

        EXPECT_TRUE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), nearEntity);
        EXPECT_FALSE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{0.0F, 0.0F}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);

        editor.SelectEntityUVE(farEntity);
        EXPECT_TRUE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F}, true));
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), (std::vector<Scene::EntityUVE>{farEntity, nearEntity}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), nearEntity);
        EXPECT_FALSE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{0.0F, 0.0F}, true));
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), (std::vector<Scene::EntityUVE>{farEntity, nearEntity}));
        EXPECT_TRUE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F}, true));
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), std::vector<Scene::EntityUVE>{farEntity});
        EXPECT_EQ(editor.GetSelectedEntityUVE(), farEntity);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TranslateSelectedAlongAxis_UpdatesLocalTransformAndConvertsParentScale) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_gizmo.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, parent, parentTransform);

        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        EXPECT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 2.0F));
        const Scene::TransformComponentUVE& translated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_NEAR(translated.localPosition.x, 2.0F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.y, 2.0F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.z, 3.0F, 0.0001F);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localPosition.x,
                    1.0F, 0.0001F);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localPosition.x,
                    2.0F, 0.0001F);
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(
            EditorTransformAxisUVE::Y, std::numeric_limits<float>::infinity()));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_RotatesRootPreservesOtherLocalFieldsAndReplaysHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        initial.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, entity, initial);

        editor.SelectEntityUVE(entity);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Translate);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Rotate);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Rotate);
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Z,
                                                            std::numbers::pi_v<float> * 0.5F));
        const Scene::TransformComponentUVE& rotated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        const Math::Vector3UVE localXAxis =
            Math::RotateVectorUVE(rotated.localRotation, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(localXAxis.x, 0.0F, 0.0001F);
        EXPECT_NEAR(localXAxis.y, 1.0F, 0.0001F);
        EXPECT_NEAR(localXAxis.z, 0.0F, 0.0001F);
        EXPECT_EQ(rotated.localPosition, initial.localPosition);
        EXPECT_EQ(rotated.localScale, initial.localScale);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  initial.localRotation);
        ASSERT_TRUE(editor.RedoUVE());
        const Math::Vector3UVE replayedXAxis = Math::RotateVectorUVE(
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
            Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(replayedXAxis.x, 0.0F, 0.0001F);
        EXPECT_NEAR(replayedXAxis.y, 1.0F, 0.0001F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_ConvertsParentWorldRotationToLocalRotation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_parented.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        Math::QuaternionUVE parentRotation{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 0.0F, 1.0F},
                                              std::numbers::pi_v<float> * 0.5F, parentRotation));
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localRotation = parentRotation;
        AttachRootUVE(engine, parent, parentTransform);

        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        childTransform.localScale = Math::Vector3UVE{2.0F, 2.0F, 2.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::X,
                                                            std::numbers::pi_v<float> * 0.5F));
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        Math::QuaternionUVE worldDelta{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                              std::numbers::pi_v<float> * 0.5F, worldDelta));
        const Math::QuaternionUVE expectedWorld = Math::MultiplyUVE(worldDelta, parentRotation);
        const Scene::WorldTransformComponentUVE& childWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(child);
        const Math::Vector3UVE expectedProbe =
            Math::RotateVectorUVE(expectedWorld, Math::Vector3UVE{0.0F, 1.0F, 0.0F});
        const Math::Vector3UVE actualProbe =
            Math::RotateVectorUVE(childWorld.worldRotation, Math::Vector3UVE{0.0F, 1.0F, 0.0F});
        EXPECT_NEAR(actualProbe.x, expectedProbe.x, 0.0001F);
        EXPECT_NEAR(actualProbe.y, expectedProbe.y, 0.0001F);
        EXPECT_NEAR(actualProbe.z, expectedProbe.z, 0.0001F);
        const Scene::TransformComponentUVE& rotated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_EQ(rotated.localPosition, childTransform.localPosition);
        EXPECT_EQ(rotated.localScale, childTransform.localScale);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_RejectsInvalidOrUnsafeStateWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_safety.uvescene");
        editor.InitUVE();
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Z, 1.0F));

        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(entity);
        const Scene::TransformComponentUVE before =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Y,
                                                             std::numeric_limits<float>::infinity()));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  before.localRotation);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TransformGizmoSixDof_AppliesAllTranslationAndRotationAxesAsAtomicHistoryEntries) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_gizmo_six_dof.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        initial.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, entity, initial);
        editor.SelectEntityUVE(entity);

        ASSERT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 0.5F));
        ASSERT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::Y, -0.25F));
        ASSERT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::Z, 0.75F));
        const Scene::TransformComponentUVE translated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_NEAR(translated.localPosition.x, 1.5F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.y, 1.75F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.z, 3.75F, 0.0001F);
        EXPECT_EQ(translated.localScale, initial.localScale);
        EXPECT_EQ(translated.localRotation, initial.localRotation);

        const auto quaternionChanged = [](const Math::QuaternionUVE& before,
                                           const Math::QuaternionUVE& after) {
            return std::abs(before.x - after.x) > 0.0001F || std::abs(before.y - after.y) > 0.0001F ||
                   std::abs(before.z - after.z) > 0.0001F || std::abs(before.w - after.w) > 0.0001F;
        };
        const Math::QuaternionUVE rotationBefore = translated.localRotation;
        const float angle = std::numbers::pi_v<float> * 0.25F;
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::X, angle));
        const Math::QuaternionUVE rotationAfterX =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation;
        EXPECT_TRUE(quaternionChanged(rotationBefore, rotationAfterX));
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Y, angle));
        const Math::QuaternionUVE rotationAfterY =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation;
        EXPECT_TRUE(quaternionChanged(rotationAfterX, rotationAfterY));
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Z, angle));
        const Math::QuaternionUVE rotationAfterZ =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation;
        EXPECT_TRUE(quaternionChanged(rotationAfterY, rotationAfterZ));

        const Scene::TransformComponentUVE afterAllSix =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_EQ(afterAllSix.localPosition, translated.localPosition);
        EXPECT_EQ(afterAllSix.localScale, initial.localScale);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE().size(), 1U);

        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  rotationBefore);
        ASSERT_TRUE(editor.RedoUVE());
        ASSERT_TRUE(editor.RedoUVE());
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  rotationAfterZ);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localPosition,
                  translated.localPosition);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, GizmoDragSixDof_HitTestsAndCommitsEveryTranslationAndRotationAxis) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_gizmo_drag_six_dof.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(entity);
        ASSERT_TRUE(editor.OrbitViewportUVE(0.37F, 0.18F));
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        const EditorViewportRectUVE viewportRect{{0.0F, 0.0F}, {1024.0F, 768.0F}};
        const std::array<EditorTransformAxisUVE, 3> axes{
            EditorTransformAxisUVE::X, EditorTransformAxisUVE::Y, EditorTransformAxisUVE::Z};

        for (const EditorTransformAxisUVE axis : axes) {
            editor.SetGizmoModeUVE(EditorGizmoModeUVE::Translate);
            const Scene::WorldTransformComponentUVE& world =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
            Math::Vector2UVE center{};
            Math::Vector3UVE worldAxis{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(editor, viewportRect, world.worldPosition, center));
            ASSERT_TRUE(EditorUVEAccessUVE::GetGizmoAxisWorldVectorUVE(editor, entity, axis, worldAxis));
            Math::Vector2UVE endpoint{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(
                editor, viewportRect, world.worldPosition + worldAxis * 1.25F, endpoint));
            ASSERT_TRUE(EditorUVEAccessUVE::BeginGizmoDragUVE(editor, viewportRect, endpoint))
                << "axis=" << static_cast<int>(axis) << " center=(" << center.x << "," << center.y
                << ") endpoint=(" << endpoint.x << "," << endpoint.y << ")";
            EXPECT_EQ(EditorUVEAccessUVE::GetGizmoDragAxisUVE(editor), axis);
            const Math::Vector2UVE screenDirection{endpoint.x - center.x, endpoint.y - center.y};
            const float screenLength = std::sqrt((screenDirection.x * screenDirection.x) +
                                                 (screenDirection.y * screenDirection.y));
            ASSERT_GT(screenLength, 1.0F);
            const Scene::TransformComponentUVE before =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
            const Math::Vector2UVE movedPointer{
                endpoint.x + (screenDirection.x * 10.0F / screenLength),
                endpoint.y + (screenDirection.y * 10.0F / screenLength)};
            EditorUVEAccessUVE::UpdateGizmoDragUVE(editor, movedPointer);
            EditorUVEAccessUVE::CommitGizmoDragUVE(editor);
            const Scene::TransformComponentUVE after =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
            EXPECT_NE(after.localPosition, before.localPosition);
            EXPECT_EQ(after.localRotation, before.localRotation);
            EXPECT_EQ(after.localScale, before.localScale);
            ASSERT_TRUE(editor.UndoUVE());
            const Scene::TransformComponentUVE restored =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
            EXPECT_EQ(restored.localPosition, before.localPosition);
            EXPECT_EQ(restored.localRotation, before.localRotation);
            EXPECT_EQ(restored.localScale, before.localScale);
            services.GetSceneGraphUVE().UpdateUVE(entityManager);
        }

        const auto ringBasis = [](const EditorTransformAxisUVE axis) {
            switch (axis) {
                case EditorTransformAxisUVE::X:
                    return std::array<Math::Vector3UVE, 2>{Math::Vector3UVE{0.0F, 1.0F, 0.0F},
                                                           Math::Vector3UVE{0.0F, 0.0F, 1.0F}};
                case EditorTransformAxisUVE::Y:
                    return std::array<Math::Vector3UVE, 2>{Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                                           Math::Vector3UVE{0.0F, 0.0F, 1.0F}};
                case EditorTransformAxisUVE::Z:
                    return std::array<Math::Vector3UVE, 2>{Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                                           Math::Vector3UVE{0.0F, 1.0F, 0.0F}};
                case EditorTransformAxisUVE::None:
                    return std::array<Math::Vector3UVE, 2>{Math::Vector3UVE{}, Math::Vector3UVE{}};
            }
            return std::array<Math::Vector3UVE, 2>{Math::Vector3UVE{}, Math::Vector3UVE{}};
        };
        const auto quaternionChanged = [](const Math::QuaternionUVE& before,
                                           const Math::QuaternionUVE& after) {
            return std::abs(before.x - after.x) > 0.0001F || std::abs(before.y - after.y) > 0.0001F ||
                   std::abs(before.z - after.z) > 0.0001F || std::abs(before.w - after.w) > 0.0001F;
        };
        for (const EditorTransformAxisUVE axis : axes) {
            editor.SetGizmoModeUVE(EditorGizmoModeUVE::Rotate);
            const Scene::WorldTransformComponentUVE& world =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
            Math::Vector2UVE center{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(editor, viewportRect, world.worldPosition, center));
            const auto basis = ringBasis(axis);
            const float startTurn = 0.37F;
            Math::Vector2UVE ringStart{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(
                editor, viewportRect,
                world.worldPosition + (basis[0] * (std::cos(startTurn) * 1.25F)) +
                    (basis[1] * (std::sin(startTurn) * 1.25F)),
                ringStart));
            ASSERT_TRUE(EditorUVEAccessUVE::BeginGizmoDragUVE(editor, viewportRect, ringStart));
            EXPECT_EQ(EditorUVEAccessUVE::GetGizmoDragAxisUVE(editor), axis);
            const float turn = startTurn + 0.5F;
            const Math::Vector3UVE ringTarget =
                world.worldPosition + (basis[0] * (std::cos(turn) * 1.25F)) +
                (basis[1] * (std::sin(turn) * 1.25F));
            Math::Vector2UVE targetPointer{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(editor, viewportRect, ringTarget, targetPointer));
            const Math::QuaternionUVE before =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation;
            EditorUVEAccessUVE::UpdateGizmoDragUVE(editor, targetPointer);
            EditorUVEAccessUVE::CommitGizmoDragUVE(editor);
            const Math::QuaternionUVE after =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation;
            EXPECT_TRUE(quaternionChanged(before, after));
            ASSERT_TRUE(editor.UndoUVE());
            EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation, before);
            services.GetSceneGraphUVE().UpdateUVE(entityManager);
        }

        for (const EditorTransformAxisUVE axis : axes) {
            editor.SetGizmoModeUVE(EditorGizmoModeUVE::Scale);
            const Scene::WorldTransformComponentUVE& world =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
            Math::Vector2UVE center{};
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(editor, viewportRect, world.worldPosition, center));
            Math::Vector2UVE endpoint{};
            Math::Vector3UVE worldAxis{};
            ASSERT_TRUE(EditorUVEAccessUVE::GetGizmoAxisWorldVectorUVE(editor, entity, axis, worldAxis));
            ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(
                editor, viewportRect, world.worldPosition + worldAxis * 1.25F, endpoint));
            ASSERT_TRUE(EditorUVEAccessUVE::BeginGizmoDragUVE(editor, viewportRect, endpoint));
            EXPECT_EQ(EditorUVEAccessUVE::GetGizmoDragAxisUVE(editor), axis);
            const Math::Vector2UVE screenDirection{endpoint.x - center.x, endpoint.y - center.y};
            const float screenLength = std::sqrt((screenDirection.x * screenDirection.x) +
                                                 (screenDirection.y * screenDirection.y));
            ASSERT_GT(screenLength, 1.0F);
            const Scene::TransformComponentUVE before =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
            const Math::Vector2UVE movedPointer{
                endpoint.x + (screenDirection.x * 10.0F / screenLength),
                endpoint.y + (screenDirection.y * 10.0F / screenLength)};
            EditorUVEAccessUVE::UpdateGizmoDragUVE(editor, movedPointer);
            EditorUVEAccessUVE::CommitGizmoDragUVE(editor);
            const Scene::TransformComponentUVE after =
                entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
            const float beforeScale = axis == EditorTransformAxisUVE::X
                                           ? before.localScale.x
                                           : (axis == EditorTransformAxisUVE::Y ? before.localScale.y : before.localScale.z);
            const float afterScale = axis == EditorTransformAxisUVE::X
                                          ? after.localScale.x
                                          : (axis == EditorTransformAxisUVE::Y ? after.localScale.y : after.localScale.z);
            EXPECT_NE(afterScale, beforeScale);
            EXPECT_EQ(after.localPosition, before.localPosition);
            EXPECT_EQ(after.localRotation, before.localRotation);
            ASSERT_TRUE(editor.UndoUVE());
            EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale,
                      before.localScale);
            services.GetSceneGraphUVE().UpdateUVE(entityManager);
        }

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ViewportNavigationGizmo_XPositiveStartsCameraPresetOrbit) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_view_nav_gizmo.uvescene");
        editor.InitUVE();
        const EditorViewportRectUVE viewportRect{{0.0F, 0.0F}, {1024.0F, 768.0F}};
        const Math::Vector2UVE widgetCenter{962.0F, 104.0F};
        ASSERT_TRUE(EditorUVEAccessUVE::ClickViewportNavigationGizmoUVE(
            editor, viewportRect, Math::Vector2UVE{widgetCenter.x + 34.0F, widgetCenter.y}));
        editor.TickUVE();
        EXPECT_GT(EditorUVEAccessUVE::GetViewportYawUVE(editor), 0.0F);
        for (int frame = 0; frame < 80; ++frame) {
            editor.TickUVE();
        }
        EXPECT_NEAR(EditorUVEAccessUVE::GetViewportYawUVE(editor),
                    std::numbers::pi_v<float> * 0.5F, 0.02F);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, UniversalGizmo_PrioritizesScaleEndpointAndUsesScreenPlanePivotMove) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_universal_gizmo.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(entity);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Universal);
        const EditorViewportRectUVE viewportRect{{0.0F, 0.0F}, {1024.0F, 768.0F}};

        const Scene::WorldTransformComponentUVE& world =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
        Math::Vector2UVE center{};
        ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(editor, viewportRect, world.worldPosition, center));
        ASSERT_TRUE(EditorUVEAccessUVE::BeginGizmoDragUVE(editor, viewportRect, center));
        EXPECT_TRUE(EditorUVEAccessUVE::IsGizmoDragHandleKindUVE(editor, 3));
        const Scene::TransformComponentUVE beforeMove =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EditorUVEAccessUVE::UpdateGizmoDragUVE(editor, Math::Vector2UVE{center.x + 18.0F, center.y - 10.0F});
        EditorUVEAccessUVE::CommitGizmoDragUVE(editor);
        const Scene::TransformComponentUVE afterMove =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_NE(afterMove.localPosition, beforeMove.localPosition);
        EXPECT_EQ(afterMove.localRotation, beforeMove.localRotation);
        EXPECT_EQ(afterMove.localScale, beforeMove.localScale);
        ASSERT_TRUE(editor.UndoUVE());
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        const Scene::WorldTransformComponentUVE& resetWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
        Math::Vector3UVE xAxis{};
        ASSERT_TRUE(EditorUVEAccessUVE::GetGizmoAxisWorldVectorUVE(editor, entity, EditorTransformAxisUVE::X, xAxis));
        Math::Vector2UVE endpoint{};
        ASSERT_TRUE(EditorUVEAccessUVE::ProjectWorldPointUVE(
            editor, viewportRect, resetWorld.worldPosition + xAxis * 1.25F, endpoint));
        ASSERT_TRUE(EditorUVEAccessUVE::BeginGizmoDragUVE(editor, viewportRect, endpoint));
        EXPECT_EQ(EditorUVEAccessUVE::GetGizmoDragAxisUVE(editor), EditorTransformAxisUVE::X);
        const Scene::TransformComponentUVE beforeScale =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        const Math::Vector2UVE screenDirection{endpoint.x - center.x, endpoint.y - center.y};
        const float screenLength = std::sqrt((screenDirection.x * screenDirection.x) +
                                             (screenDirection.y * screenDirection.y));
        ASSERT_GT(screenLength, 1.0F);
        EditorUVEAccessUVE::UpdateGizmoDragUVE(
            editor, Math::Vector2UVE{endpoint.x + screenDirection.x * 10.0F / screenLength,
                                    endpoint.y + screenDirection.y * 10.0F / screenLength});
        EditorUVEAccessUVE::CommitGizmoDragUVE(editor);
        const Scene::TransformComponentUVE afterScale =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_NE(afterScale.localScale.x, beforeScale.localScale.x);
        EXPECT_EQ(afterScale.localPosition, beforeScale.localPosition);
        EXPECT_EQ(afterScale.localRotation, beforeScale.localRotation);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedAlongAxis_UpdatesOnlyPositiveLocalScaleAndReplaysHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_scale.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        initial.localScale = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, child, initial);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Translate);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Scale);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Scale);
        ASSERT_TRUE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::Y, 1.5F));
        const Scene::TransformComponentUVE& scaled =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_EQ(scaled.localPosition, initial.localPosition);
        EXPECT_EQ(scaled.localRotation, initial.localRotation);
        EXPECT_NEAR(scaled.localScale.x, 1.0F, 0.0001F);
        EXPECT_NEAR(scaled.localScale.y, 3.5F, 0.0001F);
        EXPECT_NEAR(scaled.localScale.z, 3.0F, 0.0001F);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localScale,
                  initial.localScale);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localScale.y,
                    3.5F, 0.0001F);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedAlongAxis_RejectsUnsafeInputWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_scale_safety.uvescene");
        editor.InitUVE();
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 1.0F));
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(entity);
        const Scene::TransformComponentUVE before = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::X, -1.0F));
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::Z,
                                                       std::numeric_limits<float>::infinity()));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale,
                  before.localScale);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedUniformlyUVE_AppliesAdditiveOffsetAndSnapping) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_uniform_scale_offset.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localScale = Math::Vector3UVE{2.0F, 1.0F, 1.0F};
        AttachRootUVE(engine, entity, initial);
        editor.SelectEntityUVE(entity);
        ASSERT_TRUE(editor.ScaleSelectedUniformlyUVE(1.0F));
        const Scene::TransformComponentUVE& additive =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_NEAR(additive.localScale.x, 3.0F, 0.0001F);
        EXPECT_NEAR(additive.localScale.y, 2.0F, 0.0001F);
        EXPECT_NEAR(additive.localScale.z, 2.0F, 0.0001F);
        EditorTransformSnappingSettingsUVE settings{};
        settings.enabled = true;
        settings.scaleStep = 0.25F;
        ASSERT_TRUE(editor.SetTransformSnappingSettingsUVE(settings));
        ASSERT_TRUE(editor.ScaleSelectedUniformlyUVE(0.37F));
        const Scene::TransformComponentUVE& snapped =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_NEAR(snapped.localScale.x, 3.25F, 0.0001F);
        EXPECT_NEAR(snapped.localScale.y, 2.25F, 0.0001F);
        EXPECT_NEAR(snapped.localScale.z, 2.25F, 0.0001F);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedUniformlyUVE_RejectsAsymmetricFloorWithoutPartialMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_uniform_scale_floor.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localScale = Math::Vector3UVE{0.01F, 5.0F, 5.0F};
        AttachRootUVE(engine, entity, initial);
        editor.SelectEntityUVE(entity);
        EXPECT_FALSE(editor.ScaleSelectedUniformlyUVE(-0.02F));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale,
                  initial.localScale);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, SelectedBoundsQuery_BuildsIdentityWorldBoxWithoutMutatingEditorState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_selection_bounds_identity.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform{};
        transform.localPosition = Math::Vector3UVE{4.0F, -5.0F, 6.0F};
        AttachRootUVE(engine, entity, transform);
        editor.SelectEntityUVE(entity);
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        Scene::ColliderComponentUVE collider{};
        collider.halfExtents = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, collider);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(entity);

        const std::optional<EditorSelectionBoundsUVE> bounds = editor.TryGetSelectedBoundsUVE();
        ASSERT_TRUE(bounds.has_value());
        EXPECT_EQ(bounds->worldCenter, transform.localPosition);
        EXPECT_EQ(bounds->worldCorners[0], (Math::Vector3UVE{3.0F, -7.0F, 3.0F}));
        EXPECT_EQ(bounds->worldCorners[6], (Math::Vector3UVE{5.0F, -3.0F, 9.0F}));
        const Scene::TransformComponentUVE& afterQuery =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_EQ(afterQuery.localPosition, transform.localPosition);
        EXPECT_EQ(afterQuery.localRotation, transform.localRotation);
        EXPECT_EQ(afterQuery.localScale, transform.localScale);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), entity);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, SelectedBoundsQuery_UsesDerivedParentTransformAndRejectsUnsafeState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_selection_bounds_parented.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        Math::QuaternionUVE parentRotation{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 0.0F, 1.0F},
                                              std::numbers::pi_v<float> * 0.5F, parentRotation));
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localPosition = Math::Vector3UVE{10.0F, 20.0F, 30.0F};
        parentTransform.localRotation = parentRotation;
        parentTransform.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, parent, parentTransform);

        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        AttachRootUVE(engine, child, childTransform);
        Scene::ColliderComponentUVE collider{};
        collider.halfExtents = Math::Vector3UVE{0.5F, 1.0F, 1.5F};
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(child, collider);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(child);

        const std::optional<EditorSelectionBoundsUVE> bounds = editor.TryGetSelectedBoundsUVE();
        ASSERT_TRUE(bounds.has_value());
        EXPECT_NEAR(bounds->worldCenter.x, 10.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCenter.y, 22.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCenter.z, 30.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[0].x, 13.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[0].y, 21.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[0].z, 24.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[6].x, 7.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[6].y, 23.0F, 0.0001F);
        EXPECT_NEAR(bounds->worldCorners[6].z, 36.0F, 0.0001F);

        entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(child).halfExtents.x = 0.0F;
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(child).halfExtents = collider.halfExtents;
        Scene::WorldTransformComponentUVE& worldTransform =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(child);
        worldTransform.dirty = true;
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        worldTransform.dirty = false;
        const Math::Vector3UVE savedScale = worldTransform.worldScale;
        worldTransform.worldScale.x = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        worldTransform.worldScale = savedScale;
        const Math::QuaternionUVE savedRotation = worldTransform.worldRotation;
        worldTransform.worldRotation = Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 0.0F};
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
        worldTransform.worldRotation = savedRotation;
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());

        editor.SelectEntityUVE(editor.GetViewportCameraUVE());
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());

        editor.ShutdownUVE();
        EXPECT_FALSE(editor.TryGetSelectedBoundsUVE().has_value());
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TransformSnappingSettings_ExposeSafeDefaultsAndRejectInvalidValues) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_snapping_settings.uvescene");
        editor.InitUVE();

        const EditorTransformSnappingSettingsUVE defaults = editor.GetTransformSnappingSettingsUVE();
        EXPECT_FALSE(defaults.enabled);
        EXPECT_FLOAT_EQ(defaults.translateStep, 1.0F);
        EXPECT_FLOAT_EQ(defaults.rotateStepDegrees, 15.0F);
        EXPECT_FLOAT_EQ(defaults.scaleStep, 0.1F);

        EditorTransformSnappingSettingsUVE configured{true, 0.5F, 45.0F, 0.25F};
        ASSERT_TRUE(editor.SetTransformSnappingSettingsUVE(configured));
        EXPECT_EQ(editor.GetTransformSnappingSettingsUVE().enabled, configured.enabled);
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().translateStep, configured.translateStep);
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().rotateStepDegrees, configured.rotateStepDegrees);
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().scaleStep, configured.scaleStep);

        EditorTransformSnappingSettingsUVE invalid = configured;
        invalid.translateStep = 0.0F;
        EXPECT_FALSE(editor.SetTransformSnappingSettingsUVE(invalid));
        invalid = configured;
        invalid.rotateStepDegrees = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(editor.SetTransformSnappingSettingsUVE(invalid));
        invalid = configured;
        invalid.scaleStep = std::numeric_limits<float>::quiet_NaN();
        EXPECT_FALSE(editor.SetTransformSnappingSettingsUVE(invalid));
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().translateStep, configured.translateStep);
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().rotateStepDegrees, configured.rotateStepDegrees);
        EXPECT_FLOAT_EQ(editor.GetTransformSnappingSettingsUVE().scaleStep, configured.scaleStep);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TransformSnapping_QuantizesCommandsWithoutHistoryDriftAndReplaysRotation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_snapping_commands.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, entity, initial);
        editor.SelectEntityUVE(entity);

        ASSERT_TRUE(editor.SetTransformSnappingSettingsUVE(
            EditorTransformSnappingSettingsUVE{true, 0.5F, 15.0F, 0.25F}));
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 0.1F));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());

        ASSERT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 0.74F));
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localPosition.x,
                    1.5F, 0.0001F);
        ASSERT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE::X, -0.74F));
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localPosition.x,
                    initial.localPosition.x, 0.0001F);

        ASSERT_TRUE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::X, 0.37F));
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale.x,
                    1.25F, 0.0001F);
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE::X, -1.24F));
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale.x,
                    1.25F, 0.0001F);

        const float twentyDegreesRadians = (20.0F * std::numbers::pi_v<float>) / 180.0F;
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE::Z, twentyDegreesRadians));
        const Math::Vector3UVE rotatedXAxis = Math::RotateVectorUVE(
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
            Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(rotatedXAxis.x, std::cos(std::numbers::pi_v<float> / 12.0F), 0.0001F);
        EXPECT_NEAR(rotatedXAxis.y, std::sin(std::numbers::pi_v<float> / 12.0F), 0.0001F);
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  initial.localRotation);
        ASSERT_TRUE(editor.RedoUVE());
        const Math::Vector3UVE replayedXAxis = Math::RotateVectorUVE(
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
            Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(replayedXAxis.x, rotatedXAxis.x, 0.0001F);
        EXPECT_NEAR(replayedXAxis.y, rotatedXAxis.y, 0.0001F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, LoadMissingScene_FailsWithoutDestroyingCurrentDocument) {
    const std::filesystem::path missingScenePath = "uve_editor_tests_missing.uvescene";
    std::filesystem::remove(missingScenePath);

    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), missingScenePath);
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        EXPECT_FALSE(editor.LoadSceneUVE());
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 1U);
        EXPECT_EQ(roots.front(), root);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, PlayModeSandbox_RestoresSnapshotRejectsAuthoringAndPreservesSelectionIntent) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_play_restore.uvescene", 100U, &engine);
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE authored{};
        authored.localPosition = Math::Vector3UVE{2.0F, 3.0F, -4.0F};
        AttachRootUVE(engine, root, authored);
        editor.SelectEntityUVE(root);

        ASSERT_TRUE(editor.EnterPlayModeUVE());
        EXPECT_EQ(editor.GetPlayModeStateUVE(), EditorPlayModeStateUVE::Playing);
        EXPECT_TRUE(engine.IsTransientSimulationSessionActiveUVE());
        EXPECT_FALSE(editor.SetSelectedLocalTransformUVE(Scene::TransformComponentUVE{}));
        EXPECT_EQ(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.UndoUVE());

        ASSERT_TRUE(editor.PausePlayModeUVE());
        EXPECT_EQ(engine.GetSimulationExecutionModeUVE(), Core::SimulationExecutionModeUVE::Paused);
        ASSERT_TRUE(editor.StepPlayModeUVE());
        EXPECT_FALSE(editor.StepPlayModeUVE());
        engine.TickFrameUVE();
        ASSERT_TRUE(editor.ResumePlayModeUVE());
        EXPECT_EQ(engine.GetSimulationExecutionModeUVE(), Core::SimulationExecutionModeUVE::Running);

        ASSERT_TRUE(editor.StopPlayModeUVE());
        EXPECT_EQ(editor.GetPlayModeStateUVE(), EditorPlayModeStateUVE::Edit);
        EXPECT_FALSE(engine.IsTransientSimulationSessionActiveUVE());
        const std::vector<Scene::EntityUVE> restoredRoots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(restoredRoots.size(), 1U);
        EXPECT_NE(restoredRoots.front(), root);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), restoredRoots.front());
        const Scene::TransformComponentUVE& restored =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(restoredRoots.front());
        EXPECT_EQ(restored.localPosition, authored.localPosition);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, PlayModeSandbox_RestoresOrderedMultiSelectionAndActiveEntity) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_play_multi_selection.uvescene", 100U, &engine);
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE first = entityManager.CreateEntityUVE();
        const Scene::EntityUVE second = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, first, Scene::TransformComponentUVE{});
        AttachRootUVE(engine, second, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(first);
        editor.ToggleEntitySelectionUVE(second);

        ASSERT_TRUE(editor.EnterPlayModeUVE());
        ASSERT_TRUE(editor.StopPlayModeUVE());

        const std::vector<Scene::EntityUVE> restoredRoots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(restoredRoots.size(), 2U);
        EXPECT_NE(restoredRoots[0], first);
        EXPECT_NE(restoredRoots[1], second);
        EXPECT_EQ(editor.GetSelectedEntitiesUVE(), restoredRoots);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), restoredRoots.back());
        EXPECT_FALSE(editor.HasSingleDocumentSelectionUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, PlayModeSandbox_HandlesEmptyDocumentAndMissingControlSafely) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE withoutControl(engine.GetServicesUVE(), "uve_editor_tests_play_no_control.uvescene");
        withoutControl.InitUVE();
        EXPECT_FALSE(withoutControl.EnterPlayModeUVE());
        withoutControl.ShutdownUVE();

        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_play_empty.uvescene", 100U, &engine);
        editor.InitUVE();
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        ASSERT_TRUE(editor.EnterPlayModeUVE());
        ASSERT_TRUE(editor.StopPlayModeUVE());
        EXPECT_EQ(editor.GetPlayModeStateUVE(), EditorPlayModeStateUVE::Edit);
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, GizmoCoordinateSpaceUVE_DefaultsToWorldAndRejectsSandboxChanges) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_coordinate_space.uvescene", 100U, &engine);
        editor.InitUVE();
        EXPECT_EQ(editor.GetGizmoCoordinateSpaceUVE(), EditorGizmoCoordinateSpaceUVE::World);
        EXPECT_TRUE(editor.SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE::Local));
        EXPECT_EQ(editor.GetGizmoCoordinateSpaceUVE(), EditorGizmoCoordinateSpaceUVE::Local);
        EXPECT_TRUE(editor.SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE::World));

        ASSERT_TRUE(editor.EnterPlayModeUVE());
        EXPECT_FALSE(editor.SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE::Local));
        ASSERT_TRUE(editor.PausePlayModeUVE());
        EXPECT_FALSE(editor.SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE::Local));
        ASSERT_TRUE(editor.StopPlayModeUVE());
        EXPECT_TRUE(editor.SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE::Local));
        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, VisualScriptSearchInsertionPreservesPositionAndCompilerUsesNativeGraph) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_scripting_workspace.uvescene");
        editor.InitUVE();
        Scripting::ScriptGraphCanvasUVE& canvas = editor.GetVisualScriptCanvasUVE();
        const Scripting::ScriptGraphCanvasSnapshotUVE before = canvas.GetSnapshotUVE();
        ASSERT_TRUE(std::any_of(
            before.paletteDescriptors.cbegin(), before.paletteDescriptors.cend(),
            [](const Scripting::ScriptGraphCanvasPaletteEntryUVE& entry) { return entry.typeId == "engine.log"; }));

        const Scripting::ScriptGraphCanvasPointUVE insertionPosition{-37.5F, 82.25F};
        const auto addResult = canvas.AddNodeTypeUVE("engine.log", insertionPosition, before.revision);
        ASSERT_TRUE(addResult.IsAppliedUVE());
        const Scripting::ScriptGraphCanvasSnapshotUVE after = canvas.GetSnapshotUVE();
        ASSERT_EQ(after.nodes.size(), 1U);
        EXPECT_EQ(after.nodes.front().typeId, "engine.log");
        EXPECT_EQ(after.nodes.front().position, insertionPosition);
        EXPECT_EQ(after.revision, addResult.revision);

        EditorUVEAccessUVE::CompileVisualScriptUVE(editor);
        EXPECT_TRUE(EditorUVEAccessUVE::IsVisualScriptCompileSuccessfulUVE(editor));
        EXPECT_EQ(EditorUVEAccessUVE::GetLastCompiledVisualScriptGraphRevisionUVE(editor), after.graphRevision);
        EXPECT_GT(EditorUVEAccessUVE::GetVisualScriptCompileInstructionCountUVE(editor), 0U);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests


TEST(GizmoSystemUVETest, PackageLayerCompositionAndNativeAxisSpace) {
    const UVE::Editor::Gizmo::GizmoLayerVisibilityUVE move =
        UVE::Editor::Gizmo::GizmoSystemUVE::LayersForUVE(UVE::Editor::Gizmo::GizmoModeUVE::Move);
    EXPECT_TRUE(move.move);
    EXPECT_FALSE(move.rotate);
    EXPECT_FALSE(move.scale);

    const UVE::Editor::Gizmo::GizmoLayerVisibilityUVE universal =
        UVE::Editor::Gizmo::GizmoSystemUVE::LayersForUVE(UVE::Editor::Gizmo::GizmoModeUVE::Universal);
    EXPECT_TRUE(universal.move);
    EXPECT_TRUE(universal.rotate);
    EXPECT_TRUE(universal.scale);

    const UVE::Math::QuaternionUVE identity{1.0F, 0.0F, 0.0F, 0.0F};
    const UVE::Math::Vector3UVE worldX = UVE::Editor::Gizmo::GizmoSystemUVE::AxisDirectionUVE(
        UVE::Editor::Gizmo::GizmoAxisUVE::X, identity, UVE::Editor::Gizmo::GizmoSpaceUVE::World);
    EXPECT_NEAR(worldX.x, 1.0F, 0.0001F);
    EXPECT_NEAR(worldX.y, 0.0F, 0.0001F);
    EXPECT_NEAR(worldX.z, 0.0F, 0.0001F);
}

TEST(GizmoSystemUVETest, HtmlReferencePalette) {
    using UVE::Editor::Gizmo::GizmoAxisUVE;
    using UVE::Editor::Gizmo::GizmoSystemUVE;
    const auto x = GizmoSystemUVE::AxisColorUVE(GizmoAxisUVE::X, false);
    const auto y = GizmoSystemUVE::AxisColorUVE(GizmoAxisUVE::Y, false);
    const auto z = GizmoSystemUVE::AxisColorUVE(GizmoAxisUVE::Z, false);
    const auto hover = GizmoSystemUVE::AxisColorUVE(GizmoAxisUVE::X, true);
    EXPECT_NEAR(x.x, 1.0F, 0.001F);
    EXPECT_NEAR(x.y, 93.0F / 255.0F, 0.001F);
    EXPECT_NEAR(x.z, 93.0F / 255.0F, 0.001F);
    EXPECT_NEAR(y.x, 74.0F / 255.0F, 0.001F);
    EXPECT_NEAR(y.y, 222.0F / 255.0F, 0.001F);
    EXPECT_NEAR(y.z, 128.0F / 255.0F, 0.001F);
    EXPECT_NEAR(z.x, 59.0F / 255.0F, 0.001F);
    EXPECT_NEAR(z.y, 156.0F / 255.0F, 0.001F);
    EXPECT_NEAR(z.z, 1.0F, 0.001F);
    EXPECT_NEAR(hover.x, 1.0F, 0.001F);
    EXPECT_NEAR(hover.y, 217.0F / 255.0F, 0.001F);
    EXPECT_NEAR(hover.z, 51.0F / 255.0F, 0.001F);
}

TEST(ViewportNavGizmoUVETest, HtmlReferencePaletteAndLabels) {
    using UVE::Editor::Gizmo::ViewportNavAxisUVE;
    using UVE::Editor::Gizmo::ViewportNavGizmoUVE;
    EXPECT_EQ(ViewportNavGizmoUVE::AxisColorUVE(ViewportNavAxisUVE::X, true, false), 0xFF5D5DFFU);
    EXPECT_EQ(ViewportNavGizmoUVE::AxisColorUVE(ViewportNavAxisUVE::Y, true, false), 0xFF80DE4AU);
    EXPECT_EQ(ViewportNavGizmoUVE::AxisColorUVE(ViewportNavAxisUVE::Z, true, false), 0xFFFF9C3BU);
    EXPECT_EQ(ViewportNavGizmoUVE::AxisColorUVE(ViewportNavAxisUVE::X, true, true), 0xFF33D9FFU);
    EXPECT_STREQ(ViewportNavGizmoUVE::AxisLabelUVE(ViewportNavAxisUVE::X, true), "X+");
    EXPECT_STREQ(ViewportNavGizmoUVE::AxisLabelUVE(ViewportNavAxisUVE::Z, false), "Z-");
}

TEST(ViewportNavGizmoUVETest, PackageSixButtonLayoutAndPresetMapping) {
    UVE::Editor::Gizmo::ViewportNavGizmoUVE navigation;
    navigation.SetAnchorUVE(UVE::Math::Vector2UVE{100.0F, 100.0F});
    navigation.UpdateLayoutUVE(0.0F, 0.0F);
    ASSERT_EQ(navigation.GetButtonsUVE().size(), 6U);
    EXPECT_NEAR(navigation.GetButtonsUVE()[0].screenPosition.x, 132.0F, 0.001F);
    EXPECT_NEAR(navigation.GetButtonsUVE()[0].screenPosition.y, 100.0F, 0.001F);
    EXPECT_TRUE(navigation.GetButtonsUVE()[4].degenerate);
    EXPECT_TRUE(navigation.GetButtonsUVE()[5].degenerate);
    EXPECT_TRUE(navigation.HitTestPlateUVE(UVE::Math::Vector2UVE{100.0F, 100.0F}));

    UVE::Editor::Gizmo::ViewportNavPresetUVE preset = UVE::Editor::Gizmo::ViewportNavPresetUVE::Front;
    ASSERT_TRUE(navigation.HandleClickUVE(UVE::Math::Vector2UVE{132.0F, 100.0F}, preset));
    EXPECT_EQ(preset, UVE::Editor::Gizmo::ViewportNavPresetUVE::Right);
    float yaw = 0.0F;
    float pitch = 0.0F;
    UVE::Editor::Gizmo::ViewportNavGizmoUVE::PresetAnglesUVE(preset, yaw, pitch);
    EXPECT_NEAR(yaw, std::numbers::pi_v<float> * 0.5F, 0.0001F);
    EXPECT_NEAR(pitch, 0.0F, 0.0001F);
}
