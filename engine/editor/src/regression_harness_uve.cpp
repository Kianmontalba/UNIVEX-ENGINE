// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/regression_harness_uve.h"

#include "uve/scripting/script_compiler_ir_uve.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace UVE::Editor {
namespace {

[[nodiscard]] EditorRegressionRunResultUVE MakeRunErrorUVE(EditorRegressionCodeUVE code,
                                                           const char* message) {
    return EditorRegressionRunResultUVE{code, message, {}, 0U, 0U, {}};
}

[[nodiscard]] std::string BuildDigestUVE(
    const std::string& encodedSchema, const Scripting::ScriptIrProgramUVE& program,
    const std::vector<Core::TransformPoseUVE>& sampledPoses,
    const EditorBridgeVisualScriptingSnapshotUVE& visualScriptDto) {
    std::ostringstream stream;
    stream << std::setprecision(9);
    stream << "schema=" << encodedSchema << "|ir-version=" << program.version
           << "|instructions=" << program.instructions.size() << "|sources=";
    for (const std::uint32_t nodeId : program.sourceNodeIds) {
        stream << nodeId << ',';
    }
    stream << "|samples=";
    for (const Core::TransformPoseUVE& pose : sampledPoses) {
        stream << pose.position.x << ',' << pose.position.y << ',' << pose.position.z << ';'
               << pose.rotation.x << ',' << pose.rotation.y << ',' << pose.rotation.z << ','
               << pose.rotation.w << ';' << pose.scale.x << ',' << pose.scale.y << ','
               << pose.scale.z << '|';
    }
    stream << "|dto=" << visualScriptDto.graphRevision << ':' << visualScriptDto.nodeCount << ':'
           << visualScriptDto.linkCount << ':' << visualScriptDto.canvas.revision << ':'
           << visualScriptDto.canvas.dirty;
    return stream.str();
}

[[nodiscard]] bool IsFiniteSampleTimeUVE(double timeSeconds) noexcept {
    return std::isfinite(timeSeconds);
}

} // namespace

