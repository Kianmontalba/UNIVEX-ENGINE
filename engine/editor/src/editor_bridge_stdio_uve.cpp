// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_bridge_stdio_uve.h"

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
                   {"type", static_cast<std::uint8_t>(pin.type)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) {
    JsonUVE pins = JsonUVE::array();
    for (const auto& pin : node.pins) {
        pins.push_back(ToJsonUVE(pin));
    }
    return JsonUVE{{"id", node.id}, {"typeId", node.typeId}, {"displayName", node.displayName},
                   {"x", node.position.x}, {"y", node.position.y}, {"selected", node.selected},
                   {"pins", std::move(pins)}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptGraphCanvasLinkSnapshotUVE& link) {
    return JsonUVE{{"output", {{"nodeId", link.link.output.nodeId}, {"pinName", link.link.output.pinName}}},
                   {"input", {{"nodeId", link.link.input.nodeId}, {"pinName", link.link.input.pinName}}}};
}

[[nodiscard]] JsonUVE ToJsonUVE(const Scripting::ScriptValidationDiagnosticUVE& diagnostic) {
    return JsonUVE{{"code", static_cast<std::uint8_t>(diagnostic.code)}, {"nodeId", diagnostic.nodeId},
                   {"pinName", diagnostic.pinName}, {"message", diagnostic.message}};
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
    JsonUVE diagnostics = JsonUVE::array();
    for (const auto& diagnostic : canvas.diagnostics) {
        diagnostics.push_back(ToJsonUVE(diagnostic));
    }
    return JsonUVE{{"revision", canvas.revision}, {"graphRevision", canvas.graphRevision},
                   {"pan", {{"x", canvas.view.pan.x}, {"y", canvas.view.pan.y}}},
                   {"zoom", canvas.view.zoom}, {"nodesTruncated", canvas.nodesTruncated},
                   {"linksTruncated", canvas.linksTruncated}, {"paletteTruncated", canvas.paletteTruncated},
                   {"diagnosticsTruncated", canvas.diagnosticsTruncated}, {"nodes", std::move(nodes)},
                   {"links", std::move(links)}, {"selectedNodeIds", std::move(selection)},
                   {"paletteNodeTypeIds", std::move(palette)}, {"diagnostics", std::move(diagnostics)}};
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

[[nodiscard]] JsonUVE ToJsonUVE(const EditorBridgeSnapshotUVE& snapshot) {
    JsonUVE selectedEntities = JsonUVE::array();
    for (const EditorBridgeEntitySnapshotUVE& entity : snapshot.selectedEntities) {
        selectedEntities.push_back(ToJsonUVE(entity));
    }
    JsonUVE capabilities = JsonUVE::array();
    for (const EditorBridgeCapabilityUVE capability : snapshot.capabilities) {
        capabilities.push_back(static_cast<std::uint8_t>(capability));
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
                   {"dataTableCatalog", ToJsonUVE(snapshot.dataTableCatalog)},
                   {"dataTablePreview", ToJsonUVE(snapshot.dataTablePreview)},
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
