// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeBridgeTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_editor_bridge_tests.log";
    config.settingsFilePath = "uve_editor_bridge_tests_settings.json";
    config.assetDatabaseFilePath = "uve_editor_bridge_tests_assets.json";
    config.saveDirectoryPath = "uve_editor_bridge_tests_saves";
    config.shaderCachePath = "uve_editor_bridge_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

[[nodiscard]] std::vector<Scene::EntityUVE> GetRootsUVE(EditorUVE& editor) {
    return editor.GetDocumentRootsUVE();
}

TEST(EditorBridgeUVETest, EntityRefUVE_ValidatesTheFullGenerationalIdentity) {
    const EditorBridgeEntityRefUVE invalid{};
    EXPECT_FALSE(invalid.IsValidUVE());

    const EditorBridgeEntityRefUVE sameIndexDifferentGeneration{
        Scene::kInvalidEntityUVE.index, Scene::kInvalidEntityUVE.generation + 1U};
    EXPECT_TRUE(sameIndexDifferentGeneration.IsValidUVE());

    const EditorBridgeEntityRefUVE ordinaryEntity{0U, 7U};
    EXPECT_TRUE(ordinaryEntity.IsValidUVE());
}

TEST(EditorBridgeUVETest, SnapshotUVE_ObservesNativeEditorChangesAndIncrementsRevision) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_native_state.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);

        const EditorBridgeSnapshotUVE before = bridge.GetSnapshotUVE();
        EXPECT_EQ(before.revision, 1U);
        EXPECT_TRUE(before.selectedEntities.empty());
        EXPECT_FALSE(before.sceneDirty);

        const Scene::EntityUVE created = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_NE(created, Scene::kInvalidEntityUVE);
        const EditorBridgeSnapshotUVE after = bridge.GetSnapshotUVE();
        EXPECT_GT(after.revision, before.revision);
        EXPECT_TRUE(after.sceneDirty);
        ASSERT_EQ(after.selectedEntities.size(), 1U);
        EXPECT_EQ(after.selectedEntities.front().entity, (EditorBridgeEntityRefUVE{created.index, created.generation}));
        ASSERT_TRUE(after.activeEntity.has_value());
        EXPECT_EQ(*after.activeEntity, (EditorBridgeEntityRefUVE{created.index, created.generation}));

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, DispatchUVE_RejectsStaleMutationAfterNativeEditorChange) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stale_request.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();

        ASSERT_NE(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Plane), Scene::kInvalidEntityUVE);
        const std::vector<Scene::EntityUVE> rootsBefore = GetRootsUVE(editor);
        const EditorBridgeRequestUVE staleRequest{
            kEditorBridgeProtocolVersionUVE, 41U, initial.revision, EditorBridgeRequestKindUVE::CreateDocumentEntity,
            std::nullopt, std::nullopt, EditorEntityKindUVE::Cube};
        const EditorBridgeResponseUVE response = bridge.DispatchUVE(staleRequest);
        EXPECT_FALSE(response.applied);
        EXPECT_EQ(response.code, "bridge.snapshot.stale");
        EXPECT_GT(response.snapshot.revision, initial.revision);
        EXPECT_EQ(GetRootsUVE(editor), rootsBefore);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, DispatchUVE_RoutesCreateNameUndoRedoThroughNativeCommands) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_commands.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();

        const EditorBridgeRequestUVE createRequest{
            kEditorBridgeProtocolVersionUVE, 1U, snapshot.revision, EditorBridgeRequestKindUVE::CreateDocumentEntity,
            std::nullopt, std::nullopt, EditorEntityKindUVE::UVSphere};
        const EditorBridgeResponseUVE created = bridge.DispatchUVE(createRequest);
        ASSERT_TRUE(created.applied);
        ASSERT_TRUE(created.createdEntity.has_value());
        EXPECT_TRUE(created.snapshot.sceneDirty);
        EXPECT_TRUE(created.snapshot.canUndo);
        snapshot = created.snapshot;

        const EditorBridgeRequestUVE nameRequest{
            kEditorBridgeProtocolVersionUVE, 2U, snapshot.revision, EditorBridgeRequestKindUVE::SetSelectedEntityName,
            std::nullopt, std::string{"Bridge Authored Name"}, std::nullopt};
        const EditorBridgeResponseUVE named = bridge.DispatchUVE(nameRequest);
        ASSERT_TRUE(named.applied);
        ASSERT_EQ(named.snapshot.selectedEntities.size(), 1U);
        EXPECT_EQ(named.snapshot.selectedEntities.front().displayLabel, "Bridge Authored Name");

        const EditorBridgeRequestUVE undoRequest{
            kEditorBridgeProtocolVersionUVE, 3U, named.snapshot.revision, EditorBridgeRequestKindUVE::Undo,
            std::nullopt, std::nullopt, std::nullopt};
        const EditorBridgeResponseUVE undone = bridge.DispatchUVE(undoRequest);
        ASSERT_TRUE(undone.applied);
        ASSERT_EQ(undone.snapshot.selectedEntities.size(), 1U);
        EXPECT_NE(undone.snapshot.selectedEntities.front().displayLabel, "Bridge Authored Name");

        const EditorBridgeRequestUVE redoRequest{
            kEditorBridgeProtocolVersionUVE, 4U, undone.snapshot.revision, EditorBridgeRequestKindUVE::Redo,
            std::nullopt, std::nullopt, std::nullopt};
        const EditorBridgeResponseUVE redone = bridge.DispatchUVE(redoRequest);
        ASSERT_TRUE(redone.applied);
        ASSERT_EQ(redone.snapshot.selectedEntities.size(), 1U);
        EXPECT_EQ(redone.snapshot.selectedEntities.front().displayLabel, "Bridge Authored Name");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, DispatchUVE_RejectsUnsupportedProtocolAndInvalidEntityWithoutMutation) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_rejection.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();

        const EditorBridgeRequestUVE invalidVersion{
            99U, 7U, snapshot.revision, EditorBridgeRequestKindUVE::CreateDocumentEntity,
            std::nullopt, std::nullopt, EditorEntityKindUVE::Cube};
        const EditorBridgeResponseUVE versionResponse = bridge.DispatchUVE(invalidVersion);
        EXPECT_FALSE(versionResponse.applied);
        EXPECT_EQ(versionResponse.code, "bridge.protocol.unsupported");
        EXPECT_TRUE(GetRootsUVE(editor).empty());

        const EditorBridgeRequestUVE invalidEntity{
            kEditorBridgeProtocolVersionUVE, 8U, snapshot.revision, EditorBridgeRequestKindUVE::SelectEntity,
            EditorBridgeEntityRefUVE{3U, 9U}, std::nullopt, std::nullopt};
        const EditorBridgeResponseUVE entityResponse = bridge.DispatchUVE(invalidEntity);
        EXPECT_FALSE(entityResponse.applied);
        EXPECT_EQ(entityResponse.code, "bridge.entity.invalid");
        EXPECT_TRUE(entityResponse.snapshot.selectedEntities.empty());
        EXPECT_TRUE(GetRootsUVE(editor).empty());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, SnapshotUVE_CopiesHierarchyInspectorAndNativePanelSessionState) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_panel_snapshot.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);

        const Scene::EntityUVE root = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_NE(root, Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Bridge Root"));
        const Scene::EntityUVE child = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Plane);
        ASSERT_NE(child, Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(root));
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Bridge Child"));

        EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();
        ASSERT_EQ(snapshot.hierarchy.entries.size(), 2U);
        EXPECT_EQ(snapshot.hierarchy.entries[0].entity, (EditorBridgeEntityRefUVE{root.index, root.generation}));
        EXPECT_EQ(snapshot.hierarchy.entries[0].displayLabel, "Bridge Root");
        EXPECT_EQ(snapshot.hierarchy.entries[0].depth, 0U);
        EXPECT_EQ(snapshot.hierarchy.entries[0].childCount, 1U);
        EXPECT_EQ(snapshot.hierarchy.entries[1].entity, (EditorBridgeEntityRefUVE{child.index, child.generation}));
        ASSERT_TRUE(snapshot.hierarchy.entries[1].parent.has_value());
        EXPECT_EQ(*snapshot.hierarchy.entries[1].parent, (EditorBridgeEntityRefUVE{root.index, root.generation}));
        EXPECT_EQ(snapshot.hierarchy.entries[1].depth, 1U);
        ASSERT_EQ(snapshot.inspector.mode, EditorBridgeInspectorModeUVE::SingleSelection);
        ASSERT_TRUE(snapshot.inspector.activeEntity.has_value());
        EXPECT_EQ(snapshot.inspector.activeEntity->displayLabel, "Bridge Child");
        ASSERT_TRUE(snapshot.inspector.parent.has_value());
        EXPECT_TRUE(snapshot.inspector.parent->displayLabel.starts_with("Bridge Root [Cube] (Entity "));
        EXPECT_EQ(snapshot.inspector.eligibleDrawerIds,
                  (std::vector<std::string>{"name", "hierarchy", "transform", "primitive-mesh"}));
        EXPECT_TRUE(snapshot.inspector.canEditSelectedName);
        EXPECT_FALSE(snapshot.contentBrowser.initialized);
        EXPECT_EQ(snapshot.viewportSurface.state, EditorBridgeViewportSurfaceStateUVE::Unavailable);
        EXPECT_EQ(snapshot.viewportSurface.generation, 0U);
        EXPECT_EQ(snapshot.viewportSurface.width, 0U);
        EXPECT_EQ(snapshot.viewportSurface.height, 0U);
        EXPECT_TRUE(snapshot.viewportSurface.nativeRendererOwnsSurface);
        EXPECT_FALSE(snapshot.viewportSurface.managedAttachAllowed);
        EXPECT_TRUE(snapshot.visualScripting.available);
        EXPECT_TRUE(snapshot.visualScripting.canEdit);
        EXPECT_EQ(snapshot.visualScripting.nodeCount, 0U);
        EXPECT_EQ(snapshot.visualScripting.linkCount, 0U);
        EXPECT_EQ(snapshot.visualScripting.canvas.nodes.size(), 0U);
        EXPECT_EQ(snapshot.visualScripting.canvas.links.size(), 0U);
        EXPECT_FALSE(snapshot.visualScripting.canvas.nodesTruncated);
        EXPECT_FALSE(snapshot.visualScripting.canvas.linksTruncated);

        EditorBridgeRequestUVE canvasViewRequest{};
        canvasViewRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        canvasViewRequest.requestId = 8U;
        canvasViewRequest.expectedRevision = snapshot.revision;
        canvasViewRequest.kind = EditorBridgeRequestKindUVE::SetVisualScriptView;
        canvasViewRequest.visualScriptView = Scripting::ScriptGraphCanvasViewUVE{{4.0F, -2.0F}, 1.5F};
        const EditorBridgeResponseUVE canvasViewResponse = bridge.DispatchUVE(canvasViewRequest);
        ASSERT_TRUE(canvasViewResponse.applied);
        EXPECT_EQ(canvasViewResponse.snapshot.visualScripting.canvas.view.pan,
                  (Scripting::ScriptGraphCanvasPointUVE{4.0F, -2.0F}));
        EXPECT_FLOAT_EQ(canvasViewResponse.snapshot.visualScripting.canvas.view.zoom, 1.5F);

        EditorBridgeRequestUVE canvasStaleRequest = canvasViewRequest;
        canvasStaleRequest.requestId = 81U;
        canvasStaleRequest.expectedRevision = snapshot.revision;
        const EditorBridgeResponseUVE canvasStaleResponse = bridge.DispatchUVE(canvasStaleRequest);
        EXPECT_FALSE(canvasStaleResponse.applied);
        EXPECT_EQ(canvasStaleResponse.code, "bridge.snapshot.stale");

        EditorBridgeRequestUVE surfaceRequest{};
        surfaceRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        surfaceRequest.requestId = 9U;
        surfaceRequest.expectedRevision = 0U;
        surfaceRequest.kind = EditorBridgeRequestKindUVE::ReadViewportSurface;
        const EditorBridgeResponseUVE surfaceResponse = bridge.DispatchUVE(surfaceRequest);
        EXPECT_FALSE(surfaceResponse.applied);
        EXPECT_EQ(surfaceResponse.code, "bridge.viewport_surface.unavailable");
        EXPECT_EQ(surfaceResponse.snapshot.viewportSurface, snapshot.viewportSurface);

        EditorBridgeRequestUVE filterRequest{};
        filterRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        filterRequest.requestId = 10U;
        filterRequest.expectedRevision = canvasViewResponse.snapshot.revision;
        filterRequest.kind = EditorBridgeRequestKindUVE::SetHierarchyFilter;
        filterRequest.hierarchyFilter = "child";
        const EditorBridgeResponseUVE filtered = bridge.DispatchUVE(filterRequest);
        ASSERT_TRUE(filtered.applied);
        EXPECT_TRUE(filtered.snapshot.hierarchy.filterActive);
        ASSERT_EQ(filtered.snapshot.hierarchy.entries.size(), 2U);
        EXPECT_GT(filtered.snapshot.revision, snapshot.revision);

        EditorBridgeRequestUVE toggleRequest{};
        toggleRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        toggleRequest.requestId = 11U;
        toggleRequest.expectedRevision = filtered.snapshot.revision;
        toggleRequest.kind = EditorBridgeRequestKindUVE::ToggleEntitySelection;
        toggleRequest.entity = EditorBridgeEntityRefUVE{root.index, root.generation};
        const EditorBridgeResponseUVE toggled = bridge.DispatchUVE(toggleRequest);
        ASSERT_TRUE(toggled.applied);
        EXPECT_EQ(toggled.snapshot.inspector.mode, EditorBridgeInspectorModeUVE::MultiSelection);
        EXPECT_FALSE(toggled.snapshot.inspector.canEditSelectedName);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, SnapshotUVE_BoundsCopiedPanelRowsWithoutClaimingDeletion) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_panel_bound.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        for (std::size_t index = 0U; index < kEditorBridgeMaximumPanelEntriesUVE + 1U; ++index) {
            ASSERT_NE(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);
        }

        const EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();
        EXPECT_EQ(snapshot.hierarchy.entries.size(), kEditorBridgeMaximumPanelEntriesUVE);
        EXPECT_TRUE(snapshot.hierarchy.truncated);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