EditorRegressionRunResultUVE RunEditorAnimationRegressionUVE(
    const EditorAnimationRegressionCaseUVE& regressionCase,
    Scripting::ScriptNodeRegistryUVE& registry) {
    if (regressionCase.operations.size() >
            EditorAnimationRegressionCaseUVE::kMaximumOperationsUVE ||
        regressionCase.animationSampleTimes.size() >
            EditorAnimationRegressionCaseUVE::kMaximumSampleTimesUVE) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::CapacityExceeded,
                               "regression case exceeds its bounded replay capacity");
    }
    if (!Core::ValidateAnimationClipUVE(regressionCase.animationClip).IsValidUVE()) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::InvalidCase,
                               "regression case contains an invalid animation clip");
    }

    std::vector<Scripting::ScriptPersistenceDiagnosticUVE> encodeDiagnostics;
    const std::string initialSchemaText = Scripting::EncodeScriptGraphSchemaUVE(
        regressionCase.graphSchema, encodeDiagnostics);
    if (!encodeDiagnostics.empty()) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::PersistenceFailed,
                               "initial graph schema encoding failed");
    }
    const Scripting::ScriptGraphSchemaDecodeResultUVE initialDecode =
        Scripting::DecodeScriptGraphSchemaUVE(initialSchemaText);
    if (!initialDecode.IsSuccessUVE() || initialDecode.schema.value() != regressionCase.graphSchema) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::PersistenceFailed,
                               "initial graph schema round-trip failed");
    }

    Scripting::ScriptGraphCanvasUVE canvas(registry);
    const Scripting::ScriptGraphCanvasCommandResultUVE schemaResult =
        canvas.ApplyGraphSchemaUVE(*initialDecode.schema);
    if (!schemaResult.IsAppliedUVE()) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::GraphEditRejected,
                               "initial graph schema was rejected by the canvas");
    }

    std::size_t appliedOperationCount = 0U;
    for (const EditorRegressionOperationUVE& operation : regressionCase.operations) {
        Scripting::ScriptGraphCanvasCommandResultUVE commandResult;
        switch (operation.kind) {
        case EditorRegressionOperationKindUVE::AddNode:
            commandResult = canvas.AddNodeUVE(
                Scripting::ScriptNodeUVE{operation.nodeId, operation.nodeTypeId}, operation.position);
            break;
        case EditorRegressionOperationKindUVE::MoveNode:
            commandResult = canvas.MoveNodeUVE(operation.nodeId, operation.position);
            break;
        case EditorRegressionOperationKindUVE::SetSelection:
            commandResult = canvas.SetSelectionUVE(operation.selection);
            break;
        case EditorRegressionOperationKindUVE::SetView:
            commandResult = canvas.SetViewUVE(operation.view);
            break;
        case EditorRegressionOperationKindUVE::Undo:
            commandResult = canvas.UndoUVE();
            break;
        case EditorRegressionOperationKindUVE::Redo:
            commandResult = canvas.RedoUVE();
            break;
        }
        if (!commandResult.IsAppliedUVE()) {
            return MakeRunErrorUVE(EditorRegressionCodeUVE::GraphEditRejected,
                                   "replay operation was rejected by the canvas");
        }
        ++appliedOperationCount;
    }

    const Scripting::ScriptGraphCanvasSnapshotUVE canvasSnapshot = canvas.GetSnapshotUVE();
    const Scripting::ScriptGraphCanvasLayoutSnapshotUVE layoutSnapshot = canvas.GetLayoutSnapshotUVE();
    Scripting::ScriptGraphSchemaUVE finalSchema;
    finalSchema.schemaVersion = Scripting::kScriptGraphSchemaVersionUVE;
    finalSchema.graph = canvas.GetGraphUVE();
    finalSchema.layout.clear();
    finalSchema.layout.reserve(layoutSnapshot.entries.size());
    for (const Scripting::ScriptGraphCanvasLayoutEntryUVE& layoutEntry : layoutSnapshot.entries) {
        finalSchema.layout.push_back(
            Scripting::ScriptGraphLayoutEntryUVE{layoutEntry.nodeId, layoutEntry.position.x,
                                                  layoutEntry.position.y});
    }

    encodeDiagnostics.clear();
    const std::string finalSchemaText =
        Scripting::EncodeScriptGraphSchemaUVE(finalSchema, encodeDiagnostics);
    if (!encodeDiagnostics.empty()) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::PersistenceFailed,
                               "replayed graph schema encoding failed");
    }
    const Scripting::ScriptGraphSchemaDecodeResultUVE finalDecode =
        Scripting::DecodeScriptGraphSchemaUVE(finalSchemaText);
    if (!finalDecode.IsSuccessUVE() || finalDecode.schema.value() != finalSchema) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::PersistenceFailed,
                               "replayed graph schema round-trip failed");
    }

    const Scripting::ScriptIrCompileResultUVE compileResult =
        Scripting::CompileScriptGraphToIrUVE(finalDecode.schema->graph, registry);
    if (!compileResult.IsSuccessUVE()) {
        return MakeRunErrorUVE(EditorRegressionCodeUVE::CompileFailed,
                               "replayed graph compilation failed");
    }

    std::vector<Core::TransformPoseUVE> sampledPoses;
    sampledPoses.reserve(regressionCase.animationSampleTimes.size());
    for (const double sampleTimeSeconds : regressionCase.animationSampleTimes) {
        if (!IsFiniteSampleTimeUVE(sampleTimeSeconds)) {
            return MakeRunErrorUVE(EditorRegressionCodeUVE::AnimationFailed,
                                   "animation replay contains a non-finite sample time");
        }
        Core::TransformPoseUVE pose;
        if (!Core::TrySampleAnimationClipUVE(regressionCase.animationClip, sampleTimeSeconds, true,
                                             pose)) {
            return MakeRunErrorUVE(EditorRegressionCodeUVE::AnimationFailed,
                                   "animation replay sample failed");
        }
        sampledPoses.push_back(pose);
    }

    EditorBridgeVisualScriptingSnapshotUVE visualScriptDto;
    visualScriptDto.available = true;
    visualScriptDto.graphRevision = canvasSnapshot.graphRevision;
    visualScriptDto.nodeCount = canvasSnapshot.nodes.size();
    visualScriptDto.linkCount = canvasSnapshot.links.size();
    visualScriptDto.canEdit = true;
    visualScriptDto.canvas = canvasSnapshot;
    visualScriptDto.reason.clear();

    EditorRegressionRunResultUVE result;
    result.code = EditorRegressionCodeUVE::Passed;
    result.message = "regression passed";
    result.deterministicDigest = BuildDigestUVE(finalSchemaText, *compileResult.program,
                                                sampledPoses, visualScriptDto);
    result.appliedOperationCount = appliedOperationCount;
    result.sampledAnimationCount = sampledPoses.size();
    result.visualScriptDto = std::move(visualScriptDto);
    return result;
}

EditorRegressionDeterminismResultUVE RunEditorAnimationDeterminismUVE(
    const EditorAnimationRegressionCaseUVE& regressionCase,
    Scripting::ScriptNodeRegistryUVE& registry, std::size_t iterations) {
    if (iterations == 0U) {
        return EditorRegressionDeterminismResultUVE{
            EditorRegressionCodeUVE::InvalidCase, "determinism iteration count must be non-zero", {},
            iterations, 0U};
    }

    std::string expectedDigest;
    for (std::size_t iteration = 0U; iteration < iterations; ++iteration) {
        const EditorRegressionRunResultUVE runResult =
            RunEditorAnimationRegressionUVE(regressionCase, registry);
        if (!runResult.IsPassedUVE()) {
            return EditorRegressionDeterminismResultUVE{runResult.code, runResult.message, {},
                                                       iterations, iteration};
        }
        if (iteration == 0U) {
            expectedDigest = runResult.deterministicDigest;
        } else if (runResult.deterministicDigest != expectedDigest) {
            return EditorRegressionDeterminismResultUVE{
                EditorRegressionCodeUVE::NonDeterministic,
                "repeated regression runs produced different deterministic digests", {}, iterations,
                iteration};
        }
    }

    return EditorRegressionDeterminismResultUVE{EditorRegressionCodeUVE::Passed,
                                                "determinism passed", expectedDigest, iterations,
                                                iterations};
}

} // namespace UVE::Editor
