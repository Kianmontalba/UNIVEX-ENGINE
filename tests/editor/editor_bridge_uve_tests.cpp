// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"
#include "uve/scene/components/mesh_component_uve.h"

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

TEST(EditorBridgeUVETest, VisualScriptGraphSchemaUVE_IsAdvertisedAndUsesNativeAuthority) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_graph_schema.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        bool serializeAdvertised = false;
        bool deserializeAdvertised = false;
        bool addTypeAdvertised = false;
        bool pinDefaultAdvertised = false;
        for (const EditorBridgeCapabilityUVE capability : initial.capabilities) {
            serializeAdvertised = serializeAdvertised ||
                                  capability == EditorBridgeCapabilityUVE::SerializeVisualScriptGraph;
            deserializeAdvertised = deserializeAdvertised ||
                                    capability == EditorBridgeCapabilityUVE::DeserializeVisualScriptGraph;
            addTypeAdvertised = addTypeAdvertised ||
                                capability == EditorBridgeCapabilityUVE::AddVisualScriptNodeType;
            pinDefaultAdvertised = pinDefaultAdvertised ||
                                   capability == EditorBridgeCapabilityUVE::SetVisualScriptPinDefault;
        }
        ASSERT_TRUE(serializeAdvertised);
        ASSERT_TRUE(deserializeAdvertised);
        ASSERT_TRUE(addTypeAdvertised);
        ASSERT_TRUE(pinDefaultAdvertised);

        EditorBridgeRequestUVE defaultRequest{};
        defaultRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        defaultRequest.requestId = 151U;
        defaultRequest.expectedRevision = initial.revision;
        defaultRequest.kind = EditorBridgeRequestKindUVE::SetVisualScriptPinDefault;
        const EditorBridgeResponseUVE rejectedDefault = bridge.DispatchUVE(defaultRequest);
        EXPECT_FALSE(rejectedDefault.applied);
        EXPECT_EQ(rejectedDefault.code, "bridge.visual_scripting.request.invalid");

        EditorBridgeRequestUVE addTypeRequest{};
        addTypeRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        addTypeRequest.requestId = 146U;
        addTypeRequest.expectedRevision = initial.revision;
        addTypeRequest.kind = EditorBridgeRequestKindUVE::AddVisualScriptNodeType;
        addTypeRequest.visualScriptNodeTypeId = "missing.palette.node";
        addTypeRequest.visualScriptPosition = Scripting::ScriptGraphCanvasPointUVE{24.0F, 48.0F};
        const EditorBridgeResponseUVE rejectedAdd = bridge.DispatchUVE(addTypeRequest);
        EXPECT_FALSE(rejectedAdd.applied);
        EXPECT_EQ(rejectedAdd.code, "bridge.command.rejected");

        EditorBridgeRequestUVE serializeRequest{};
        serializeRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        serializeRequest.requestId = 148U;
        serializeRequest.expectedRevision = initial.revision + 100U;
        serializeRequest.kind = EditorBridgeRequestKindUVE::SerializeVisualScriptGraph;
        const EditorBridgeResponseUVE serialized = bridge.DispatchUVE(serializeRequest);
        ASSERT_TRUE(serialized.applied);
        ASSERT_TRUE(serialized.visualScriptGraphSchema.has_value());
        EXPECT_EQ(serialized.visualScriptGraphSchema->schemaVersion,
                  Scripting::kScriptGraphSchemaVersionUVE);
        EXPECT_EQ(serialized.visualScriptGraphSchema->metadata.at("assetType"), "visual-script-graph");

        EditorBridgeRequestUVE deserializeRequest{};
        deserializeRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        deserializeRequest.requestId = 149U;
        deserializeRequest.expectedRevision = serialized.snapshot.revision;
        deserializeRequest.kind = EditorBridgeRequestKindUVE::DeserializeVisualScriptGraph;
        deserializeRequest.visualScriptGraphSchema =
            R"({"schemaVersion":1,"nodes":[],"links":[],"layout":[],"metadata":{"assetType":"visual-script-graph"}})";
        const EditorBridgeResponseUVE deserialized = bridge.DispatchUVE(deserializeRequest);
        EXPECT_TRUE(deserialized.applied);
        EXPECT_EQ(deserialized.code, "bridge.visual_scripting.graph_schema.deserialized");
        EXPECT_TRUE(deserialized.visualScriptGraphSchema.has_value());

        deserializeRequest.requestId = 150U;
        deserializeRequest.expectedRevision = deserialized.snapshot.revision;
        deserializeRequest.visualScriptGraphSchema =
            R"({"schemaVersion":1,"nodes":[],"links":[],"layout":[],"metadata":{},"future":true})";
        const EditorBridgeResponseUVE rejected = bridge.DispatchUVE(deserializeRequest);
        EXPECT_FALSE(rejected.applied);
        EXPECT_EQ(rejected.code, "bridge.visual_scripting.graph_schema.invalid");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, ReadScriptRuntimeUVE_IsAdvertisedAndReadOnly) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_script_runtime_request.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();

        bool advertised = false;
        for (const EditorBridgeCapabilityUVE capability : initial.capabilities) {
            advertised = advertised || capability == EditorBridgeCapabilityUVE::ReadScriptRuntime;
        }
        ASSERT_TRUE(advertised);

        EditorBridgeRequestUVE request{};
        request.protocolVersion = kEditorBridgeProtocolVersionUVE;
        request.requestId = 77U;
        request.expectedRevision = initial.revision + 100U;
        request.kind = EditorBridgeRequestKindUVE::ReadScriptRuntime;
        const EditorBridgeResponseUVE response = bridge.DispatchUVE(request);

        EXPECT_TRUE(response.applied);
        EXPECT_EQ(response.code, "bridge.script_runtime.snapshot.read");
        EXPECT_EQ(response.snapshot.revision, initial.revision);
        EXPECT_FALSE(response.snapshot.scriptRuntime.available);
        EXPECT_EQ(response.snapshot.scriptRuntime.instanceCount, 0U);
        EXPECT_EQ(response.snapshot.scriptRuntime.reason,
                  "No native ScriptRuntime is attached to this bridge session.");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, ReadScriptRuntimeTickDiagnosticsUVE_IsAdvertisedAndDispatchesCopiedSummary) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_script_runtime_tick.uvescene");
        editor.InitUVE();
        const Scene::EntityUVE entity = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Cube);
        ASSERT_NE(entity, Scene::kInvalidEntityUVE);
        Scripting::ScriptRuntimeUVE runtime;
        Scripting::ScriptBytecodeProgramUVE program;
        program.instructions.resize(2U);
        ASSERT_TRUE(runtime.AttachUVE(entity, std::move(program)));
        EditorBridgeUVE bridge(editor, nullptr, nullptr, &runtime);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        bool advertised = false;
        for (const EditorBridgeCapabilityUVE capability : initial.capabilities) {
            advertised = advertised || capability == EditorBridgeCapabilityUVE::ReadScriptRuntimeTickDiagnostics;
        }
        ASSERT_TRUE(advertised);

        EditorBridgeRequestUVE request{};
        request.protocolVersion = kEditorBridgeProtocolVersionUVE;
        request.requestId = 142U;
        request.expectedRevision = initial.revision + 100U;
        request.kind = EditorBridgeRequestKindUVE::ReadScriptRuntimeTickDiagnostics;
        const EditorBridgeResponseUVE response = bridge.DispatchUVE(request);

        ASSERT_TRUE(response.applied);
        EXPECT_EQ(response.code, "bridge.script_runtime.tick.completed");
        EXPECT_TRUE(response.snapshot.scriptRuntimeTickSummary.available);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickSummary.enabledInstanceCount, 1U);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickSummary.completedCount, 1U);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickSummary.instructionBudgetExceededCount, 0U);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickSummary.invalidInstructionCount, 0U);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickSummary.diagnosticCount, 0U);
        EXPECT_EQ(response.snapshot.revision, initial.revision + 1U);
        EXPECT_FALSE(response.snapshot.scriptRuntimeTickHistoryTruncated);
        ASSERT_EQ(response.snapshot.scriptRuntimeTickHistory.size(), 1U);
        EXPECT_EQ(response.snapshot.scriptRuntimeTickHistory.front().sequence, 1U);

        EditorBridgeResponseUVE latest = response;
        for (std::uint64_t requestId = 143U; requestId <= 151U; ++requestId) {
            request.requestId = requestId;
            latest = bridge.DispatchUVE(request);
        }
        ASSERT_EQ(latest.snapshot.scriptRuntimeTickHistory.size(), kEditorBridgeMaximumScriptRuntimeTickHistoryUVE);
        EXPECT_TRUE(latest.snapshot.scriptRuntimeTickHistoryTruncated);
        EXPECT_EQ(latest.snapshot.scriptRuntimeTickHistory.front().sequence, 3U);
        EXPECT_EQ(latest.snapshot.scriptRuntimeTickHistory.back().sequence, 10U);
        EXPECT_EQ(latest.snapshot.revision, initial.revision + 10U);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
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
        auto& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        entityManager.AddComponentUVE<Scene::MeshComponentUVE>(
            child, Scene::MeshComponentUVE{Asset::AssetGuidUVE{0x1111U}, Asset::AssetGuidUVE{0x2222U}});

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
        ASSERT_TRUE(snapshot.inspector.assetBinding.has_value());
        ASSERT_TRUE(snapshot.inspector.assetBinding->meshGuid.has_value());
        ASSERT_TRUE(snapshot.inspector.assetBinding->materialGuid.has_value());
        EXPECT_EQ(*snapshot.inspector.assetBinding->meshGuid, 0x1111U);
        EXPECT_EQ(*snapshot.inspector.assetBinding->materialGuid, 0x2222U);
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

