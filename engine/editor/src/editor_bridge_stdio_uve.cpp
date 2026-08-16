// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_bridge_stdio_uve.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Editor {
namespace {

using JsonUVE = nlohmann::json;

enum class FrameReadResultUVE : std::uint8_t {
    Body,
    EndOfFile,
    TruncatedHeader,
    TruncatedBody,
    ZeroLength,
    Oversized,
};

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeEntityRefUVE entity) {
    return JsonUVE{{"index", entity.index}, {"generation", entity.generation}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeEntitySnapshotUVE& entity) {
    return JsonUVE{{"entity", ToJsonUVE(entity.entity)}, {"displayLabel", entity.displayLabel}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeHierarchyEntryUVE& entry) {
    return JsonUVE{{"entity", ToJsonUVE(entry.entity)},
                   {"parent", entry.parent.has_value() ? ToJsonUVE(*entry.parent) : JsonUVE(nullptr)},
                   {"displayLabel", entry.displayLabel},
                   {"typeTag", entry.typeTag},
                   {"depth", entry.depth},
                   {"childCount", entry.childCount},
                   {"selected", entry.selected},
                   {"active", entry.active}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeHierarchySnapshotUVE& snapshot) {
    JsonUVE entries = JsonUVE::array();
    for (const EditorBridgeHierarchyEntryUVE& entry : snapshot.entries) {
        entries.push_back(ToJsonUVE(entry));
    }
    return JsonUVE{{"filter", snapshot.filter},
                   {"filterActive", snapshot.filterActive},
                   {"truncated", snapshot.truncated},
                   {"entries", std::move(entries)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeInspectorSnapshotUVE& snapshot) {
    JsonUVE selectedEntities = JsonUVE::array();
    for (const EditorBridgeEntitySnapshotUVE& entity : snapshot.selectedEntities) {
        selectedEntities.push_back(ToJsonUVE(entity));
    }
    JsonUVE ancestry = JsonUVE::array();
    for (const EditorBridgeEntitySnapshotUVE& entity : snapshot.ancestry) {
        ancestry.push_back(ToJsonUVE(entity));
    }
    JsonUVE drawerIds = JsonUVE::array();
    for (const std::string& identifier : snapshot.eligibleDrawerIds) {
        drawerIds.push_back(identifier);
    }
    return JsonUVE{{"mode", static_cast<std::uint8_t>(snapshot.mode)},
                   {"selectedEntitiesTruncated", snapshot.selectedEntitiesTruncated},
                   {"selectedEntities", std::move(selectedEntities)},
                   {"activeEntity", snapshot.activeEntity.has_value() ? ToJsonUVE(*snapshot.activeEntity)
                                                                         : JsonUVE(nullptr)},
                   {"parent", snapshot.parent.has_value() ? ToJsonUVE(*snapshot.parent) : JsonUVE(nullptr)},
                   {"ancestry", std::move(ancestry)},
                   {"eligibleDrawerIds", std::move(drawerIds)},
                   {"canEditSelectedName", snapshot.canEditSelectedName}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeContentBrowserEntryUVE& entry) {
    return JsonUVE{{"relativePath", entry.relativePath},
                   {"isDirectory", entry.isDirectory},
                   {"typeLabel", entry.typeLabel},
                   {"registeredAssetGuid", entry.registeredAssetGuid.has_value()
                                               ? JsonUVE(*entry.registeredAssetGuid)
                                               : JsonUVE(nullptr)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeContentBrowserSnapshotUVE& snapshot) {
    JsonUVE entries = JsonUVE::array();
    for (const EditorBridgeContentBrowserEntryUVE& entry : snapshot.entries) {
        entries.push_back(ToJsonUVE(entry));
    }
    JsonUVE breadcrumbs = JsonUVE::array();
    for (const std::string& breadcrumb : snapshot.breadcrumbs) {
        breadcrumbs.push_back(breadcrumb);
    }
    return JsonUVE{{"contentRoot", snapshot.contentRoot},
                   {"currentDirectory", snapshot.currentDirectory},
                   {"filter", snapshot.filter},
                   {"typeFocus", snapshot.typeFocus},
                   {"breadcrumbs", std::move(breadcrumbs)},
                   {"refreshGeneration", snapshot.refreshGeneration},
                   {"visibleEntryCount", snapshot.visibleEntryCount},
                   {"directEntryCount", snapshot.directEntryCount},
                   {"contentRootExists", snapshot.contentRootExists},
                   {"initialized", snapshot.initialized},
                   {"lastRefreshSucceeded", snapshot.lastRefreshSucceeded},
                   {"truncated", snapshot.truncated},
                   {"entries", std::move(entries)},
                   {"selectedEntry", snapshot.selectedEntry.has_value() ? ToJsonUVE(*snapshot.selectedEntry)
                                                                           : JsonUVE(nullptr)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const DeveloperConsoleEntryUVE& entry) {
    return JsonUVE{{"severity", static_cast<std::uint8_t>(entry.severity)}, {"text", entry.text}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const DeveloperConsoleCVarUVE& cvar) {
    return JsonUVE{{"name", cvar.name}, {"value", cvar.value}, {"readOnly", cvar.readOnly}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const DeveloperConsoleCompletionUVE& completion) {
    return JsonUVE{{"identifier", completion.identifier}, {"help", completion.help}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDeveloperConsoleSnapshotUVE& snapshot) {
    JsonUVE output = JsonUVE::array();
    for (const DeveloperConsoleEntryUVE& entry : snapshot.console.output) {
        output.push_back(ToJsonUVE(entry));
    }
    JsonUVE history = JsonUVE::array();
    for (const std::string& command : snapshot.console.history) {
        history.push_back(command);
    }
    JsonUVE cvars = JsonUVE::array();
    for (const DeveloperConsoleCVarUVE& cvar : snapshot.console.cvars) {
        cvars.push_back(ToJsonUVE(cvar));
    }
    JsonUVE completions = JsonUVE::array();
    for (const DeveloperConsoleCompletionUVE& completion : snapshot.console.completions) {
        completions.push_back(ToJsonUVE(completion));
    }
    return JsonUVE{{"generation", snapshot.console.generation},
                   {"available", snapshot.console.available},
                   {"developmentOnly", snapshot.console.developmentOnly},
                   {"severityFilter", static_cast<std::uint8_t>(snapshot.console.severityFilter)},
                   {"historyCursor", snapshot.console.historyCursor},
                   {"historyEntry", snapshot.console.historyEntry},
                   {"outputTruncated", snapshot.console.outputTruncated},
                   {"historyTruncated", snapshot.console.historyTruncated},
                   {"cvarsTruncated", snapshot.console.cvarsTruncated},
                   {"completionTruncated", snapshot.console.completionTruncated},
                   {"output", std::move(output)}, {"history", std::move(history)},
                   {"cvars", std::move(cvars)}, {"completions", std::move(completions)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDataTableCatalogEntryUVE& entry) {
    return JsonUVE{{"name", entry.name},
                   {"generation", entry.generation},
                   {"columnCount", entry.columnCount},
                   {"rowCount", entry.rowCount},
                   {"valid", entry.valid}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDataTableCatalogSnapshotUVE& snapshot) {
    JsonUVE entries = JsonUVE::array();
    for (const EditorBridgeDataTableCatalogEntryUVE& entry : snapshot.entries) {
        entries.push_back(ToJsonUVE(entry));
    }
    return JsonUVE{{"generation", snapshot.generation},
                   {"entriesTruncated", snapshot.entriesTruncated},
                   {"entries", std::move(entries)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDataTablePreviewColumnUVE& column) {
    return JsonUVE{{"name", column.name}, {"type", static_cast<std::uint8_t>(column.type)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDataTablePreviewRowUVE& row) {
    JsonUVE values = JsonUVE::array();
    for (const std::string& value : row.values) {
        values.push_back(value);
    }
    return JsonUVE{{"identifier", row.identifier}, {"values", std::move(values)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeDataTablePreviewSnapshotUVE& snapshot) {
    JsonUVE columns = JsonUVE::array();
    for (const EditorBridgeDataTablePreviewColumnUVE& column : snapshot.columns) {
        columns.push_back(ToJsonUVE(column));
    }
    JsonUVE rows = JsonUVE::array();
    for (const EditorBridgeDataTablePreviewRowUVE& row : snapshot.rows) {
        rows.push_back(ToJsonUVE(row));
    }
    return JsonUVE{{"available", snapshot.available},
                   {"generation", snapshot.generation},
                   {"name", snapshot.name},
                   {"totalColumnCount", snapshot.totalColumnCount},
                   {"totalRowCount", snapshot.totalRowCount},
                   {"columnsTruncated", snapshot.columnsTruncated},
                   {"rowsTruncated", snapshot.rowsTruncated},
                   {"valuesTruncated", snapshot.valuesTruncated},
                   {"reason", snapshot.reason},
                   {"columns", std::move(columns)},
                   {"rows", std::move(rows)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeViewportSurfaceSnapshotUVE& surface) {
    return JsonUVE{{"state", static_cast<std::uint8_t>(surface.state)},
                   {"generation", surface.generation},
                   {"width", surface.width},
                   {"height", surface.height},
                   {"nativeRendererOwnsSurface", surface.nativeRendererOwnsSurface},
                   {"managedAttachAllowed", surface.managedAttachAllowed},
                   {"reason", surface.reason}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasPinSnapshotUVE& pin) {
    return JsonUVE{{"name", pin.name}, {"direction", static_cast<std::uint8_t>(pin.direction)},
                   {"type", static_cast<std::uint8_t>(pin.type)},
                   {"role", static_cast<std::uint8_t>(pin.role)},
                   {"defaultValue", pin.defaultValue.has_value() ? JsonUVE(*pin.defaultValue) : JsonUVE(nullptr)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) {
    JsonUVE pins = JsonUVE::array();
    for (const auto& pin : node.pins) {
        pins.push_back(ToJsonUVE(pin));
    }
    return JsonUVE{{"id", node.id}, {"typeId", node.typeId}, {"displayName", node.displayName},
                   {"category", node.category}, {"iconId", node.iconId},
                   {"displayOrder", node.displayOrder}, {"presentationFlags", node.presentationFlags},
                   {"x", node.position.x}, {"y", node.position.y}, {"selected", node.selected},
                   {"pins", std::move(pins)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasLinkSnapshotUVE& link) {
    return JsonUVE{{"output", {{"nodeId", link.link.output.nodeId}, {"pinName", link.link.output.pinName}}},
                   {"input", {{"nodeId", link.link.input.nodeId}, {"pinName", link.link.input.pinName}}}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasPaletteEntryUVE& entry) {
    JsonUVE pins = JsonUVE::array();
    for (const auto& pin : entry.pins) {
        pins.push_back(ToJsonUVE(pin));
    }
    return JsonUVE{{"typeId", entry.typeId}, {"displayName", entry.displayName},
                   {"category", entry.category}, {"iconId", entry.iconId},
                   {"displayOrder", entry.displayOrder}, {"presentationFlags", entry.presentationFlags},
                   {"pins", std::move(pins)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptValidationDiagnosticUVE& diagnostic) {
    JsonUVE result{{"code", static_cast<std::uint8_t>(diagnostic.code)},
                   {"severity", static_cast<std::uint8_t>(diagnostic.severity)},
                   {"nodeId", diagnostic.nodeId}, {"pinName", diagnostic.pinName},
                   {"message", diagnostic.message}, {"sourceContext", diagnostic.sourceContext}};
    result["relatedEndpoint"] = diagnostic.relatedEndpoint.has_value()
        ? JsonUVE{{"nodeId", diagnostic.relatedEndpoint->nodeId}, {"pinName", diagnostic.relatedEndpoint->pinName}}
        : JsonUVE{};
    return result;
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphSchemaUVE& schema) {
    JsonUVE nodes = JsonUVE::array();
    std::vector<Scripting::ScriptNodeUVE> sortedNodes = schema.graph.GetNodesUVE();
    std::sort(sortedNodes.begin(), sortedNodes.end(), [](const auto& left, const auto& right) {
        return left.id < right.id;
    });
    for (const Scripting::ScriptNodeUVE& node : sortedNodes) {
        nodes.push_back({{"id", node.id}, {"typeId", node.typeId}});
    }
    JsonUVE links = JsonUVE::array();
    std::vector<Scripting::ScriptLinkUVE> sortedLinks = schema.graph.GetLinksUVE();
    std::sort(sortedLinks.begin(), sortedLinks.end(), [](const auto& left, const auto& right) {
        if (left.output.nodeId != right.output.nodeId) return left.output.nodeId < right.output.nodeId;
        if (left.output.pinName != right.output.pinName) return left.output.pinName < right.output.pinName;
        if (left.input.nodeId != right.input.nodeId) return left.input.nodeId < right.input.nodeId;
        return left.input.pinName < right.input.pinName;
    });
    for (const Scripting::ScriptLinkUVE& link : sortedLinks) {
        links.push_back({{"output", {{"nodeId", link.output.nodeId}, {"pinName", link.output.pinName}}},
                         {"input", {{"nodeId", link.input.nodeId}, {"pinName", link.input.pinName}}}});
    }
    JsonUVE layout = JsonUVE::array();
    std::vector<Scripting::ScriptGraphLayoutEntryUVE> sortedLayout = schema.layout;
    std::sort(sortedLayout.begin(), sortedLayout.end(), [](const auto& left, const auto& right) {
        return left.nodeId < right.nodeId;
    });
    for (const Scripting::ScriptGraphLayoutEntryUVE& entry : sortedLayout) {
        layout.push_back({{"nodeId", entry.nodeId}, {"x", entry.x}, {"y", entry.y}});
    }
    JsonUVE metadata = JsonUVE::object();
    for (const auto& [key, value] : schema.metadata) metadata[key] = value;
    return JsonUVE{{"schemaVersion", schema.schemaVersion}, {"nodes", std::move(nodes)},
                   {"links", std::move(links)}, {"layout", std::move(layout)}, {"metadata", std::move(metadata)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasSnapshotUVE& canvas) {
    JsonUVE nodes = JsonUVE::array();
    for (const auto& node : canvas.nodes) {
        nodes.push_back(ToJsonUVE(node));
    }
    JsonUVE links = JsonUVE::array();
    for (const auto& link : canvas.links) {
        links.push_back(ToJsonUVE(link));
    }
    JsonUVE selection = JsonUVE::array();
    for (const std::uint32_t nodeId : canvas.selectedNodeIds) {
        selection.push_back(nodeId);
    }
    JsonUVE palette = JsonUVE::array();
    for (const std::string& typeId : canvas.paletteNodeTypeIds) {
        palette.push_back(typeId);
    }
    JsonUVE paletteDescriptors = JsonUVE::array();
    for (const auto& descriptor : canvas.paletteDescriptors) {
        paletteDescriptors.push_back(ToJsonUVE(descriptor));
    }
    JsonUVE diagnostics = JsonUVE::array();
    for (const auto& diagnostic : canvas.diagnostics) {
        diagnostics.push_back(ToJsonUVE(diagnostic));
    }
    return JsonUVE{{"revision", canvas.revision}, {"graphRevision", canvas.graphRevision},
                   {"pan", {{"x", canvas.view.pan.x}, {"y", canvas.view.pan.y}}},
                   {"zoom", canvas.view.zoom}, {"nodesTruncated", canvas.nodesTruncated},
                   {"linksTruncated", canvas.linksTruncated}, {"paletteTruncated", canvas.paletteTruncated},
                   {"diagnosticsTruncated", canvas.diagnosticsTruncated}, {"dirty", canvas.dirty},
                   {"canUndo", canvas.canUndo}, {"canRedo", canvas.canRedo}, {"nodes", std::move(nodes)},
                   {"links", std::move(links)}, {"selectedNodeIds", std::move(selection)},
                   {"paletteNodeTypeIds", std::move(palette)}, {"paletteDescriptors", std::move(paletteDescriptors)},
                   {"diagnostics", std::move(diagnostics)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeVisualScriptDebuggerSnapshotUVE& debugger) {
    JsonUVE breakpoints = JsonUVE::array();
    for (const std::uint32_t nodeId : debugger.breakpointNodeIds) {
        breakpoints.push_back(nodeId);
    }
    return JsonUVE{{"available", debugger.available},
                   {"state", static_cast<std::uint8_t>(debugger.state)},
                   {"instructionIndex", debugger.instructionIndex},
                   {"sourceNodeId", debugger.sourceNodeId},
                   {"executedInstructions", debugger.executedInstructions},
                   {"pauseReason", debugger.pauseReason},
                   {"breakpointNodeIds", std::move(breakpoints)},
                   {"reason", debugger.reason}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeVisualScriptingSnapshotUVE& scripting) {
    return JsonUVE{{"available", scripting.available},
                   {"graphRevision", scripting.graphRevision},
                   {"nodeCount", scripting.nodeCount},
                   {"linkCount", scripting.linkCount},
                   {"canEdit", scripting.canEdit},
                   {"reason", scripting.reason},
                   {"canvas", ToJsonUVE(scripting.canvas)},
                   {"debugger", ToJsonUVE(scripting.debugger)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeScriptRuntimeInstanceEntryUVE& entry) {
    return JsonUVE{{"entityIndex", entry.entityIndex},
                   {"entityGeneration", entry.entityGeneration},
                   {"generation", entry.generation},
                   {"programVersion", entry.programVersion},
                   {"instructionCount", entry.instructionCount},
                   {"stateValueCount", entry.stateValueCount},
                   {"enabled", entry.enabled}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeScriptRuntimeSnapshotUVE& snapshot) {
    JsonUVE entries = JsonUVE::array();
    for (const EditorBridgeScriptRuntimeInstanceEntryUVE& entry : snapshot.entries) {
        entries.push_back(ToJsonUVE(entry));
    }
    return JsonUVE{{"available", snapshot.available},
                   {"instanceCount", snapshot.instanceCount},
                   {"entriesTruncated", snapshot.entriesTruncated},
                   {"reason", snapshot.reason},
                   {"entries", std::move(entries)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeScriptRuntimeTickSummaryUVE& summary) {
    return JsonUVE{{"available", summary.available},
                   {"reason", summary.reason},
                   {"enabledInstanceCount", summary.enabledInstanceCount},
                   {"completedCount", summary.completedCount},
                   {"instructionBudgetExceededCount", summary.instructionBudgetExceededCount},
                   {"invalidInstructionCount", summary.invalidInstructionCount},
                   {"diagnosticCount", summary.diagnosticCount}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeScriptRuntimeTickHistoryEntryUVE& entry) {
    return JsonUVE{{"sequence", entry.sequence}, {"summary", ToJsonUVE(entry.summary)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Asset::ResourceHandleUVE handle) {
    return JsonUVE{{"guid", handle.guid.value}, {"generation", handle.generation}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Plugins::Editor::MotionQueryEditorDatabaseRowUVE& row) {
    return JsonUVE{{"resource", ToJsonUVE(row.resource)},
                   {"displayName", row.displayName},
                   {"databaseId", row.databaseId},
                   {"generation", row.generation},
                   {"schemaVersion", row.schemaVersion},
                   {"schemaId", row.schemaId},
                   {"candidateCount", row.candidateCount},
                   {"maximumCandidates", row.maximumCandidates},
                   {"valid", row.valid},
                   {"selected", row.selected},
                   {"dirty", row.dirty}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryAuthoringSnapshotUVE& snapshot) {
    JsonUVE databases = JsonUVE::array();
    for (const auto& row : snapshot.databases) {
        databases.push_back(ToJsonUVE(row));
    }
    return JsonUVE{{"revision", snapshot.revision},
                   {"selectedResource", snapshot.selectedResource.has_value()
                                            ? ToJsonUVE(*snapshot.selectedResource)
                                            : JsonUVE(nullptr)},
                   {"databases", std::move(databases)},
                   {"diagnostic", snapshot.diagnostic}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryDebuggerSnapshotUVE& snapshot) {
    return JsonUVE{{"attached", snapshot.attached},
                   {"generation", snapshot.generation},
                   {"database", snapshot.database.has_value() ? ToJsonUVE(*snapshot.database)
                                                                  : JsonUVE(nullptr)},
                   {"selectedCandidateIndex", snapshot.selectedCandidateIndex.has_value()
                                                    ? JsonUVE(*snapshot.selectedCandidateIndex)
                                                    : JsonUVE(nullptr)},
                   {"candidateCount", snapshot.candidateCount},
                   {"candidatesEvaluated", snapshot.candidatesEvaluated},
                   {"selectedCost", snapshot.selectedCost},
                   {"selectedCandidateId", snapshot.selectedCandidateId},
                   {"selectedSourceClipId", snapshot.selectedSourceClipId},
                   {"qualityTier", snapshot.qualityTier},
                   {"continuityCode", snapshot.continuityCode},
                   {"continuityApplied", snapshot.continuityApplied},
                   {"transitionCode", snapshot.transitionCode},
                   {"transitionHeldPrevious", snapshot.transitionHeldPrevious},
                   {"telemetryCode", snapshot.telemetryCode},
                   {"telemetryIndexEntryCount", snapshot.telemetryIndexEntryCount},
                   {"telemetryCandidatesConsidered", snapshot.telemetryCandidatesConsidered},
                   {"telemetryBudgetSaturated", snapshot.telemetryBudgetSaturated},
                   {"provenance", snapshot.provenance},
                   {"message", snapshot.message}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Plugins::Editor::MotionQueryTraceEventUVE& event) {
    return JsonUVE{{"sequence", event.sequence},
                   {"timestampNanoseconds", event.timestampNanoseconds},
                   {"frameNumber", event.frameNumber},
                   {"kind", event.kind},
                   {"database", event.database.has_value() ? ToJsonUVE(*event.database) : JsonUVE(nullptr)},
                   {"candidatesConsidered", event.candidatesConsidered},
                   {"candidatesEvaluated", event.candidatesEvaluated},
                   {"cost", event.cost},
                   {"selectedCandidateIndex", event.selectedCandidateIndex.has_value()
                                                    ? JsonUVE(*event.selectedCandidateIndex)
                                                    : JsonUVE(nullptr)},
                   {"qualityTier", event.qualityTier},
                   {"continuityCode", event.continuityCode},
                   {"continuityApplied", event.continuityApplied},
                   {"transitionCode", event.transitionCode},
                   {"transitionHeldPrevious", event.transitionHeldPrevious},
                   {"telemetryCode", event.telemetryCode},
                   {"telemetryIndexEntryCount", event.telemetryIndexEntryCount},
                   {"telemetryCandidatesConsidered", event.telemetryCandidatesConsidered},
                   {"telemetryBudgetSaturated", event.telemetryBudgetSaturated},
                   {"provenance", event.provenance},
                   {"message", event.message}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryTraceSnapshotUVE& snapshot) {
    JsonUVE events = JsonUVE::array();
    for (const auto& event : snapshot.events) {
        events.push_back(ToJsonUVE(event));
    }
    return JsonUVE{{"generation", snapshot.generation},
                   {"truncated", snapshot.truncated},
                   {"events", std::move(events)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayBaselineEntryUVE& entry) {
    return JsonUVE{{"name", entry.name},
                   {"sourceGeneration", entry.sourceGeneration},
                   {"eventCount", entry.eventCount},
                   {"truncated", entry.truncated}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayBaselineSnapshotUVE& snapshot) {
    JsonUVE entries = JsonUVE::array();
    for (const EditorBridgeMotionQueryReplayBaselineEntryUVE& entry : snapshot.entries) {
        entries.push_back(ToJsonUVE(entry));
    }
    return JsonUVE{{"generation", snapshot.generation},
                   {"truncated", snapshot.truncated},
                   {"entries", std::move(entries)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayBatchEntryUVE& entry) {
    return JsonUVE{{"baselineName", entry.baselineName},
                   {"regressionCode", entry.regressionCode},
                   {"comparisonCode", entry.comparisonCode},
                   {"comparedEventCount", entry.comparedEventCount},
                   {"mismatchIndex", entry.mismatchIndex},
                   {"mismatchFieldMask", entry.mismatchFieldMask},
                   {"diagnosticSummary", entry.diagnosticSummary},
                   {"compatibilityMismatchMask", entry.compatibilityMismatchMask},
                   {"compatibilityDiagnosticSummary", entry.compatibilityDiagnosticSummary}};
}
[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayBatchSnapshotUVE& batch) {
    JsonUVE results = JsonUVE::array();
    for (const EditorBridgeMotionQueryReplayBatchEntryUVE& entry : batch.results) {
        results.push_back(ToJsonUVE(entry));
    }
    return JsonUVE{{"available", batch.available},
                   {"code", batch.code},
                   {"registryGeneration", batch.registryGeneration},
                   {"evaluatedBaselineCount", batch.evaluatedBaselineCount},
                   {"matchCount", batch.matchCount},
                   {"mismatchCount", batch.mismatchCount},
                   {"truncated", batch.truncated},
                   {"message", batch.message},
                   {"results", std::move(results)}};
}
[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayWorkflowStatusUVE& status) {
    return JsonUVE{{"registryGeneration", status.registryGeneration},
                   {"baselineCount", status.baselineCount},
                   {"activeBaselineSelected", status.activeBaselineSelected},
                   {"activeFixtureAvailable", status.activeFixtureAvailable},
                   {"historyTruncated", status.historyTruncated},
                   {"readyForComparison", status.readyForComparison},
                   {"diagnostic", status.diagnostic}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayBatchHistoryEntryUVE& entry) {
    return JsonUVE{{"sequence", entry.sequence},
                   {"registryGeneration", entry.registryGeneration},
                   {"code", entry.code},
                   {"evaluatedBaselineCount", entry.evaluatedBaselineCount},
                   {"matchCount", entry.matchCount},
                   {"mismatchCount", entry.mismatchCount},
                   {"message", entry.message}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplaySessionFactsUVE& facts) {
    return JsonUVE{{"totalIndividualComparisons", facts.totalIndividualComparisons},
                   {"totalBatchRuns", facts.totalBatchRuns},
                   {"totalBaselinesEvaluated", facts.totalBaselinesEvaluated},
                   {"totalMatchesFound", facts.totalMatchesFound},
                   {"totalMismatchesFound", facts.totalMismatchesFound}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayComparisonHistoryEntryUVE& entry) {
    return JsonUVE{{"sequence", entry.sequence},
                   {"baselineName", entry.baselineName},
                   {"registryGeneration", entry.registryGeneration},
                   {"comparisonCode", entry.comparisonCode},
                   {"comparedEventCount", entry.comparedEventCount},
                   {"mismatchIndex", entry.mismatchIndex},
                   {"mismatchFieldMask", entry.mismatchFieldMask},
                   {"diagnosticSummary", entry.diagnosticSummary}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQueryReplayComparisonUVE& comparison) {
    return JsonUVE{{"available", comparison.available},
                   {"code", comparison.code},
                   {"comparisonCode", comparison.comparisonCode},
                   {"comparedEventCount", comparison.comparedEventCount},
                   {"mismatchIndex", comparison.mismatchIndex},
                   {"fixtureTruncated", comparison.fixtureTruncated},
                   {"snapshotTruncated", comparison.snapshotTruncated},
                   {"mismatchFieldMask", comparison.mismatchFieldMask},
                   {"message", comparison.message},
                   {"diagnosticSummary", comparison.diagnosticSummary},
                   {"compatibilityMismatchMask", comparison.compatibilityMismatchMask},
                   {"compatibilityDiagnosticSummary", comparison.compatibilityDiagnosticSummary}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeMotionQuerySnapshotUVE& snapshot) {
    return JsonUVE{{"authoring", ToJsonUVE(snapshot.authoring)},
                   {"debugger", ToJsonUVE(snapshot.debugger)},
                   {"trace", ToJsonUVE(snapshot.trace)},
                   {"liveDebugActive", snapshot.liveDebugActive},
                   {"liveDebugGeneration", snapshot.liveDebugGeneration},
                   {"liveDebugDatabase", snapshot.liveDebugDatabase.has_value()
                                                ? ToJsonUVE(*snapshot.liveDebugDatabase)
                                                : JsonUVE(nullptr)},
                   {"liveDebugFilter", snapshot.liveDebugFilter},
                   {"liveDebugTotalTraceEventCount", snapshot.liveDebugTotalTraceEventCount},
                   {"liveDebugVisibleTraceEventCount", snapshot.liveDebugVisibleTraceEventCount},
                   {"liveDebugTraceTruncated", snapshot.liveDebugTraceTruncated},
                   {"liveDebugDiagnostic", snapshot.liveDebugDiagnostic},
                   {"replayComparison", ToJsonUVE(snapshot.replayComparison)},
                   {"replayBaselines", ToJsonUVE(snapshot.replayBaselines)},
                   {"replayComparisonHistoryTruncated", snapshot.replayComparisonHistoryTruncated},
                   {"replayComparisonHistory", [&snapshot] {
                       JsonUVE history = JsonUVE::array();
                       for (const EditorBridgeMotionQueryReplayComparisonHistoryEntryUVE& entry :
                            snapshot.replayComparisonHistory) {
                           history.push_back(ToJsonUVE(entry));
                       }
                       return history;
                   }()},
                   {"replayWorkflow", ToJsonUVE(snapshot.replayWorkflow)},
                   {"replayBatch", ToJsonUVE(snapshot.replayBatch)},
                   {"replayBatchHistoryTruncated", snapshot.replayBatchHistoryTruncated},
                   {"replayBatchHistory", [&snapshot] {
                       JsonUVE history = JsonUVE::array();
                       for (const EditorBridgeMotionQueryReplayBatchHistoryEntryUVE& entry :
                            snapshot.replayBatchHistory) {
                           history.push_back(ToJsonUVE(entry));
                       }
                       return history;
                   }()},
                   {"replaySessionFacts", ToJsonUVE(snapshot.replaySessionFacts)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeSnapshotUVE& snapshot) {
    JsonUVE selectedEntities = JsonUVE::array();
    for (const EditorBridgeEntitySnapshotUVE& entity : snapshot.selectedEntities) {
        selectedEntities.push_back(ToJsonUVE(entity));
    }
    JsonUVE capabilities = JsonUVE::array();
    for (const EditorBridgeCapabilityUVE capability : snapshot.capabilities) {
        capabilities.push_back(static_cast<std::uint8_t>(capability));
    }
    JsonUVE tickHistory = JsonUVE::array();
    for (const EditorBridgeScriptRuntimeTickHistoryEntryUVE& entry : snapshot.scriptRuntimeTickHistory) {
        tickHistory.push_back(ToJsonUVE(entry));
    }

    return JsonUVE{{"protocolVersion", snapshot.protocolVersion},
                   {"revision", snapshot.revision},
                   {"editorState", static_cast<std::uint8_t>(snapshot.editorState)},
                   {"playModeState", static_cast<std::uint8_t>(snapshot.playModeState)},
                   {"sceneDirty", snapshot.sceneDirty},
                   {"canUndo", snapshot.canUndo},
                   {"canRedo", snapshot.canRedo},
                   {"activeScenePath", snapshot.activeScenePath.generic_string()},
                   {"selectedEntities", std::move(selectedEntities)},
                   {"selectedEntitiesTruncated", snapshot.selectedEntitiesTruncated},
                   {"activeEntity", snapshot.activeEntity.has_value() ? ToJsonUVE(*snapshot.activeEntity)
                                                                        : JsonUVE(nullptr)},
                   {"hierarchy", ToJsonUVE(snapshot.hierarchy)},
                   {"inspector", ToJsonUVE(snapshot.inspector)},
                   {"contentBrowser", ToJsonUVE(snapshot.contentBrowser)},
                   {"viewportSurface", ToJsonUVE(snapshot.viewportSurface)},
                   {"visualScripting", ToJsonUVE(snapshot.visualScripting)},
                   {"developerConsole", ToJsonUVE(snapshot.developerConsole)},
                   {"scriptRuntime", ToJsonUVE(snapshot.scriptRuntime)},
                   {"scriptRuntimeTickSummary", ToJsonUVE(snapshot.scriptRuntimeTickSummary)},
                   {"scriptRuntimeTickHistoryTruncated", snapshot.scriptRuntimeTickHistoryTruncated},
                   {"scriptRuntimeTickHistory", std::move(tickHistory)},
                   {"dataTableCatalog", ToJsonUVE(snapshot.dataTableCatalog)},
                   {"dataTablePreview", ToJsonUVE(snapshot.dataTablePreview)},
                   {"motionQuery", ToJsonUVE(snapshot.motionQuery)},
                   {"capabilities", std::move(capabilities)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeResponseUVE& response) {
    return JsonUVE{{"protocolVersion", response.protocolVersion},
                   {"requestId", response.requestId},
                   {"applied", response.applied},
                   {"code", response.code},
                   {"message", response.message},
                   {"snapshot", ToJsonUVE(response.snapshot)},
                   {"createdEntity", response.createdEntity.has_value() ? ToJsonUVE(*response.createdEntity)
                                                                         : JsonUVE(nullptr)},
                   {"graphSchema", response.visualScriptGraphSchema.has_value()
                                        ? ToJsonUVE(*response.visualScriptGraphSchema)
                                        : JsonUVE(nullptr)},
                   {"motionQueryReplayBaselineEnvelopePayload",
                    response.motionQueryReplayBaselineEnvelopePayload.has_value()
                        ? JsonUVE(*response.motionQueryReplayBaselineEnvelopePayload)
                        : JsonUVE(nullptr)}};
}

[[nodiscard]] FrameReadResultUVE ReadFrameUVE(std::istream& input, std::string& body) {
    std::array<char, 4U> header{};
    input.read(header.data(), static_cast<std::streamsize>(header.size()));
    const std::streamsize headerBytes = input.gcount();
    if (headerBytes == 0 && input.eof()) {
        return FrameReadResultUVE::EndOfFile;
    }
    if (headerBytes != static_cast<std::streamsize>(header.size())) {
        return FrameReadResultUVE::TruncatedHeader;
    }

    const std::uint32_t length =
        (static_cast<std::uint32_t>(static_cast<unsigned char>(header[0])) << 24U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(header[1])) << 16U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(header[2])) << 8U) |
        static_cast<std::uint32_t>(static_cast<unsigned char>(header[3]));
    if (length == 0U) {
        return FrameReadResultUVE::ZeroLength;
    }
    if (length > EditorBridgeStdioServerUVE::kMaximumFrameBytesUVE) {
        return FrameReadResultUVE::Oversized;
    }

    body.assign(length, '\0');
    input.read(body.data(), static_cast<std::streamsize>(body.size()));
    if (input.gcount() != static_cast<std::streamsize>(body.size())) {
        body.clear();
        return FrameReadResultUVE::TruncatedBody;
    }
    return FrameReadResultUVE::Body;
}

[[nodiscard]] bool WriteFrameUVE(std::ostream& output, const JsonUVE& payload) {
    const std::string body = payload.dump();
    if (body.empty() || body.size() > EditorBridgeStdioServerUVE::kMaximumFrameBytesUVE) {
        return false;
    }
    const std::uint32_t length = static_cast<std::uint32_t>(body.size());
    const std::array<char, 4U> header{
        static_cast<char>((length >> 24U) & 0xFFU), static_cast<char>((length >> 16U) & 0xFFU),
        static_cast<char>((length >> 8U) & 0xFFU), static_cast<char>(length & 0xFFU)};
    output.write(header.data(), static_cast<std::streamsize>(header.size()));
    output.write(body.data(), static_cast<std::streamsize>(body.size()));
    output.flush();
    return static_cast<bool>(output);
}

[[nodiscard]] JsonUVE MakeErrorUVE(const JsonUVE& id, const std::string_view code, const std::string_view message) {
    return JsonUVE{{"jsonrpc", "2.0"},
                   {"id", id},
                   {"error", {{"code", -32600}, {"message", message}, {"data", {{"code", code}}}}}};
}

[[nodiscard]] JsonUVE MakeResultUVE(const JsonUVE& id, JsonUVE result) {
    return JsonUVE{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

[[nodiscard]] std::optional<Scripting::ScriptGraphCanvasPointUVE> ParseCanvasPointUVE(const JsonUVE& value) {
    if (!value.is_object() || !value.contains("x") || !value.contains("y")) {
        return std::nullopt;
    }
    return Scripting::ScriptGraphCanvasPointUVE{value.at("x").get<float>(), value.at("y").get<float>()};
}

[[nodiscard]] std::optional<Scripting::ScriptGraphCanvasViewUVE> ParseCanvasViewUVE(const JsonUVE& value) {
    if (!value.is_object() || !value.contains("pan") || !value.contains("zoom")) {
        return std::nullopt;
    }
    const auto pan = ParseCanvasPointUVE(value.at("pan"));
    if (!pan.has_value()) {
        return std::nullopt;
    }
    return Scripting::ScriptGraphCanvasViewUVE{*pan, value.at("zoom").get<float>()};
}

[[nodiscard]] std::optional<Scripting::ScriptNodeUVE> ParseScriptNodeUVE(const JsonUVE& value) {
    if (!value.is_object() || !value.contains("id") || !value.contains("typeId")) {
        return std::nullopt;
    }
    return Scripting::ScriptNodeUVE{value.at("id").get<std::uint32_t>(), value.at("typeId").get<std::string>()};
}

[[nodiscard]] std::optional<Scripting::ScriptLinkUVE> ParseScriptLinkUVE(const JsonUVE& value) {
    if (!value.is_object() || !value.contains("output") || !value.contains("input") ||
        !value.at("output").is_object() || !value.at("input").is_object()) {
        return std::nullopt;
    }
    const JsonUVE& output = value.at("output");
    const JsonUVE& input = value.at("input");
    if (!output.contains("nodeId") || !output.contains("pinName") ||
        !input.contains("nodeId") || !input.contains("pinName")) {
        return std::nullopt;
    }
    return Scripting::ScriptLinkUVE{
        {output.at("nodeId").get<std::uint32_t>(), output.at("pinName").get<std::string>()},
        {input.at("nodeId").get<std::uint32_t>(), input.at("pinName").get<std::string>()}};
}

[[nodiscard]] std::optional<std::vector<std::uint32_t>> ParseScriptSelectionUVE(const JsonUVE& value) {
    if (!value.is_array() || value.size() > Scripting::kMaximumScriptGraphCanvasSelectionUVE) {
        return std::nullopt;
    }
    std::vector<std::uint32_t> selection;
    selection.reserve(value.size());
    for (const JsonUVE& nodeId : value) {
        if (!nodeId.is_number_unsigned()) {
            return std::nullopt;
        }
        selection.push_back(nodeId.get<std::uint32_t>());
    }
    return selection;
}

[[nodiscard]] std::optional<EditorBridgeRequestKindUVE> ParseRequestKindUVE(const std::string_view value) {
    if (value == "readSnapshot") {
        return EditorBridgeRequestKindUVE::ReadSnapshot;
    }
    if (value == "selectEntity") {
        return EditorBridgeRequestKindUVE::SelectEntity;
    }
    if (value == "clearSelection") {
        return EditorBridgeRequestKindUVE::ClearSelection;
    }
    if (value == "setSelectedEntityName") {
        return EditorBridgeRequestKindUVE::SetSelectedEntityName;
    }
    if (value == "createDocumentEntity") {
        return EditorBridgeRequestKindUVE::CreateDocumentEntity;
    }
    if (value == "undo") {
        return EditorBridgeRequestKindUVE::Undo;
    }
    if (value == "redo") {
        return EditorBridgeRequestKindUVE::Redo;
    }
    if (value == "toggleEntitySelection") {
        return EditorBridgeRequestKindUVE::ToggleEntitySelection;
    }
    if (value == "setHierarchyFilter") {
        return EditorBridgeRequestKindUVE::SetHierarchyFilter;
    }
    if (value == "setContentBrowserDirectory") {
        return EditorBridgeRequestKindUVE::SetContentBrowserDirectory;
    }
    if (value == "setContentBrowserFilter") {
        return EditorBridgeRequestKindUVE::SetContentBrowserFilter;
    }
    if (value == "setContentBrowserFocus") {
        return EditorBridgeRequestKindUVE::SetContentBrowserFocus;
    }
    if (value == "refreshContentBrowser") {
        return EditorBridgeRequestKindUVE::RefreshContentBrowser;
    }
    if (value == "selectContentBrowserEntry") {
        return EditorBridgeRequestKindUVE::SelectContentBrowserEntry;
    }
    if (value == "readVisualScriptCanvas") {
        return EditorBridgeRequestKindUVE::ReadVisualScriptCanvas;
    }
    if (value == "readVisualScriptDebugger") {
        return EditorBridgeRequestKindUVE::ReadVisualScriptDebugger;
    }
    if (value == "addVisualScriptNode") {
        return EditorBridgeRequestKindUVE::AddVisualScriptNode;
    }
    if (value == "removeVisualScriptNode") {
        return EditorBridgeRequestKindUVE::RemoveVisualScriptNode;
    }
    if (value == "moveVisualScriptNode") {
        return EditorBridgeRequestKindUVE::MoveVisualScriptNode;
    }
    if (value == "addVisualScriptLink") {
        return EditorBridgeRequestKindUVE::AddVisualScriptLink;
    }
    if (value == "removeVisualScriptLink") {
        return EditorBridgeRequestKindUVE::RemoveVisualScriptLink;
    }
    if (value == "setVisualScriptSelection") {
        return EditorBridgeRequestKindUVE::SetVisualScriptSelection;
    }
    if (value == "setVisualScriptView") {
        return EditorBridgeRequestKindUVE::SetVisualScriptView;
    }
    if (value == "undoVisualScript") {
        return EditorBridgeRequestKindUVE::UndoVisualScript;
    }
    if (value == "redoVisualScript") {
        return EditorBridgeRequestKindUVE::RedoVisualScript;
    }
    if (value == "readDeveloperConsole") {
        return EditorBridgeRequestKindUVE::ReadDeveloperConsole;
    }
    if (value == "submitDeveloperConsoleCommand") {
        return EditorBridgeRequestKindUVE::SubmitDeveloperConsoleCommand;
    }
    if (value == "clearDeveloperConsole") {
        return EditorBridgeRequestKindUVE::ClearDeveloperConsole;
    }
    if (value == "setDeveloperConsoleSeverityFilter") {
        return EditorBridgeRequestKindUVE::SetDeveloperConsoleSeverityFilter;
    }
    if (value == "setDeveloperConsoleCompletionPrefix") {
        return EditorBridgeRequestKindUVE::SetDeveloperConsoleCompletionPrefix;
    }
    if (value == "moveDeveloperConsoleHistory") {
        return EditorBridgeRequestKindUVE::MoveDeveloperConsoleHistory;
    }
    if (value == "selectDataTablePreview") {
        return EditorBridgeRequestKindUVE::SelectDataTablePreview;
    }
    if (value == "readScriptRuntime") {
        return EditorBridgeRequestKindUVE::ReadScriptRuntime;
    }
    if (value == "readScriptRuntimeTickDiagnostics") {
        return EditorBridgeRequestKindUVE::ReadScriptRuntimeTickDiagnostics;
    }
    if (value == "serializeGraph") {
        return EditorBridgeRequestKindUVE::SerializeVisualScriptGraph;
    }
    if (value == "deserializeGraph") {
        return EditorBridgeRequestKindUVE::DeserializeVisualScriptGraph;
    }
    if (value == "addVisualScriptNodeType") {
        return EditorBridgeRequestKindUVE::AddVisualScriptNodeType;
    }
    if (value == "setVisualScriptPinDefault") {
        return EditorBridgeRequestKindUVE::SetVisualScriptPinDefault;
    }
    if (value == "readMotionQuery") {
        return EditorBridgeRequestKindUVE::ReadMotionQuery;
    }
    if (value == "dispatchMotionQueryCommand") {
        return EditorBridgeRequestKindUVE::DispatchMotionQueryCommand;
    }
    if (value == "dispatchMotionQueryDebugCommand") {
        return EditorBridgeRequestKindUVE::DispatchMotionQueryDebugCommand;
    }
    if (value == "loadMotionQueryReplayBaseline") {
        return EditorBridgeRequestKindUVE::LoadMotionQueryReplayBaseline;
    }
    if (value == "clearMotionQueryReplayBaseline") {
        return EditorBridgeRequestKindUVE::ClearMotionQueryReplayBaseline;
    }
    if (value == "runMotionQueryReplayBaselineBatch") {
        return EditorBridgeRequestKindUVE::RunMotionQueryReplayBaselineBatch;
    }
    if (value == "exportMotionQueryReplayBaselineRegistry") {
        return EditorBridgeRequestKindUVE::ExportMotionQueryReplayBaselineRegistry;
    }
    if (value == "importMotionQueryReplayBaselineRegistry") {
        return EditorBridgeRequestKindUVE::ImportMotionQueryReplayBaselineRegistry;
    }
    if (value == "renameMotionQueryReplayBaseline") {
        return EditorBridgeRequestKindUVE::RenameMotionQueryReplayBaseline;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<EditorEntityKindUVE> ParseEntityKindUVE(const std::string_view value) {
    if (value == "empty") {
        return EditorEntityKindUVE::Empty;
    }
    if (value == "camera") {
        return EditorEntityKindUVE::Camera;
    }
    if (value == "directionalLight") {
        return EditorEntityKindUVE::DirectionalLight;
    }
    if (value == "collisionBox") {
        return EditorEntityKindUVE::CollisionBox;
    }
    if (value == "cube") {
        return EditorEntityKindUVE::Cube;
    }
    if (value == "uvSphere") {
        return EditorEntityKindUVE::UVSphere;
    }
    if (value == "plane") {
        return EditorEntityKindUVE::Plane;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<EditorBridgeEntityRefUVE> ParseEntityUVE(const JsonUVE& json) {
    if (!json.is_object() || !json.contains("index") || !json.contains("generation")) {
        return std::nullopt;
    }
    return EditorBridgeEntityRefUVE{json.at("index").get<std::uint32_t>(),
                                    json.at("generation").get<std::uint32_t>()};
}

[[nodiscard]] std::optional<Asset::ResourceHandleUVE> ParseResourceHandleUVE(const JsonUVE& json) {
    if (!json.is_object() || !json.contains("guid") || !json.contains("generation") ||
        !json.at("guid").is_number_unsigned() || !json.at("generation").is_number_unsigned()) {
        return std::nullopt;
    }
    return Asset::ResourceHandleUVE{Asset::AssetGuidUVE{json.at("guid").get<std::uint64_t>()},
                                   json.at("generation").get<std::uint64_t>()};
}

[[nodiscard]] std::optional<Plugins::Editor::MotionQueryEditorCommandKindUVE>
ParseMotionQueryCommandKindUVE(const std::string_view value) {
    using Kind = Plugins::Editor::MotionQueryEditorCommandKindUVE;
    if (value == "readSnapshot") return Kind::ReadSnapshot;
    if (value == "registerDatabase") return Kind::RegisterDatabase;
    if (value == "removeDatabase") return Kind::RemoveDatabase;
    if (value == "selectDatabase") return Kind::SelectDatabase;
    if (value == "setDisplayName") return Kind::SetDisplayName;
    if (value == "setSchemaId") return Kind::SetSchemaId;
    if (value == "setMaximumCandidates") return Kind::SetMaximumCandidates;
    if (value == "addCandidate") return Kind::AddCandidate;
    if (value == "removeCandidate") return Kind::RemoveCandidate;
    if (value == "validateDatabase") return Kind::ValidateDatabase;
    return std::nullopt;
}

[[nodiscard]] std::optional<Plugins::Editor::MotionQueryEditorCommandUVE>
ParseMotionQueryCommandUVE(const JsonUVE& json, const std::uint64_t requestId) {
    if (!json.is_object() || !json.contains("kind") || !json.contains("expectedRevision") ||
        !json.at("kind").is_string() || !json.at("expectedRevision").is_number_unsigned()) {
        return std::nullopt;
    }
    const auto kind = ParseMotionQueryCommandKindUVE(json.at("kind").get<std::string>());
    if (!kind.has_value()) {
        return std::nullopt;
    }
    Plugins::Editor::MotionQueryEditorCommandUVE command;
    command.requestId = requestId;
    command.expectedRevision = json.at("expectedRevision").get<std::uint64_t>();
    command.kind = *kind;
    if (json.contains("protocolVersion") && !json.at("protocolVersion").is_null()) {
        if (!json.at("protocolVersion").is_number_unsigned()) return std::nullopt;
        command.protocolVersion = json.at("protocolVersion").get<std::uint32_t>();
    }
    if (json.contains("resource") && !json.at("resource").is_null()) {
        command.resource = ParseResourceHandleUVE(json.at("resource"));
        if (!command.resource.has_value()) return std::nullopt;
    }
    if (json.contains("text") && !json.at("text").is_null()) {
        if (!json.at("text").is_string() || json.at("text").get_ref<const std::string&>().size() >
            Plugins::Editor::kMotionQueryEditorMaximumDisplayNameBytesUVE) return std::nullopt;
        command.text = json.at("text").get<std::string>();
    }
    if (json.contains("candidateIndex") && !json.at("candidateIndex").is_null()) {
        if (!json.at("candidateIndex").is_number_unsigned()) return std::nullopt;
        command.candidateIndex = json.at("candidateIndex").get<std::size_t>();
    }
    return command;
}

[[nodiscard]] std::optional<Plugins::Editor::MotionQueryLiveDebugCommandKindUVE>
ParseMotionQueryLiveDebugCommandKindUVE(const std::string_view value) {
    using Kind = Plugins::Editor::MotionQueryLiveDebugCommandKindUVE;
    if (value == "readSnapshot") return Kind::ReadSnapshot;
    if (value == "attach") return Kind::Attach;
    if (value == "detach") return Kind::Detach;
    if (value == "clearTrace") return Kind::ClearTrace;
    if (value == "clearSession") return Kind::ClearSession;
    if (value == "setFilter") return Kind::SetFilter;
    return std::nullopt;
}

[[nodiscard]] std::optional<Plugins::Editor::MotionQueryLiveDebugCommandUVE>
ParseMotionQueryLiveDebugCommandUVE(const JsonUVE& json, const std::uint64_t requestId) {
    if (!json.is_object() || !json.contains("kind") || !json.contains("expectedGeneration") ||
        !json.at("kind").is_string() || !json.at("expectedGeneration").is_number_unsigned()) {
        return std::nullopt;
    }
    const auto kind = ParseMotionQueryLiveDebugCommandKindUVE(json.at("kind").get<std::string>());
    if (!kind.has_value()) {
        return std::nullopt;
    }
    Plugins::Editor::MotionQueryLiveDebugCommandUVE command;
    command.requestId = requestId;
    command.expectedGeneration = json.at("expectedGeneration").get<std::uint64_t>();
    command.kind = *kind;
    if (json.contains("protocolVersion") && !json.at("protocolVersion").is_null()) {
        if (!json.at("protocolVersion").is_number_unsigned()) return std::nullopt;
        command.protocolVersion = json.at("protocolVersion").get<std::uint32_t>();
    }
    if (json.contains("database") && !json.at("database").is_null()) {
        command.database = ParseResourceHandleUVE(json.at("database"));
        if (!command.database.has_value()) return std::nullopt;
    }
    if (json.contains("filter") && !json.at("filter").is_null()) {
        if (!json.at("filter").is_string() || json.at("filter").get_ref<const std::string&>().size() >
            Plugins::Editor::kMotionQueryMaximumDebugMessageBytesUVE) return std::nullopt;
        command.filter = json.at("filter").get<std::string>();
    }
    if (json.contains("eventSequence") && !json.at("eventSequence").is_null()) {
        if (!json.at("eventSequence").is_number_unsigned()) return std::nullopt;
        command.eventSequence = json.at("eventSequence").get<std::uint64_t>();
    }
    return command;
}

[[nodiscard]] std::optional<EditorBridgeRequestUVE> ParseBridgeRequestUVE(const JsonUVE& params) {
    if (!params.is_object()) {
        return std::nullopt;
    }
    EditorBridgeRequestUVE request{};
    request.protocolVersion = params.at("protocolVersion").get<std::uint32_t>();
    request.requestId = params.at("requestId").get<std::uint64_t>();
    request.expectedRevision = params.at("expectedRevision").get<std::uint64_t>();
    const std::optional<EditorBridgeRequestKindUVE> kind = ParseRequestKindUVE(params.at("kind").get<std::string>());
    if (!kind.has_value()) {
        return std::nullopt;
    }
    request.kind = *kind;
    if (params.contains("motionQueryCommand") && !params.at("motionQueryCommand").is_null()) {
        request.motionQueryCommand = ParseMotionQueryCommandUVE(params.at("motionQueryCommand"), request.requestId);
        if (!request.motionQueryCommand.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("motionQueryDebugCommand") && !params.at("motionQueryDebugCommand").is_null()) {
        request.motionQueryDebugCommand = ParseMotionQueryLiveDebugCommandUVE(
            params.at("motionQueryDebugCommand"), request.requestId);
        if (!request.motionQueryDebugCommand.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("motionQueryReplayBaselineName") &&
        !params.at("motionQueryReplayBaselineName").is_null()) {
        if (!params.at("motionQueryReplayBaselineName").is_string() ||
            params.at("motionQueryReplayBaselineName").get_ref<const std::string&>().empty() ||
            params.at("motionQueryReplayBaselineName").get_ref<const std::string&>().size() >
                Plugins::Editor::kMotionQueryMaximumReplayBaselineNameBytesUVE) {
            return std::nullopt;
        }
        request.motionQueryReplayBaselineName = params.at("motionQueryReplayBaselineName").get<std::string>();
    }
    if (params.contains("motionQueryReplayFixturePayload") &&
        !params.at("motionQueryReplayFixturePayload").is_null()) {
        if (!params.at("motionQueryReplayFixturePayload").is_string() ||
            params.at("motionQueryReplayFixturePayload").get_ref<const std::string&>().size() >
                Plugins::Editor::kMotionQueryMaximumTraceReplayPayloadBytesUVE) {
            return std::nullopt;
        }
        request.motionQueryReplayFixturePayload = params.at("motionQueryReplayFixturePayload").get<std::string>();
    }
    if (params.contains("motionQueryReplayBaselineEnvelopePayload") &&
        !params.at("motionQueryReplayBaselineEnvelopePayload").is_null()) {
        if (!params.at("motionQueryReplayBaselineEnvelopePayload").is_string() ||
            params.at("motionQueryReplayBaselineEnvelopePayload").get_ref<const std::string&>().size() >
                Plugins::Editor::kMotionQueryMaximumReplayBaselineEnvelopeBytesUVE) {
            return std::nullopt;
        }
        request.motionQueryReplayBaselineEnvelopePayload =
            params.at("motionQueryReplayBaselineEnvelopePayload").get<std::string>();
    }
    if (params.contains("motionQueryReplayBaselineNewName") &&
        !params.at("motionQueryReplayBaselineNewName").is_null()) {
        if (!params.at("motionQueryReplayBaselineNewName").is_string() ||
            params.at("motionQueryReplayBaselineNewName").get_ref<const std::string&>().empty() ||
            params.at("motionQueryReplayBaselineNewName").get_ref<const std::string&>().size() >
                Plugins::Editor::kMotionQueryMaximumReplayBaselineNameBytesUVE) {
            return std::nullopt;
        }
        request.motionQueryReplayBaselineNewName = params.at("motionQueryReplayBaselineNewName").get<std::string>();
    }

    if (params.contains("entity") && !params.at("entity").is_null()) {
        request.entity = ParseEntityUVE(params.at("entity"));
        if (!request.entity.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("entityName") && !params.at("entityName").is_null()) {
        request.entityName = params.at("entityName").get<std::string>();
    }
    if (params.contains("entityKind") && !params.at("entityKind").is_null()) {
        request.entityKind = ParseEntityKindUVE(params.at("entityKind").get<std::string>());
        if (!request.entityKind.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("hierarchyFilter") && !params.at("hierarchyFilter").is_null()) {
        request.hierarchyFilter = params.at("hierarchyFilter").get<std::string>();
    }
    if (params.contains("contentDirectory") && !params.at("contentDirectory").is_null()) {
        request.contentDirectory = params.at("contentDirectory").get<std::string>();
    }
    if (params.contains("contentFilter") && !params.at("contentFilter").is_null()) {
        request.contentFilter = params.at("contentFilter").get<std::string>();
    }
    if (params.contains("contentFocus") && !params.at("contentFocus").is_null()) {
        request.contentFocus = params.at("contentFocus").get<std::string>();
    }
    if (params.contains("contentEntryPath") && !params.at("contentEntryPath").is_null()) {
        request.contentEntryPath = params.at("contentEntryPath").get<std::string>();
    }
    if (params.contains("visualScriptNodeId") && !params.at("visualScriptNodeId").is_null()) {
        request.visualScriptNodeId = params.at("visualScriptNodeId").get<std::uint32_t>();
    }
    if (params.contains("visualScriptNode") && !params.at("visualScriptNode").is_null()) {
        request.visualScriptNode = ParseScriptNodeUVE(params.at("visualScriptNode"));
        if (!request.visualScriptNode.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("visualScriptNodeTypeId") && !params.at("visualScriptNodeTypeId").is_null()) {
        if (!params.at("visualScriptNodeTypeId").is_string() ||
            params.at("visualScriptNodeTypeId").get_ref<const std::string&>().empty() ||
            params.at("visualScriptNodeTypeId").get_ref<const std::string&>().size() > 256U) {
            return std::nullopt;
        }
        request.visualScriptNodeTypeId = params.at("visualScriptNodeTypeId").get<std::string>();
    }
    if (params.contains("visualScriptPosition") && !params.at("visualScriptPosition").is_null()) {
        request.visualScriptPosition = ParseCanvasPointUVE(params.at("visualScriptPosition"));
        if (!request.visualScriptPosition.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("visualScriptLink") && !params.at("visualScriptLink").is_null()) {
        request.visualScriptLink = ParseScriptLinkUVE(params.at("visualScriptLink"));
        if (!request.visualScriptLink.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("visualScriptSelection") && !params.at("visualScriptSelection").is_null()) {
        request.visualScriptSelection = ParseScriptSelectionUVE(params.at("visualScriptSelection"));
        if (!request.visualScriptSelection.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("visualScriptView") && !params.at("visualScriptView").is_null()) {
        request.visualScriptView = ParseCanvasViewUVE(params.at("visualScriptView"));
        if (!request.visualScriptView.has_value()) {
            return std::nullopt;
        }
    }
    if (params.contains("visualScriptGraphSchema") && !params.at("visualScriptGraphSchema").is_null()) {
        if (!params.at("visualScriptGraphSchema").is_string() ||
            params.at("visualScriptGraphSchema").get_ref<const std::string&>().size() >
                EditorBridgeStdioServerUVE::kMaximumFrameBytesUVE) {
            return std::nullopt;
        }
        request.visualScriptGraphSchema = params.at("visualScriptGraphSchema").get<std::string>();
    }
    if (params.contains("visualScriptPinName") && !params.at("visualScriptPinName").is_null()) {
        if (!params.at("visualScriptPinName").is_string() ||
            params.at("visualScriptPinName").get_ref<const std::string&>().empty() ||
            params.at("visualScriptPinName").get_ref<const std::string&>().size() > 256U) {
            return std::nullopt;
        }
        request.visualScriptPinName = params.at("visualScriptPinName").get<std::string>();
    }
    if (params.contains("visualScriptDefaultValue") && !params.at("visualScriptDefaultValue").is_null()) {
        if (!params.at("visualScriptDefaultValue").is_string() ||
            params.at("visualScriptDefaultValue").get_ref<const std::string&>().size() >
                Scripting::kMaximumScriptGraphCanvasDefaultValueBytesUVE) {
            return std::nullopt;
        }
        request.visualScriptDefaultValue = params.at("visualScriptDefaultValue").get<std::string>();
    }
    if (params.contains("dataTableName") && !params.at("dataTableName").is_null()) {
        request.dataTableName = params.at("dataTableName").get<std::string>();
    }
    if (params.contains("developerConsoleCommand") && !params.at("developerConsoleCommand").is_null()) {
        request.developerConsoleCommand = params.at("developerConsoleCommand").get<std::string>();
    }
    if (params.contains("developerConsoleSeverityFilter") && !params.at("developerConsoleSeverityFilter").is_null()) {
        const std::uint8_t filter = params.at("developerConsoleSeverityFilter").get<std::uint8_t>();
        if (filter > static_cast<std::uint8_t>(DeveloperConsoleSeverityFilterUVE::Error)) {
            return std::nullopt;
        }
        request.developerConsoleSeverityFilter = static_cast<DeveloperConsoleSeverityFilterUVE>(filter);
    }
    if (params.contains("developerConsoleCompletionPrefix") && !params.at("developerConsoleCompletionPrefix").is_null()) {
        request.developerConsoleCompletionPrefix = params.at("developerConsoleCompletionPrefix").get<std::string>();
    }
    if (params.contains("developerConsoleHistoryDelta") && !params.at("developerConsoleHistoryDelta").is_null()) {
        request.developerConsoleHistoryDelta = params.at("developerConsoleHistoryDelta").get<std::int32_t>();
    }
    return request;
}

[[nodiscard]] bool IsJsonRpcRequestUVE(const JsonUVE& request) {
    return request.is_object() && request.value("jsonrpc", "") == "2.0" && request.contains("method") &&
           request.at("method").is_string();
}

[[nodiscard]] JsonUVE DispatchJsonRequestUVE(EditorBridgeUVE& bridge, const JsonUVE& request) {
    const JsonUVE id = request.value("id", JsonUVE(nullptr));
    if (!IsJsonRpcRequestUVE(request)) {
        return MakeErrorUVE(id, "bridge.request.invalid", "The request is not a JSON-RPC 2.0 object.");
    }

    const std::string method = request.at("method").get<std::string>();
    const JsonUVE params = request.value("params", JsonUVE::object());
    try {
        if (method == "bridge.hello") {
            const std::uint32_t requestedVersion = params.at("protocolVersion").get<std::uint32_t>();
            const EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();
            const bool compatible = requestedVersion == kEditorBridgeProtocolVersionUVE;
            return MakeResultUVE(id, JsonUVE{{"code", compatible ? "bridge.hello.compatible"
                                                                  : "bridge.protocol.unsupported"},
                                             {"compatible", compatible},
                                             {"protocolVersion", kEditorBridgeProtocolVersionUVE},
                                             {"snapshot", ToJsonUVE(snapshot)}});
        }
        if (method == "bridge.getSnapshot") {
            return MakeResultUVE(id, JsonUVE{{"code", "bridge.snapshot.read"},
                                             {"snapshot", ToJsonUVE(bridge.GetSnapshotUVE())}});
        }
        if (method == "bridge.dispatch") {
            const std::optional<EditorBridgeRequestUVE> parsed = ParseBridgeRequestUVE(params);
            if (!parsed.has_value()) {
                return MakeErrorUVE(id, "bridge.request.invalid", "The bridge dispatch payload is invalid.");
            }
            return MakeResultUVE(id, ToJsonUVE(bridge.DispatchUVE(*parsed)));
        }
    } catch (const JsonUVE::exception&) {
        return MakeErrorUVE(id, "bridge.request.invalid", "The request contains invalid bridge field values.");
    }
    return MakeErrorUVE(id, "bridge.request.invalid", "The bridge method is not supported.");
}

[[nodiscard]] std::string_view FrameReadDiagnosticUVE(const FrameReadResultUVE result) {
    switch (result) {
        case FrameReadResultUVE::TruncatedHeader:
            return "bridge.transport.frame.truncated_header";
        case FrameReadResultUVE::TruncatedBody:
            return "bridge.transport.frame.truncated_body";
        case FrameReadResultUVE::ZeroLength:
            return "bridge.transport.frame.zero_length";
        case FrameReadResultUVE::Oversized:
            return "bridge.transport.frame.oversized";
        case FrameReadResultUVE::Body:
        case FrameReadResultUVE::EndOfFile:
            return "bridge.transport.frame.invalid";
    }
    return "bridge.transport.frame.invalid";
}

} // namespace

EditorBridgeStdioServerUVE::EditorBridgeStdioServerUVE(EditorBridgeUVE& bridge) noexcept : m_bridge(&bridge) {}

int EditorBridgeStdioServerUVE::ServeUVE(std::istream& input, std::ostream& output, std::ostream& diagnostics) {
    while (true) {
        std::string body;
        const FrameReadResultUVE frameResult = ReadFrameUVE(input, body);
        if (frameResult == FrameReadResultUVE::EndOfFile) {
            return 0;
        }
        if (frameResult != FrameReadResultUVE::Body) {
            diagnostics << FrameReadDiagnosticUVE(frameResult) << '\n';
            static_cast<void>(WriteFrameUVE(output, MakeErrorUVE(JsonUVE(nullptr), FrameReadDiagnosticUVE(frameResult),
                                                                  "The bridge frame is malformed or outside protocol bounds.")));
            return 2;
        }

        try {
            const JsonUVE request = JsonUVE::parse(body);
            if (!WriteFrameUVE(output, DispatchJsonRequestUVE(*m_bridge, request))) {
                diagnostics << "bridge.transport.write.failed\n";
                return 3;
            }
        } catch (const JsonUVE::exception&) {
            diagnostics << "bridge.transport.json.invalid\n";
            if (!WriteFrameUVE(output, MakeErrorUVE(JsonUVE(nullptr), "bridge.transport.json.invalid",
                                                     "The bridge frame body is not valid UTF-8 JSON."))) {
                return 3;
            }
        }
    }
}

} // namespace UVE::Editor
