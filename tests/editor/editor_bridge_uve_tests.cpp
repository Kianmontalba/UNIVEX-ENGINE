// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <filesystem>
#include <fstream>
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

TEST(EditorBridgeUVETest, ContentBrowserImportCapabilityUVE_ReportsRawParserBoundaryForSelectedEntry) {
    const std::filesystem::path contentRoot = "uve_editor_bridge_content_import_capability";
    std::filesystem::remove_all(contentRoot);
    std::filesystem::create_directories(contentRoot);
    {
        std::ofstream fixture(contentRoot / "character.fbx", std::ios::binary);
        ASSERT_TRUE(fixture.is_open());
        fixture << "raw model source";
    }
    {
        std::ofstream fixture(contentRoot / "notes.txt", std::ios::binary);
        ASSERT_TRUE(fixture.is_open());
        fixture << "plain source document";
    }
    {
        std::ofstream fixture(contentRoot / "basic.vert", std::ios::binary);
        ASSERT_TRUE(fixture.is_open());
        fixture << "#version 450\nvoid main() {}\n";
    }

    Core::EngineConfigUVE config = MakeBridgeTestConfigUVE();
    config.projectContentRootUVE = contentRoot;
    Core::EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_content_import_capability.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);

        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        EditorBridgeRequestUVE refresh{};
        refresh.protocolVersion = kEditorBridgeProtocolVersionUVE;
        refresh.requestId = 1U;
        refresh.expectedRevision = initial.revision;
        refresh.kind = EditorBridgeRequestKindUVE::RefreshContentBrowser;
        const EditorBridgeResponseUVE refreshed = bridge.DispatchUVE(refresh);
        ASSERT_TRUE(refreshed.applied);

        EditorBridgeRequestUVE select{};
        select.protocolVersion = kEditorBridgeProtocolVersionUVE;
        select.requestId = 2U;
        select.expectedRevision = refreshed.snapshot.revision;
        select.kind = EditorBridgeRequestKindUVE::SelectContentBrowserEntry;
        select.contentEntryPath = "character.fbx";
        const EditorBridgeResponseUVE selected = bridge.DispatchUVE(select);
        ASSERT_TRUE(selected.applied);
        ASSERT_TRUE(selected.snapshot.contentBrowser.selectedEntry.has_value());
        EXPECT_EQ(selected.snapshot.contentBrowser.selectedEntry->relativePath, "character.fbx");
        EXPECT_TRUE(selected.snapshot.contentBrowser.importAction.hasSelection);
        EXPECT_FALSE(selected.snapshot.contentBrowser.importAction.canImport);
        EXPECT_FALSE(selected.snapshot.contentBrowser.importAction.canReimport);
        EXPECT_FALSE(selected.snapshot.contentBrowser.importAction.importerRegistered);
        EXPECT_TRUE(selected.snapshot.contentBrowser.importAction.requiresFormatSpecificParser);
        EXPECT_EQ(selected.snapshot.contentBrowser.importAction.sourceKind, "rawModel");
        EXPECT_EQ(selected.snapshot.contentBrowser.importAction.diagnostic,
                  "format-specific parser is not registered");

        EditorBridgeRequestUVE selectShader{};
        selectShader.protocolVersion = kEditorBridgeProtocolVersionUVE;
        selectShader.requestId = 3U;
        selectShader.expectedRevision = selected.snapshot.revision;
        selectShader.kind = EditorBridgeRequestKindUVE::SelectContentBrowserEntry;
        selectShader.contentEntryPath = "basic.vert";
        const EditorBridgeResponseUVE selectedShader = bridge.DispatchUVE(selectShader);
        ASSERT_TRUE(selectedShader.applied);
        EXPECT_TRUE(selectedShader.snapshot.contentBrowser.importAction.hasSelection);
        EXPECT_TRUE(selectedShader.snapshot.contentBrowser.importAction.canImport);
        EXPECT_FALSE(selectedShader.snapshot.contentBrowser.importAction.canReimport);
        EXPECT_TRUE(selectedShader.snapshot.contentBrowser.importAction.importerRegistered);
        EXPECT_TRUE(selectedShader.snapshot.contentBrowser.importAction.requiresFormatSpecificParser);
        EXPECT_EQ(selectedShader.snapshot.contentBrowser.importAction.sourceKind, "rawShader");
        EXPECT_EQ(selectedShader.snapshot.contentBrowser.importAction.diagnostic,
                  "format-specific parser is registered");

        EditorBridgeRequestUVE selectPlain{};
        selectPlain.protocolVersion = kEditorBridgeProtocolVersionUVE;
        selectPlain.requestId = 4U;
        selectPlain.expectedRevision = selectedShader.snapshot.revision;
        selectPlain.kind = EditorBridgeRequestKindUVE::SelectContentBrowserEntry;
        selectPlain.contentEntryPath = "notes.txt";
        const EditorBridgeResponseUVE selectedPlain = bridge.DispatchUVE(selectPlain);
        ASSERT_TRUE(selectedPlain.applied);

        EditorBridgeRequestUVE queueImport{};
        queueImport.protocolVersion = kEditorBridgeProtocolVersionUVE;
        queueImport.requestId = 5U;
        queueImport.expectedRevision = selectedPlain.snapshot.revision;
        queueImport.kind = EditorBridgeRequestKindUVE::QueueContentBrowserImport;
        queueImport.contentEntryPath = "notes.txt";
        queueImport.contentImportDestinationPath = "notes_imported.txt";
        const EditorBridgeResponseUVE queued = bridge.DispatchUVE(queueImport);
        ASSERT_TRUE(queued.applied);
        EXPECT_EQ(queued.code, "bridge.content.import.queued");
        ASSERT_TRUE(queued.contentImportJobId.has_value());
        EXPECT_EQ(*queued.contentImportJobId, 1U);

        Asset::IAssetImportQueueUVE& importQueue = engine.GetServicesUVE().GetAssetImportQueueUVE();
        ASSERT_TRUE(importQueue.TickUVE());
        const std::vector<Asset::AssetImportJobUVE> jobs = importQueue.GetJobsUVE();
        ASSERT_EQ(jobs.size(), 1U);
        EXPECT_EQ(jobs.front().state, Asset::AssetImportJobStateUVE::Succeeded);
        EXPECT_TRUE(jobs.front().resultGuid.has_value());
        EXPECT_TRUE(std::filesystem::exists(contentRoot / "notes_imported.txt"));

        editor.ShutdownUVE();
    }
    engine.Shutdown();
    std::filesystem::remove_all(contentRoot);
}