TEST(EditorBridgeUVETest, MotionQueryReplayHistoryUVE_IsBoundedAndSequenceOrdered) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_replay_history.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        for (std::uint64_t sourceGeneration = 1U;
             sourceGeneration <= kEditorBridgeMaximumMotionQueryReplayHistoryUVE + 2U;
             ++sourceGeneration) {
            Plugins::Editor::MotionQueryTraceReplayFixtureUVE fixture;
            fixture.compatibility = Plugins::Editor::MotionQueryTraceReplayCompatibilityUVE{
                1U, 2U, 3U, sourceGeneration};
            bridge.SetMotionQueryReplayFixtureUVE(std::move(fixture));
        }
        const EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();
        ASSERT_EQ(snapshot.motionQuery.replayComparisonHistory.size(),
                  kEditorBridgeMaximumMotionQueryReplayHistoryUVE);
        EXPECT_TRUE(snapshot.motionQuery.replayComparisonHistoryTruncated);
        EXPECT_EQ(snapshot.motionQuery.replayComparisonHistory.front().sequence, 3U);
        EXPECT_EQ(snapshot.motionQuery.replayComparisonHistory.back().sequence,
                  kEditorBridgeMaximumMotionQueryReplayHistoryUVE + 2U);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeUVETest, MotionQueryUVE_ExposesCopiedSnapshotAndRevisionGuardedNamedAuthoringCommand) {
    Core::EngineCoreUVE engine(MakeBridgeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_motion_query.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        bool readAdvertised = false;
        bool dispatchAdvertised = false;
        bool replayLoadAdvertised = false;
        bool replayClearAdvertised = false;
        for (const EditorBridgeCapabilityUVE capability : initial.capabilities) {
            readAdvertised = readAdvertised || capability == EditorBridgeCapabilityUVE::ReadMotionQuery;
            dispatchAdvertised = dispatchAdvertised ||
                                 capability == EditorBridgeCapabilityUVE::DispatchMotionQueryCommand;
            replayLoadAdvertised = replayLoadAdvertised ||
                                   capability == EditorBridgeCapabilityUVE::LoadMotionQueryReplayBaseline;
            replayClearAdvertised = replayClearAdvertised ||
                                    capability == EditorBridgeCapabilityUVE::ClearMotionQueryReplayBaseline;
        }
        ASSERT_TRUE(readAdvertised);
        ASSERT_TRUE(dispatchAdvertised);
        ASSERT_TRUE(replayLoadAdvertised);
        ASSERT_TRUE(replayClearAdvertised);

        EditorBridgeRequestUVE readRequest{};
        readRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        readRequest.requestId = 500U;
        readRequest.expectedRevision = initial.revision + 100U;
        readRequest.kind = EditorBridgeRequestKindUVE::ReadMotionQuery;
        const EditorBridgeResponseUVE readResponse = bridge.DispatchUVE(readRequest);
        ASSERT_TRUE(readResponse.applied);
        EXPECT_EQ(readResponse.code, "bridge.motion_query.snapshot.read");
        EXPECT_TRUE(readResponse.snapshot.motionQuery.authoring.databases.empty());

        Core::MotionQueryDatabaseContractUVE contract;
        contract.context.databaseId = "bridge-db";
        contract.context.generation = 1U;
        contract.schema.schemaId = "bridge-schema";
        contract.settings.maximumCandidates = 4U;
        Core::MotionMatchingCandidateUVE candidate;
        candidate.candidateId = "bridge-candidate";
        candidate.sourceClipId = "walk";
        candidate.feature.facingDirection = Math::Vector3UVE{0.0F, 0.0F, 1.0F};
        contract.database.candidates = {candidate};
        Plugins::Editor::MotionQueryEditorDatabaseEntryUVE entry;
        entry.resource = Asset::ResourceHandleUVE{Asset::AssetGuidUVE{77U}, 1U};
        entry.displayName = "Bridge Motion Database";
        entry.contract = contract;

        EditorBridgeRequestUVE dispatchRequest{};
        dispatchRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        dispatchRequest.requestId = 501U;
        dispatchRequest.expectedRevision = initial.revision;
        dispatchRequest.kind = EditorBridgeRequestKindUVE::DispatchMotionQueryCommand;
        Plugins::Editor::MotionQueryEditorCommandUVE command;
        command.requestId = 501U;
        command.expectedRevision = 0U;
        command.kind = Plugins::Editor::MotionQueryEditorCommandKindUVE::RegisterDatabase;
        command.database = entry;
        dispatchRequest.motionQueryCommand = command;
        const EditorBridgeResponseUVE dispatchResponse = bridge.DispatchUVE(dispatchRequest);
        ASSERT_TRUE(dispatchResponse.applied) << dispatchResponse.message;
        EXPECT_EQ(dispatchResponse.code, "bridge.motion_query.command.applied");
        ASSERT_EQ(dispatchResponse.snapshot.motionQuery.authoring.databases.size(), 1U);
        EXPECT_EQ(dispatchResponse.snapshot.motionQuery.authoring.databases.front().displayName,
                  "Bridge Motion Database");
        EXPECT_EQ(dispatchResponse.snapshot.motionQuery.authoring.revision, 1U);
        EXPECT_EQ(dispatchResponse.snapshot.revision, initial.revision + 1U);

        EditorBridgeRequestUVE debugRequest{};
        debugRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        debugRequest.requestId = 503U;
        debugRequest.expectedRevision = dispatchResponse.snapshot.revision;
        debugRequest.kind = EditorBridgeRequestKindUVE::DispatchMotionQueryDebugCommand;
        Plugins::Editor::MotionQueryLiveDebugCommandUVE attachCommand;
        attachCommand.requestId = 503U;
        attachCommand.expectedGeneration = 0U;
        attachCommand.kind = Plugins::Editor::MotionQueryLiveDebugCommandKindUVE::Attach;
        attachCommand.database = entry.resource;
        debugRequest.motionQueryDebugCommand = attachCommand;
        const EditorBridgeResponseUVE debugResponse = bridge.DispatchUVE(debugRequest);
        ASSERT_TRUE(debugResponse.applied) << debugResponse.message;
        EXPECT_TRUE(debugResponse.snapshot.motionQuery.liveDebugActive);
        EXPECT_EQ(debugResponse.snapshot.motionQuery.liveDebugDatabase, entry.resource);
        EXPECT_EQ(debugResponse.snapshot.motionQuery.liveDebugGeneration, 1U);
        EXPECT_EQ(debugResponse.snapshot.revision, dispatchResponse.snapshot.revision + 1U);

        Plugins::Editor::MotionQueryTraceReplayFixtureUVE replayFixture;
        bridge.SetMotionQueryReplayFixtureUVE(replayFixture);
        const EditorBridgeSnapshotUVE replaySnapshot = bridge.GetSnapshotUVE();
        EXPECT_TRUE(replaySnapshot.motionQuery.replayComparison.available);
        EXPECT_EQ(replaySnapshot.motionQuery.replayComparison.code,
                  static_cast<std::uint8_t>(Plugins::Editor::MotionQueryTraceReplayRegressionCodeUVE::EmptyTrace));
        EXPECT_FALSE(replaySnapshot.motionQuery.replayComparison.message.empty());
        bridge.ClearMotionQueryReplayFixtureUVE();
        EXPECT_FALSE(bridge.GetSnapshotUVE().motionQuery.replayComparison.available);

        Plugins::Editor::MotionQueryTraceReplayFixtureUVE commandFixture;
        const Plugins::Editor::MotionQueryTraceReplaySerializationResultUVE encodedFixture =
            Plugins::Editor::SerializeMotionQueryTraceReplayFixtureUVE(commandFixture);
        ASSERT_TRUE(encodedFixture.IsAcceptedUVE());
        EditorBridgeRequestUVE loadReplayRequest{};
        loadReplayRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        loadReplayRequest.requestId = 504U;
        loadReplayRequest.expectedRevision = bridge.GetSnapshotUVE().revision;
        loadReplayRequest.kind = EditorBridgeRequestKindUVE::LoadMotionQueryReplayBaseline;
        loadReplayRequest.motionQueryReplayBaselineName = "bridge-baseline";
        loadReplayRequest.motionQueryReplayFixturePayload = encodedFixture.payload;
        const std::size_t historyBeforeLoad = bridge.GetSnapshotUVE().motionQuery.replayComparisonHistory.size();
        const EditorBridgeResponseUVE loadReplayResponse = bridge.DispatchUVE(loadReplayRequest);
        ASSERT_TRUE(loadReplayResponse.applied) << loadReplayResponse.message;
        EXPECT_EQ(loadReplayResponse.code, "bridge.motion_query.replay.baseline.loaded");
        EXPECT_TRUE(loadReplayResponse.snapshot.motionQuery.replayComparison.available);
        ASSERT_EQ(loadReplayResponse.snapshot.motionQuery.replayBaselines.entries.size(), 1U);
        EXPECT_EQ(loadReplayResponse.snapshot.motionQuery.replayBaselines.entries.front().name,
                  "bridge-baseline");
        EXPECT_TRUE(loadReplayResponse.snapshot.motionQuery.replayWorkflow.activeBaselineSelected);
        EXPECT_TRUE(loadReplayResponse.snapshot.motionQuery.replayWorkflow.activeFixtureAvailable);
        EXPECT_FALSE(loadReplayResponse.snapshot.motionQuery.replayWorkflow.readyForComparison);
        EXPECT_FALSE(loadReplayResponse.snapshot.motionQuery.replayWorkflow.diagnostic.empty());
        EXPECT_TRUE(loadReplayResponse.snapshot.motionQuery.replayBatch.available);
        EXPECT_EQ(loadReplayResponse.snapshot.motionQuery.replayBatch.evaluatedBaselineCount, 1U);
        ASSERT_EQ(loadReplayResponse.snapshot.motionQuery.replayBatch.results.size(), 1U);
        EXPECT_EQ(loadReplayResponse.snapshot.motionQuery.replayBatch.results.front().baselineName,
                  "bridge-baseline");
        EditorBridgeRequestUVE batchRequest{};
        batchRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        batchRequest.requestId = 507U;
        batchRequest.expectedRevision = loadReplayResponse.snapshot.revision;
        batchRequest.kind = EditorBridgeRequestKindUVE::RunMotionQueryReplayBaselineBatch;
        const std::size_t batchHistoryBefore = loadReplayResponse.snapshot.motionQuery.replayBatchHistory.size();
        const std::size_t batchRunsBefore = loadReplayResponse.snapshot.motionQuery.replaySessionFacts.totalBatchRuns;
        const EditorBridgeResponseUVE batchResponse = bridge.DispatchUVE(batchRequest);
        EXPECT_FALSE(batchResponse.applied);
        EXPECT_EQ(batchResponse.code, "bridge.motion_query.replay.baseline.batch.read");
        EXPECT_EQ(batchResponse.snapshot.revision, loadReplayResponse.snapshot.revision);
        EXPECT_EQ(batchResponse.snapshot.motionQuery.replayBatch.evaluatedBaselineCount, 1U);
        ASSERT_EQ(batchResponse.snapshot.motionQuery.replayBatchHistory.size(), batchHistoryBefore + 1U);
        EXPECT_EQ(batchResponse.snapshot.motionQuery.replayBatchHistory.back().evaluatedBaselineCount, 1U);
        EXPECT_EQ(batchResponse.snapshot.motionQuery.replaySessionFacts.totalBatchRuns, batchRunsBefore + 1U);
        ASSERT_EQ(loadReplayResponse.snapshot.motionQuery.replayComparisonHistory.size(), historyBeforeLoad + 1U);
        EXPECT_EQ(loadReplayResponse.snapshot.motionQuery.replayComparisonHistory.back().baselineName,
                  "bridge-baseline");

        EditorBridgeRequestUVE staleClearRequest{};
        staleClearRequest.protocolVersion = kEditorBridgeProtocolVersionUVE;
        staleClearRequest.requestId = 505U;
        staleClearRequest.expectedRevision = loadReplayRequest.expectedRevision;
        staleClearRequest.kind = EditorBridgeRequestKindUVE::ClearMotionQueryReplayBaseline;
        staleClearRequest.motionQueryReplayBaselineName = "bridge-baseline";
        EXPECT_EQ(bridge.DispatchUVE(staleClearRequest).code, "bridge.snapshot.stale");

        EditorBridgeRequestUVE clearReplayRequest = staleClearRequest;
        clearReplayRequest.requestId = 506U;
        clearReplayRequest.expectedRevision = loadReplayResponse.snapshot.revision;
        const EditorBridgeResponseUVE clearReplayResponse = bridge.DispatchUVE(clearReplayRequest);
        ASSERT_TRUE(clearReplayResponse.applied) << clearReplayResponse.message;
        EXPECT_EQ(clearReplayResponse.code, "bridge.motion_query.replay.baseline.cleared");
        EXPECT_FALSE(clearReplayResponse.snapshot.motionQuery.replayComparison.available);
        EXPECT_TRUE(clearReplayResponse.snapshot.motionQuery.replayBaselines.entries.empty());
        EXPECT_FALSE(clearReplayResponse.snapshot.motionQuery.replayWorkflow.activeBaselineSelected);
        EXPECT_FALSE(clearReplayResponse.snapshot.motionQuery.replayWorkflow.readyForComparison);
        EXPECT_TRUE(clearReplayResponse.snapshot.motionQuery.replayBatch.available);
        EXPECT_EQ(clearReplayResponse.snapshot.motionQuery.replayBatch.evaluatedBaselineCount, 0U);
        EXPECT_TRUE(clearReplayResponse.snapshot.motionQuery.replayBatch.results.empty());
        ASSERT_EQ(clearReplayResponse.snapshot.motionQuery.replayComparisonHistory.size(), historyBeforeLoad + 2U);
        EXPECT_GT(clearReplayResponse.snapshot.motionQuery.replayComparisonHistory.back().sequence,
                  loadReplayResponse.snapshot.motionQuery.replayComparisonHistory.back().sequence);
        EXPECT_EQ(clearReplayResponse.snapshot.motionQuery.replayComparisonHistory.back().baselineName,
                  "bridge-baseline");

        dispatchRequest.requestId = 502U;
        dispatchRequest.expectedRevision = initial.revision;
        command.requestId = 502U;
        command.expectedRevision = 1U;
        dispatchRequest.motionQueryCommand = command;
        const EditorBridgeResponseUVE staleBridge = bridge.DispatchUVE(dispatchRequest);
        EXPECT_FALSE(staleBridge.applied);
        EXPECT_EQ(staleBridge.code, "bridge.snapshot.stale");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
