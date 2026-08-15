// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_runtime_uve.h"
#include "uve/scripting/script_vm_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_debugger_uve.h"
#include "uve/scripting/script_graph_editor_backend_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"
#include "uve/scripting/script_graph_persistence_uve.h"
#include "uve/scripting/script_graph_uve.h"
#include "uve/scripting/script_hot_reload_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Scripting {
namespace {

ScriptNodeTypeDescriptorUVE MakeSourceNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.source",
        "Test Source",
        {{"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
    };
}

ScriptNodeTypeDescriptorUVE MakeSinkNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.sink",
        "Test Sink",
        {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution}},
    };
}

void RegisterTestNodesUVE(ScriptNodeRegistryUVE& registry) {
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
}

} // namespace

TEST(ScriptNodeRegistryUVETest, RegisterNodeTypeUVE_RejectsDuplicateAndMalformedDescriptors) {
    ScriptNodeRegistryUVE registry;
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"", "Empty", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-display", "", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-pin", "Invalid", {{"", ScriptPinDirectionUVE::Input,
                                                                                ScriptValueTypeUVE::Number}}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.duplicate-pin", "Invalid",
                                                {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
                                                 {"Value", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}}}));
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_EQ(registry.GetNodeTypeCountUVE(), 1U);
}

TEST(ScriptNodeRegistryUVETest, FindNodeTypeUVE_ReturnsCopiedStableDescriptorView) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE("test.source");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->displayName, "Test Source");
    EXPECT_EQ(descriptor->pins.size(), 2U);
    EXPECT_EQ(registry.FindNodeTypeUVE("missing"), nullptr);
}

TEST(ScriptGraphUVETest, AddNodeUVE_RejectsEmptyTypeAndDuplicateIdsWithoutMutation) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddNodeUVE({1U, ""}));
    EXPECT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    EXPECT_FALSE(graph.AddNodeUVE({1U, "test.sink"}));
    EXPECT_EQ(graph.GetNodesUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, AddLinkUVE_RejectsEmptyEndpointsAndDuplicateLinks) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddLinkUVE({{0U, "Out"}, {2U, "In"}}));
    EXPECT_FALSE(graph.AddLinkUVE({{1U, ""}, {2U, "In"}}));
    const ScriptLinkUVE link{{1U, "Out"}, {2U, "In"}};
    EXPECT_TRUE(graph.AddLinkUVE(link));
    EXPECT_FALSE(graph.AddLinkUVE(link));
    EXPECT_EQ(graph.GetLinksUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, ValidateUVE_ValidTypedOutputToInputHasNoDiagnostics) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsUnknownNodeTypeAndUnknownPins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "Missing"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsWrongDirectionsAndIncompatibleTypes) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.boolean-sink", "Boolean Sink",
                                              {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean}}}));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{2U, "In"}, {3U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::WrongPinDirection);
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::IncompatiblePinTypes);
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsSelfLinkAndMissingNodeDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {1U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {9U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 3U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::SelfLink);
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::WrongPinDirection);
    EXPECT_EQ(diagnostics[2].code, ScriptValidationCodeUVE::EmptyLinkEndpoint);
}

TEST(ScriptPinCompatibilityUVETest, AreScriptPinTypesCompatibleUVE_RequiresExactTypes) {
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Execution, ScriptValueTypeUVE::Execution));
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Vector3, ScriptValueTypeUVE::Vector3));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Number, ScriptValueTypeUVE::Boolean));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Entity, ScriptValueTypeUVE::Asset));
}

} // namespace UVE::Scripting

namespace UVE::Scripting {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_SortsNodesAndLinksDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    EXPECT_EQ(result.program->version, ScriptIrProgramUVE::kCurrentVersionUVE);
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 20U);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[2].targetNodeId, 20U);
    EXPECT_EQ(result.program->sourceNodeIds, (std::vector<std::uint32_t>{10U, 20U, 10U}));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsInvalidGraphWithoutPartialProgram) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_FALSE(result.program.has_value());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptBytecodeUVETest, EncodeDecodeScriptBytecodeUVE_RoundTripsVersionedProgram) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In"});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 2U);
    EXPECT_EQ(decoded.program->instructions[0].nodeTypeId, "test.source");
    EXPECT_EQ(decoded.program->instructions[1].sourcePinName, "Out");
}