TEST(EditorBridgeUVETest, ContentBrowserAnimationEnvelopeUVE_ReportsTypedImporterAuthority) {
    const std::filesystem::path contentRoot = "uve_editor_bridge_animation_asset";
    std::filesystem::remove_all(contentRoot);
    std::filesystem::create_directories(contentRoot);
    {
        std::ofstream fixture(contentRoot / "walk.uveanim", std::ios::binary);
        ASSERT_TRUE(fixture.is_open());
        fixture << "typed animation envelope placeholder";
    }

    Core::EngineConfigUVE config = MakeBridgeTestConfigUVE();
    config.projectContentRootUVE = contentRoot;
    Core::EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_animation_asset.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();

        EditorBridgeRequestUVE refresh{};
        refresh.protocolVersion = kEditorBridgeProtocolVersionUVE;
        refresh.requestId = 1U;
        refresh.expectedRevision = initial.revision;
        refresh.kind = EditorBridgeRequestKindUVE::RefreshContentBrowser;
        const EditorBridgeResponseUVE refreshed = bridge.DispatchUVE(refresh);
        ASSERT_TRUE(refreshed.applied);

        EditorBridgeRequestUVE select{};
        select.protocolVersion = kEditorBridgeProtocolVersionUVE;
        select.requestId = 2U;
        select.expectedRevision = refreshed.snapshot.revision;
        select.kind = EditorBridgeRequestKindUVE::SelectContentBrowserEntry;
        select.contentEntryPath = "walk.uveanim";
        const EditorBridgeResponseUVE selected = bridge.DispatchUVE(select);
        ASSERT_TRUE(selected.applied);
        ASSERT_TRUE(selected.snapshot.contentBrowser.importAction.hasSelection);
        EXPECT_TRUE(selected.snapshot.contentBrowser.importAction.canImport);
        EXPECT_TRUE(selected.snapshot.contentBrowser.importAction.importerRegistered);
        EXPECT_FALSE(selected.snapshot.contentBrowser.importAction.requiresFormatSpecificParser);
        EXPECT_EQ(selected.snapshot.contentBrowser.importAction.sourceKind, "animationEnvelope");
        EXPECT_EQ(selected.snapshot.contentBrowser.importAction.diagnostic,
                  "built-in generic copy importer is registered");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
    std::filesystem::remove_all(contentRoot);
}

TEST(EditorBridgeUVETest, ContentBrowserMotionQueryFocusUVE_ParsesFocusAndCopiesTypedEntry) {
    const std::filesystem::path contentRoot = "uve_editor_bridge_motion_query_focus";
    std::filesystem::remove_all(contentRoot);
    std::filesystem::create_directories(contentRoot / "Queries");
    {
        std::ofstream fixture(contentRoot / "Queries" / "Locomotion.uvemotionquery", std::ios::binary);
        ASSERT_TRUE(fixture.is_open());
        fixture << "typed motion query asset placeholder";
    }

    Core::EngineConfigUVE config = MakeBridgeTestConfigUVE();
    config.projectContentRootUVE = contentRoot;
    Core::EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_motion_query_focus.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();

        EditorBridgeRequestUVE refresh{};
        refresh.protocolVersion = kEditorBridgeProtocolVersionUVE;
        refresh.requestId = 1U;
        refresh.expectedRevision = initial.revision;
        refresh.kind = EditorBridgeRequestKindUVE::RefreshContentBrowser;
        const EditorBridgeResponseUVE refreshed = bridge.DispatchUVE(refresh);
        ASSERT_TRUE(refreshed.applied);

        EditorBridgeRequestUVE directory{};
        directory.protocolVersion = kEditorBridgeProtocolVersionUVE;
        directory.requestId = 2U;
        directory.expectedRevision = refreshed.snapshot.revision;
        directory.kind = EditorBridgeRequestKindUVE::SetContentBrowserDirectory;
        directory.contentDirectory = "Queries";
        const EditorBridgeResponseUVE navigated = bridge.DispatchUVE(directory);
        ASSERT_TRUE(navigated.applied);

        EditorBridgeRequestUVE focus{};
        focus.protocolVersion = kEditorBridgeProtocolVersionUVE;
        focus.requestId = 3U;
        focus.expectedRevision = navigated.snapshot.revision;
        focus.kind = EditorBridgeRequestKindUVE::SetContentBrowserFocus;
        focus.contentFocus = "motionQuery";
        const EditorBridgeResponseUVE focused = bridge.DispatchUVE(focus);
        ASSERT_TRUE(focused.applied);
        EXPECT_EQ(focused.snapshot.contentBrowser.typeFocus, "Motion Query");
        ASSERT_EQ(focused.snapshot.contentBrowser.entries.size(), 1U);
        EXPECT_EQ(focused.snapshot.contentBrowser.entries.front().relativePath, "Queries/Locomotion.uvemotionquery");
        EXPECT_EQ(focused.snapshot.contentBrowser.entries.front().typeLabel, "Motion Query");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
    std::filesystem::remove_all(contentRoot);
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
        EXPECT_EQ(snapshot.inspector.parent->displayLabel, "Bridge Root");
        EXPECT_EQ(snapshot.inspector.eligibleDrawerIds,
                  (std::vector<std::string>{"name", "hierarchy", "transform", "primitive-mesh", "mesh", "collider"}));
        EXPECT_EQ(snapshot.inspector.attachedComponentIds, (std::vector<std::string>{"mesh", "collider"}));
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
        ASSERT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds.size(), 161U);
        ASSERT_EQ(snapshot.visualScripting.canvas.paletteDescriptors.size(), 161U);
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[0], "flow.sequence");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[1], "flow.branch");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[2], "flow.return");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[3], "flow.do_once");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[4], "flow.gate");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[5], "flow.switch");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[6], "flow.event");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[7], "flow.loop");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[8], "flow.for_loop");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[9], "flow.while_loop");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[10], "flow.delay");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[11], "convert.number_to_boolean");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[12], "convert.boolean_to_number");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[13], "convert.vector2_to_vector3");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[14], "convert.vector3_to_vector2");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[15], "math.float.add");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[19], "math.float.modulo");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[20], "math.float.abs");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[24], "math.float.power");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[25], "math.float.lerp");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[26], "math.float.remap");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[27], "math.float.sin");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[28], "math.float.cos");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[29], "math.float.tan");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[30], "math.float.sqrt");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[31], "math.float.random");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[32], "math.float.random_range");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[33], "math.vector2.make");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[38], "math.vector2.normalize");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[39], "math.vector2.dot");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[40], "math.vector2.distance");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[41], "math.vector2.direction");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[42], "math.vector2.lerp");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[43], "math.vector3.make");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[50], "math.vector3.normalize");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[51], "math.vector3.distance");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[52], "math.vector3.direction");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[53], "math.vector3.lerp");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[54], "math.rotation.make");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[55], "math.rotation.break");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[56], "math.rotation.degrees");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[57], "math.rotation.radians");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[58], "math.rotation.euler");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[59], "math.rotation.quaternion");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[60], "math.rotation.look_at");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[61], "math.rotation.slerp");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[62], "math.rotation.rotate");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[66], "logic.boolean.xor");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[67], "logic.boolean.equal");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[68], "logic.boolean.not_equal");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[69], "logic.boolean.greater");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[70], "logic.boolean.less");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[71], "logic.boolean.greater_equal");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[72], "logic.boolean.less_equal");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[73], "math.transform.make");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[74], "math.transform.break");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[75], "math.transform.get_position");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[76], "math.transform.set_position");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[77], "math.transform.get_rotation");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[78], "math.transform.set_rotation");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[79], "math.transform.get_scale");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[80], "math.transform.set_scale");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[81], "math.transform.translate");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[82], "math.transform.rotate");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[83], "math.transform.transform_point");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[84], "query.entity.has_component");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[85], "query.entity.get_component");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[86], "engine.log");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[87], "engine.get_time");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[88], "variable.make_number");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[89], "variable.get_number");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[90], "variable.set_number");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[91], "variable.make_boolean");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[92], "variable.get_boolean");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[93], "variable.set_boolean");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[94], "variable.make_vector3");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[95], "variable.get_vector3");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[96], "variable.set_vector3");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[97], "variable.make_array");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[98], "variable.get_array");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[99], "variable.set_array");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[100], "variable.make_map");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[101], "variable.get_map");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[102], "variable.set_map");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[103], "variable.make_set");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[104], "variable.get_set");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[105], "variable.set_set");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[106], "variable.make_struct");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[107], "variable.get_struct");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[108], "variable.set_struct");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[109], "entity.spawn");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[110], "entity.destroy");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[111], "entity.find");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[112], "entity.get_entity");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[113], "entity.add_component");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[114], "entity.remove_component");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[115], "input.key_pressed");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[116], "input.key_released");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[117], "input.key_down");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[118], "input.mouse_position");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[119], "input.mouse_button");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[120], "input.gamepad_button");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[121], "input.get_axis");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[122], "input.get_action");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[123], "camera.get_camera");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[124], "camera.set_position");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[125], "camera.set_rotation");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[126], "camera.look_at");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[127], "camera.set_fov");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[128], "camera.shake");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[129], "camera.set_active");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[130], "animation.play");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[131], "animation.stop");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[132], "animation.pause");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[133], "animation.blend");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[134], "animation.blend_space");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[135], "animation.set_speed");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[136], "animation.set_weight");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[137], "animation.montage");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[138], "animation.get_current_animation");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[139], "animation.is_playing");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[140], "physics.raycast");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[141], "physics.sphere_cast");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[142], "physics.box_cast");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[143], "physics.capsule_cast");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[144], "physics.overlap");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[145], "physics.apply_force");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[146], "physics.apply_impulse");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[147], "physics.set_velocity");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[148], "physics.get_velocity");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[149], "physics.enable_gravity");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[150], "physics.is_colliding");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[158], "debug.print");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[159], "debug.warning");
        EXPECT_EQ(snapshot.visualScripting.canvas.paletteNodeTypeIds[160], "debug.error");
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