TEST(ScriptBytecodeUVETest, DecodeScriptBytecodeUVE_RejectsCorruptHeadersAndTruncation) {
    const ScriptBytecodeDecodeResultUVE badMagic = DecodeScriptBytecodeUVE({0U, 1U, 2U, 3U});
    ASSERT_FALSE(badMagic.IsSuccessUVE());
    ASSERT_EQ(badMagic.diagnostics.size(), 1U);
    EXPECT_EQ(badMagic.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InvalidMagic);
    const ScriptBytecodeDecodeResultUVE truncated = DecodeScriptBytecodeUVE({'U', 'V', 'E', 'S', 1U, 0U, 0U, 0U});
    ASSERT_FALSE(truncated.IsSuccessUVE());
    EXPECT_EQ(truncated.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::Truncated);
}

TEST(ScriptBytecodeUVETest, EncodeScriptBytecodeUVE_RejectsInstructionLimit) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptBytecodeProgramUVE::kMaximumInstructionsUVE + 1U);
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptBytecodeUVE(program, diagnostics).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CompletesValidProgramWithinBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 1U, 2U, {}, "Out", "In"});
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_StopsAtInstructionBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(3U);
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].instructionIndex, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RejectsUnsupportedVersion) {
    ScriptBytecodeProgramUVE program;
    program.version = 99U;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InvalidInstruction);
    EXPECT_EQ(result.instructionsExecuted, 0U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptRuntimeUVETest, AttachUVE_RejectsInvalidDuplicateAndAcceptsGenerationalIdentity) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    EXPECT_FALSE(runtime.AttachUVE(Scene::kInvalidEntityUVE, program));
    const Scene::EntityUVE first{3U, 1U};
    const Scene::EntityUVE replacement{3U, 2U};
    EXPECT_TRUE(runtime.AttachUVE(first, program));
    EXPECT_FALSE(runtime.AttachUVE(first, program));
    EXPECT_TRUE(runtime.AttachUVE(replacement, program));
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 2U);
}

TEST(ScriptRuntimeUVETest, TickUVE_IsDeterministicAndSkipsDisabledInstances) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, program));
    ASSERT_TRUE(runtime.AttachUVE({2U, 4U}, program));
    ASSERT_TRUE(runtime.AttachUVE({5U, 1U}, program));
    ASSERT_TRUE(runtime.SetEnabledUVE({5U, 1U}, false));
    const auto results = runtime.TickUVE({2U});
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].entity, (Scene::EntityUVE{2U, 4U}));
    EXPECT_EQ(results[1].entity, (Scene::EntityUVE{9U, 1U}));
    EXPECT_TRUE(results[0].execution.IsSuccessUVE());
    EXPECT_EQ(results[0].execution.instructionsExecuted, 2U);
}

TEST(ScriptRuntimeUVETest, DetachUVE_RemovesOnlyExactGenerationalHandle) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    ASSERT_TRUE(runtime.AttachUVE({4U, 1U}, program));
    EXPECT_FALSE(runtime.DetachUVE({4U, 2U}));
    EXPECT_TRUE(runtime.HasInstanceUVE({4U, 1U}));
    EXPECT_TRUE(runtime.DetachUVE({4U, 1U}));
    EXPECT_FALSE(runtime.HasInstanceUVE({4U, 1U}));
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphPersistenceUVETest, EncodeDecodeScriptGraphUVE_RoundTripsDeterministically) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphUVE(graph, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptGraphDecodeResultUVE decoded = DecodeScriptGraphUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.graph->GetNodesUVE().size(), 2U);
    EXPECT_EQ(decoded.graph->GetLinksUVE().size(), 1U);
    std::vector<ScriptPersistenceDiagnosticUVE> secondDiagnostics;
    EXPECT_EQ(EncodeScriptGraphUVE(*decoded.graph, secondDiagnostics), encoded);
    EXPECT_TRUE(secondDiagnostics.empty());
}

TEST(ScriptGraphPersistenceUVETest, DecodeScriptGraphUVE_RejectsMalformedVersionAndDuplicates) {
    const ScriptGraphDecodeResultUVE malformed = DecodeScriptGraphUVE("{not-json");
    ASSERT_FALSE(malformed.IsSuccessUVE());
    EXPECT_EQ(malformed.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::InvalidJson);
    const ScriptGraphDecodeResultUVE unsupported = DecodeScriptGraphUVE(
        R"({"schemaVersion":99,"nodes":[],"links":[]})");
    ASSERT_FALSE(unsupported.IsSuccessUVE());
    EXPECT_EQ(unsupported.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion);
    const ScriptGraphDecodeResultUVE duplicate = DecodeScriptGraphUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"},{"id":1,"typeId":"test"}],"links":[]})");
    ASSERT_FALSE(duplicate.IsSuccessUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);
}

TEST(ScriptGraphPersistenceUVETest, EncodeScriptGraphUVE_EnforcesTextLimitWithoutOutput) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptGraphUVE(graph, diagnostics, {4096U, 8192U, 4U}).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphEditorBackendUVETest, CommandsAreTransactionalAndNodeRemovalCleansIncidentLinks) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    EXPECT_FALSE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    ASSERT_TRUE(backend.RemoveNodeUVE(1U).IsAppliedUVE());
    EXPECT_TRUE(backend.GetGraphUVE().GetNodesUVE().size() == 1U);
    EXPECT_TRUE(backend.GetGraphUVE().GetLinksUVE().empty());
}

TEST(ScriptGraphEditorBackendUVETest, UndoRedoRestoresSnapshotsAndNewEditClearsRedo) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetUndoCountUVE(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 1U);
    EXPECT_EQ(backend.GetRedoCountUVE(), 1U);
    ASSERT_TRUE(backend.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({3U, "test.source"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetRedoCountUVE(), 0U);
    EXPECT_EQ(backend.RedoUVE().code, ScriptGraphCommandCodeUVE::NoHistory);
}

TEST(ScriptGraphCanvasUVETest, RemoveNodeUndoRestoresExactGraphLayoutSelectionAndLinkOrder) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {10.0F, 20.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({2U, "test.sink"}, {30.0F, 40.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({3U, "test.sink"}, {50.0F, 60.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Exec"}, {3U, "Exec"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetSelectionUVE({1U, 2U, 3U}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE before = canvas.GetSnapshotUVE();

    ASSERT_TRUE(canvas.RemoveNodeUVE(2U).IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().selectedNodeIds, (std::vector<std::uint32_t>{1U, 3U}));
    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE restored = canvas.GetSnapshotUVE();
    EXPECT_EQ(restored.nodes, before.nodes);
    EXPECT_EQ(restored.links, before.links);
    EXPECT_EQ(restored.selectedNodeIds, before.selectedNodeIds);
    ASSERT_TRUE(canvas.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().nodes.size(), 2U);
    EXPECT_EQ(canvas.GetSnapshotUVE().links.size(), 1U);
}

TEST(ScriptGraphCanvasUVETest, ViewChangesAreNotUndoableAndRejectInvalidValues) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    const std::uint64_t initialRevision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.SetViewUVE({{12.0F, -4.0F}, 2.0F}).IsAppliedUVE());
    EXPECT_EQ(canvas.GetUndoCountUVE(), 0U);
    EXPECT_EQ(canvas.GetSnapshotUVE().view.pan, (ScriptGraphCanvasPointUVE{12.0F, -4.0F}));
    EXPECT_EQ(canvas.GetSnapshotUVE().view.zoom, 2.0F);
    EXPECT_GT(canvas.GetSnapshotUVE().revision, initialRevision);
    EXPECT_FALSE(canvas.SetViewUVE({{0.0F, 0.0F}, 0.0F}).IsAppliedUVE());
    EXPECT_FALSE(canvas.SetViewUVE({{std::numeric_limits<float>::quiet_NaN(), 0.0F}, 1.0F}).IsAppliedUVE());
}

TEST(ScriptGraphCanvasUVETest, CommandsRejectStaleRevisionAndInvalidSelectionWithoutMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {0.0F, 0.0F}).IsAppliedUVE());
    const std::uint64_t revision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.MoveNodeUVE(1U, {5.0F, 5.0F}, revision).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE beforeReject = canvas.GetSnapshotUVE();
    EXPECT_EQ(canvas.MoveNodeUVE(1U, {8.0F, 8.0F}, revision).code,
              ScriptGraphCanvasCommandCodeUVE::StaleRevision);
    EXPECT_EQ(canvas.SetSelectionUVE({1U, 1U}).code, ScriptGraphCanvasCommandCodeUVE::Rejected);
    EXPECT_EQ(canvas.GetSnapshotUVE(), beforeReject);
}

TEST(ScriptGraphCanvasUVETest, SnapshotProvidesSortedPaletteAndTypedNodePins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.sink"}, {0.0F, 0.0F}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE snapshot = canvas.GetSnapshotUVE();
    EXPECT_EQ(snapshot.paletteNodeTypeIds, (std::vector<std::string>{"test.sink", "test.source"}));
    ASSERT_EQ(snapshot.nodes.size(), 1U);
    EXPECT_EQ(snapshot.nodes[0].displayName, "Test Sink");
    ASSERT_EQ(snapshot.nodes[0].pins.size(), 2U);
    EXPECT_EQ(snapshot.nodes[0].pins[0].name, "In");
    EXPECT_EQ(snapshot.nodes[0].pins[0].direction, ScriptPinDirectionUVE::Input);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptDebuggerUVETest, ContinueUVE_PausesAtSourceNodeBreakpointAndContinueResumes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 20U, 30U, {}, "Out", "In"});
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    ASSERT_TRUE(debugger.SetBreakpointUVE(20U, true));
    const ScriptDebuggerSnapshotUVE paused = debugger.ContinueUVE();
    EXPECT_EQ(paused.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(paused.instructionIndex, 1U);
    EXPECT_EQ(paused.sourceNodeId, 20U);
    EXPECT_EQ(paused.pauseReason, "Breakpoint reached.");
    const ScriptDebuggerSnapshotUVE completed = debugger.ContinueUVE();
    EXPECT_EQ(completed.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(completed.executedInstructions, 2U);
}

TEST(ScriptDebuggerUVETest, StepUVE_AdvancesOneInstructionAndReportsCompletion) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE first = debugger.StepUVE();
    EXPECT_EQ(first.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(first.instructionIndex, 1U);
    EXPECT_EQ(first.executedInstructions, 1U);
    const ScriptDebuggerSnapshotUVE second = debugger.StepUVE();
    EXPECT_EQ(second.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(second.instructionIndex, 2U);
}

TEST(ScriptDebuggerUVETest, SetBreakpointUVE_ProvidesSortedSnapshotAndRejectsEmptyNodeId) {
    ScriptDebuggerUVE debugger;
    EXPECT_FALSE(debugger.SetBreakpointUVE(0U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(20U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(10U, true));
    const ScriptDebuggerSnapshotUVE snapshot = debugger.GetSnapshotUVE();
    EXPECT_EQ(snapshot.breakpointNodeIds, (std::vector<std::uint32_t>{10U, 20U}));
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptHotReloadManagerUVETest, LoadInitialAndReloadUVE_PublishOnlyValidatedCandidates) {
    ScriptBytecodeProgramUVE initial;
    initial.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    std::vector<std::uint8_t> initialBytes;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    initialBytes = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());

    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE loaded = manager.LoadInitialUVE(initialBytes);
    ASSERT_TRUE(loaded.IsAcceptedUVE());
    EXPECT_EQ(loaded.activeGeneration, 1U);
    EXPECT_TRUE(manager.GetSnapshotUVE().hasActiveProgram);

    std::vector<std::uint8_t> invalid = initialBytes;
    invalid[0U] = static_cast<std::uint8_t>('X');
    const ScriptHotReloadResultUVE rejected = manager.ReloadUVE(invalid);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    EXPECT_EQ(rejected.activeGeneration, 1U);
    EXPECT_EQ(manager.GetActiveProgramUVE()->instructions.size(), 1U);

    initial.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 10U, 20U, {}, "Out", "In"});
    diagnostics.clear();
    const std::vector<std::uint8_t> replacement = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptHotReloadResultUVE accepted = manager.ReloadUVE(replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_TRUE(accepted.compatibleStatePreserved);
    EXPECT_EQ(manager.GetSnapshotUVE().instructionCount, 2U);
}

TEST(ScriptHotReloadManagerUVETest, ReloadUVE_ReportsNoActiveProgramWhenInitialCandidateIsInvalid) {
    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE result = manager.ReloadUVE({0x00U, 0x01U});
    EXPECT_EQ(result.code, ScriptHotReloadCodeUVE::NoActiveProgram);
    EXPECT_FALSE(result.lastKnownGoodRetained);
    EXPECT_FALSE(manager.GetSnapshotUVE().hasActiveProgram);
}

} // namespace UVE::Scripting
