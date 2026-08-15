// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_graph_canvas_uve.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

namespace UVE::Scripting {
namespace {
[[nodiscard]] bool ContainsIdUVE(const std::vector<std::uint32_t>& ids, const std::uint32_t id) noexcept {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

[[nodiscard]] bool IsValidDefaultValueUVE(const ScriptValueTypeUVE type, const std::string& value) noexcept {
    if (value.empty() || value.size() > kMaximumScriptGraphCanvasDefaultValueBytesUVE) {
        return false;
    }
    if (type == ScriptValueTypeUVE::Boolean) {
        return value == "true" || value == "false";
    }
    if (type != ScriptValueTypeUVE::Number) {
        return false;
    }
    char* end = nullptr;
    const float parsed = std::strtof(value.c_str(), &end);
    return end != value.c_str() && end != nullptr && *end == '\0' && std::isfinite(parsed);
}

[[nodiscard]] std::vector<std::uint32_t> RemoveIdUVE(std::vector<std::uint32_t> ids,
                                                      const std::uint32_t id) {
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    return ids;
}
} // namespace

ScriptGraphCanvasUVE::ScriptGraphCanvasUVE(ScriptNodeRegistryUVE& registry,
                                           const std::size_t historyCapacity)
    : m_registry(&registry),
      m_historyCapacity(std::max<std::size_t>(1U, historyCapacity)) {}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::MakeResultUVE(
    const ScriptGraphCanvasCommandCodeUVE code, std::string message) const {
    return ScriptGraphCanvasCommandResultUVE{code, m_revision, std::move(message)};
}

bool ScriptGraphCanvasUVE::CheckExpectedRevisionUVE(const std::uint64_t expectedRevision) const noexcept {
    return expectedRevision == 0U || expectedRevision == m_revision;
}

bool ScriptGraphCanvasUVE::IsFinitePointUVE(const ScriptGraphCanvasPointUVE point) const noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool ScriptGraphCanvasUVE::IsValidViewUVE(const ScriptGraphCanvasViewUVE view) const noexcept {
    return IsFinitePointUVE(view.pan) && std::isfinite(view.zoom) &&
           view.zoom >= kMinimumScriptGraphCanvasZoomUVE &&
           view.zoom <= kMaximumScriptGraphCanvasZoomUVE;
}

bool ScriptGraphCanvasUVE::IsSelectionValidUVE(const std::vector<std::uint32_t>& nodeIds) const noexcept {
    if (nodeIds.size() > kMaximumScriptGraphCanvasSelectionUVE) {
        return false;
    }
    for (std::size_t index = 0U; index < nodeIds.size(); ++index) {
        if (nodeIds[index] == 0U || !HasNodeUVE(nodeIds[index]) ||
            std::find(nodeIds.begin(), nodeIds.begin() + static_cast<std::ptrdiff_t>(index), nodeIds[index]) !=
                nodeIds.begin() + static_cast<std::ptrdiff_t>(index)) {
            return false;
        }
    }
    return true;
}

const ScriptGraphCanvasPinDefaultOverrideUVE* ScriptGraphCanvasUVE::FindPinDefaultOverrideUVE(
    const std::uint32_t nodeId, const std::string_view pinName) const noexcept {
    const auto iterator = std::find_if(m_pinDefaultOverrides.cbegin(), m_pinDefaultOverrides.cend(),
                                       [nodeId, pinName](const auto& entry) {
                                           return entry.nodeId == nodeId && entry.pinName == pinName;
                                       });
    return iterator == m_pinDefaultOverrides.cend() ? nullptr : &*iterator;
}

ScriptGraphCanvasPinDefaultOverrideUVE* ScriptGraphCanvasUVE::FindPinDefaultOverrideUVE(
    const std::uint32_t nodeId, const std::string_view pinName) noexcept {
    const auto iterator = std::find_if(m_pinDefaultOverrides.begin(), m_pinDefaultOverrides.end(),
                                       [nodeId, pinName](const auto& entry) {
                                           return entry.nodeId == nodeId && entry.pinName == pinName;
                                       });
    return iterator == m_pinDefaultOverrides.end() ? nullptr : &*iterator;
}

const ScriptGraphCanvasUVE::LayoutEntryUVE* ScriptGraphCanvasUVE::FindLayoutUVE(
    const std::uint32_t nodeId) const noexcept {
    const auto iterator = std::find_if(m_layout.begin(), m_layout.end(),
                                       [nodeId](const LayoutEntryUVE& entry) { return entry.nodeId == nodeId; });
    return iterator == m_layout.end() ? nullptr : &*iterator;
}

ScriptGraphCanvasUVE::LayoutEntryUVE* ScriptGraphCanvasUVE::FindLayoutUVE(
    const std::uint32_t nodeId) noexcept {
    const auto iterator = std::find_if(m_layout.begin(), m_layout.end(),
                                       [nodeId](const LayoutEntryUVE& entry) { return entry.nodeId == nodeId; });
    return iterator == m_layout.end() ? nullptr : &*iterator;
}

ScriptGraphCanvasUVE::StateUVE ScriptGraphCanvasUVE::CaptureStateUVE() const {
    return StateUVE{m_backend.GetGraphUVE(), m_layout, m_selection, m_pinDefaultOverrides};
}

void ScriptGraphCanvasUVE::RestoreStateUVE(StateUVE state) {
    m_backend.RestoreGraphUVE(std::move(state.graph));
    m_layout = std::move(state.layout);
    m_selection = std::move(state.selection);
    m_pinDefaultOverrides = std::move(state.pinDefaultOverrides);
}

void ScriptGraphCanvasUVE::RecordMutationUVE(StateUVE before, const bool marksDirty) {
    m_undo.push_back(std::move(before));
    while (m_undo.size() > m_historyCapacity) {
        m_undo.pop_front();
    }
    m_redo.clear();
    if (marksDirty) {
        m_dirty = true;
    }
    ++m_revision;
    ++m_graphRevision;
}

void ScriptGraphCanvasUVE::BumpRevisionUVE() noexcept {
    if (m_revision < std::numeric_limits<std::uint64_t>::max()) {
        ++m_revision;
    }
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::ValidateAndApplyGraphEditUVE(
    ScriptGraphUVE candidate, std::string operation) {
    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = candidate.ValidateUVE(*m_registry);
    if (!diagnostics.empty()) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             std::move(operation) + " was rejected by graph validation.");
    }
    m_backend.RestoreGraphUVE(std::move(candidate));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, std::move(operation) + " was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::AddNodeTypeUVE(
    std::string typeId, const ScriptGraphCanvasPointUVE position,
    const std::uint64_t expectedRevision) {
    if (typeId.empty() || typeId.size() > 256U) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "AddNodeType requires a bounded non-empty type ID.");
    }
    std::uint32_t candidateId = 1U;
    while (candidateId != std::numeric_limits<std::uint32_t>::max() && HasNodeUVE(candidateId)) {
        ++candidateId;
    }
    if (candidateId == std::numeric_limits<std::uint32_t>::max()) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "AddNodeType could not allocate a node ID.");
    }
    return AddNodeUVE(ScriptNodeUVE{candidateId, std::move(typeId)}, position, expectedRevision);
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::SetPinDefaultValueUVE(
    const std::uint32_t nodeId, std::string pinName, std::string value,
    const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (nodeId == 0U || pinName.empty() || pinName.size() > kMaximumScriptGraphCanvasDefaultValueBytesUVE ||
        value.size() > kMaximumScriptGraphCanvasDefaultValueBytesUVE) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "SetPinDefaultValue requires bounded node, pin, and value fields.");
    }
    const ScriptNodeUVE* node = nullptr;
    for (const ScriptNodeUVE& candidate : m_backend.GetGraphUVE().GetNodesUVE()) {
        if (candidate.id == nodeId) {
            node = &candidate;
            break;
        }
    }
    const ScriptNodeTypeDescriptorUVE* descriptor = node == nullptr
        ? nullptr : m_registry->FindNodeTypeUVE(node->typeId);
    const ScriptPinDescriptorUVE* pin = nullptr;
    if (descriptor != nullptr) {
        for (const ScriptPinDescriptorUVE& candidate : descriptor->pins) {
            if (candidate.name == pinName) {
                pin = &candidate;
                break;
            }
        }
    }
    if (pin == nullptr || pin->direction != ScriptPinDirectionUVE::Input ||
        pin->role != ScriptPinRoleUVE::Data || !IsValidDefaultValueUVE(pin->type, value)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "SetPinDefaultValue requires a registered data input with a supported value.");
    }
    if (const ScriptGraphCanvasPinDefaultOverrideUVE* existing =
            FindPinDefaultOverrideUVE(nodeId, pinName); existing != nullptr && existing->value == value) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory,
                             "SetPinDefaultValue made no change.");
    }
    StateUVE before = CaptureStateUVE();
    if (ScriptGraphCanvasPinDefaultOverrideUVE* existing = FindPinDefaultOverrideUVE(nodeId, pinName); existing != nullptr) {
        existing->value = std::move(value);
    } else {
        m_pinDefaultOverrides.push_back(
            ScriptGraphCanvasPinDefaultOverrideUVE{nodeId, std::move(pinName), std::move(value)});
    }
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "SetPinDefaultValue was applied and recorded in native canvas history.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::AddNodeUVE(
    const ScriptNodeUVE node, const ScriptGraphCanvasPointUVE position,
    const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (!IsFinitePointUVE(position) || node.id == 0U || m_registry->FindNodeTypeUVE(node.typeId) == nullptr ||
        HasNodeUVE(node.id)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "AddNode requires a unique ID, registered type, and finite position.");
    }
    StateUVE before = CaptureStateUVE();
    ScriptGraphUVE candidate = before.graph;
    if (!candidate.AddNodeUVE(node)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected, "AddNode could not mutate the graph.");
    }
    const ScriptGraphCanvasCommandResultUVE result = ValidateAndApplyGraphEditUVE(
        std::move(candidate), "AddNode");
    if (!result.IsAppliedUVE()) {
        return result;
    }
    m_layout.push_back(LayoutEntryUVE{node.id, position});
    m_selection = {node.id};
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "AddNode was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::RemoveNodeUVE(
    const std::uint32_t nodeId, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (!HasNodeUVE(nodeId)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected, "RemoveNode requires a live node ID.");
    }
    StateUVE before = CaptureStateUVE();
    ScriptGraphUVE candidate = before.graph;
    if (!candidate.RemoveNodeUVE(nodeId)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected, "RemoveNode could not mutate the graph.");
    }
    const ScriptGraphCanvasCommandResultUVE result = ValidateAndApplyGraphEditUVE(
        std::move(candidate), "RemoveNode");
    if (!result.IsAppliedUVE()) {
        return result;
    }
    m_layout.erase(std::remove_if(m_layout.begin(), m_layout.end(),
                                  [nodeId](const LayoutEntryUVE& entry) { return entry.nodeId == nodeId; }),
                   m_layout.end());
    m_selection = RemoveIdUVE(std::move(m_selection), nodeId);
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "RemoveNode was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::MoveNodeUVE(
    const std::uint32_t nodeId, const ScriptGraphCanvasPointUVE position,
    const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    LayoutEntryUVE* layout = FindLayoutUVE(nodeId);
    if (layout == nullptr || !IsFinitePointUVE(position)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "MoveNode requires a live node and finite position.");
    }
    if (layout->position == position) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory, "MoveNode made no change.");
    }
    StateUVE before = CaptureStateUVE();
    layout->position = position;
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "MoveNode was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::AddLinkUVE(
    const ScriptLinkUVE link, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    StateUVE before = CaptureStateUVE();
    ScriptGraphUVE candidate = before.graph;
    if (!candidate.AddLinkUVE(link)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "AddLink was rejected by structural graph validation.");
    }
    const ScriptGraphCanvasCommandResultUVE result = ValidateAndApplyGraphEditUVE(
        std::move(candidate), "AddLink");
    if (!result.IsAppliedUVE()) {
        return result;
    }
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "AddLink was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::RemoveLinkUVE(
    const ScriptLinkUVE& link, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    StateUVE before = CaptureStateUVE();
    ScriptGraphUVE candidate = before.graph;
    if (!candidate.RemoveLinkUVE(link)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "RemoveLink requires an existing exact link.");
    }
    const ScriptGraphCanvasCommandResultUVE result = ValidateAndApplyGraphEditUVE(
        std::move(candidate), "RemoveLink");
    if (!result.IsAppliedUVE()) {
        return result;
    }
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "RemoveLink was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::SetSelectionUVE(
    std::vector<std::uint32_t> nodeIds, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (!IsSelectionValidUVE(nodeIds)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "SetSelection requires unique live node IDs within the selection bound.");
    }
    if (nodeIds == m_selection) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory, "SetSelection made no change.");
    }
    StateUVE before = CaptureStateUVE();
    m_selection = std::move(nodeIds);
    RecordMutationUVE(std::move(before), false);
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied, "SetSelection was applied.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::SetViewUVE(
    const ScriptGraphCanvasViewUVE view, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (!IsValidViewUVE(view)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "SetView requires finite pan and bounded positive zoom.");
    }
    if (view == m_view) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory, "SetView made no change.");
    }
    m_view = view;
    BumpRevisionUVE();
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "SetView was applied without creating authoring history.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::UndoUVE(const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (m_undo.empty()) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory, "Canvas undo history is empty.");
    }
    m_redo.push_back(CaptureStateUVE());
    while (m_redo.size() > m_historyCapacity) {
        m_redo.pop_front();
    }
    RestoreStateUVE(std::move(m_undo.back()));
    m_undo.pop_back();
    BumpRevisionUVE();
    ++m_graphRevision;
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "Canvas undo restored the exact prior graph/layout/selection state.");
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::RedoUVE(const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas command used a stale revision.");
    }
    if (m_redo.empty()) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory, "Canvas redo history is empty.");
    }
    m_undo.push_back(CaptureStateUVE());
    while (m_undo.size() > m_historyCapacity) {
        m_undo.pop_front();
    }
    RestoreStateUVE(std::move(m_redo.back()));
    m_redo.pop_back();
    BumpRevisionUVE();
    ++m_graphRevision;
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "Canvas redo restored the exact next graph/layout/selection state.");
}

const ScriptGraphUVE& ScriptGraphCanvasUVE::GetGraphUVE() const noexcept {
    return m_backend.GetGraphUVE();
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::ApplyGraphSchemaUVE(
    ScriptGraphSchemaUVE schema, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The graph schema used a stale revision.");
    }
    if (schema.schemaVersion != kScriptGraphSchemaVersionUVE) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "The graph schema version is not supported.");
    }
    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = schema.graph.ValidateUVE(*m_registry);
    if (!diagnostics.empty()) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "The graph schema was rejected by native graph validation.");
    }
    if (schema.layout.size() != schema.graph.GetNodesUVE().size() ||
        schema.layout.size() > kMaximumScriptGraphCanvasEntriesUVE ||
        !IsValidViewUVE(m_view)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "The graph schema requires one finite layout position for every live node.");
    }
    std::vector<std::uint32_t> nodeIds;
    nodeIds.reserve(schema.graph.GetNodesUVE().size());
    const auto hasSchemaNode = [&schema](const std::uint32_t nodeId) {
        return std::find_if(schema.graph.GetNodesUVE().begin(), schema.graph.GetNodesUVE().end(),
                            [nodeId](const ScriptNodeUVE& node) { return node.id == nodeId; }) !=
               schema.graph.GetNodesUVE().end();
    };
    for (const ScriptGraphLayoutEntryUVE& entry : schema.layout) {
        if (entry.nodeId == 0U || !hasSchemaNode(entry.nodeId) || !IsFinitePointUVE({entry.x, entry.y}) ||
            ContainsIdUVE(nodeIds, entry.nodeId)) {
            return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                                 "The graph schema layout must contain unique live node IDs and finite positions.");
        }
        nodeIds.push_back(entry.nodeId);
    }
    StateUVE before = CaptureStateUVE();
    m_backend.RestoreGraphUVE(std::move(schema.graph));
    m_layout.clear();
    m_layout.reserve(schema.layout.size());
    for (const ScriptGraphLayoutEntryUVE& entry : schema.layout) {
        m_layout.push_back(LayoutEntryUVE{entry.nodeId, {entry.x, entry.y}});
    }
    m_selection.clear();
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "The graph schema was applied through the native canvas history path.");
}

ScriptGraphCanvasLayoutSnapshotUVE ScriptGraphCanvasUVE::GetLayoutSnapshotUVE() const {
    ScriptGraphCanvasLayoutSnapshotUVE snapshot{};
    snapshot.view = m_view;
    const auto& graphNodes = m_backend.GetGraphUVE().GetNodesUVE();
    snapshot.entries.reserve(graphNodes.size());
    for (const ScriptNodeUVE& node : graphNodes) {
        if (const LayoutEntryUVE* layout = FindLayoutUVE(node.id); layout != nullptr) {
            snapshot.entries.push_back(*layout);
        }
    }
    return snapshot;
}

ScriptGraphCanvasCommandResultUVE ScriptGraphCanvasUVE::ApplyLayoutUVE(
    ScriptGraphCanvasLayoutSnapshotUVE layout, const std::uint64_t expectedRevision) {
    if (!CheckExpectedRevisionUVE(expectedRevision)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::StaleRevision,
                             "The canvas layout used a stale revision.");
    }
    const std::size_t liveNodeCount = m_backend.GetGraphUVE().GetNodesUVE().size();
    if (layout.entries.size() > kMaximumScriptGraphCanvasEntriesUVE ||
        layout.entries.size() != liveNodeCount || !IsValidViewUVE(layout.view)) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                             "ApplyLayout requires a complete live-node layout and a finite, bounded view.");
    }
    std::vector<std::uint32_t> nodeIds;
    nodeIds.reserve(layout.entries.size());
    for (const ScriptGraphCanvasLayoutEntryUVE& entry : layout.entries) {
        if (entry.nodeId == 0U || !HasNodeUVE(entry.nodeId) || !IsFinitePointUVE(entry.position) ||
            ContainsIdUVE(nodeIds, entry.nodeId)) {
            return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Rejected,
                                 "ApplyLayout requires unique live node IDs and finite positions.");
        }
        nodeIds.push_back(entry.nodeId);
    }
    const ScriptGraphCanvasLayoutSnapshotUVE current = GetLayoutSnapshotUVE();
    if (current == layout) {
        return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::NoHistory,
                             "ApplyLayout made no change.");
    }
    StateUVE before = CaptureStateUVE();
    m_view = layout.view;
    m_layout = std::move(layout.entries);
    RecordMutationUVE(std::move(before));
    return MakeResultUVE(ScriptGraphCanvasCommandCodeUVE::Applied,
                         "ApplyLayout was applied with undo support.");
}

bool ScriptGraphCanvasUVE::HasNodeUVE(const std::uint32_t nodeId) const noexcept {
    const auto& nodes = m_backend.GetGraphUVE().GetNodesUVE();
    return std::find_if(nodes.begin(), nodes.end(),
                        [nodeId](const ScriptNodeUVE& node) { return node.id == nodeId; }) != nodes.end();
}

std::size_t ScriptGraphCanvasUVE::GetUndoCountUVE() const noexcept {
    return m_undo.size();
}

std::size_t ScriptGraphCanvasUVE::GetRedoCountUVE() const noexcept {
    return m_redo.size();
}

ScriptGraphCanvasSnapshotUVE ScriptGraphCanvasUVE::GetSnapshotUVE() const {
    ScriptGraphCanvasSnapshotUVE snapshot{};
    snapshot.revision = m_revision;
    snapshot.graphRevision = m_graphRevision;
    snapshot.view = m_view;
    const auto& graph = m_backend.GetGraphUVE();
    const auto& graphNodes = graph.GetNodesUVE();
    const auto& graphLinks = graph.GetLinksUVE();

    const std::size_t nodeCount = std::min(graphNodes.size(), kMaximumScriptGraphCanvasEntriesUVE);
    snapshot.nodes.reserve(nodeCount);
    for (std::size_t index = 0U; index < nodeCount; ++index) {
        const ScriptNodeUVE& node = graphNodes[index];
        const ScriptNodeTypeDescriptorUVE* descriptor = m_registry->FindNodeTypeUVE(node.typeId);
        ScriptGraphCanvasNodeSnapshotUVE nodeSnapshot{};
        nodeSnapshot.id = node.id;
        nodeSnapshot.typeId = node.typeId;
        nodeSnapshot.displayName = descriptor == nullptr ? node.typeId : descriptor->displayName;
        if (descriptor != nullptr) {
            nodeSnapshot.category = descriptor->category;
            nodeSnapshot.iconId = descriptor->iconId;
            nodeSnapshot.displayOrder = descriptor->displayOrder;
            nodeSnapshot.presentationFlags = descriptor->presentationFlags;
        }
        if (const LayoutEntryUVE* layout = FindLayoutUVE(node.id); layout != nullptr) {
            nodeSnapshot.position = layout->position;
        }
        nodeSnapshot.selected = ContainsIdUVE(m_selection, node.id);
        if (descriptor != nullptr) {
            nodeSnapshot.pins.reserve(descriptor->pins.size());
            for (const ScriptPinDescriptorUVE& pin : descriptor->pins) {
                std::optional<std::string> defaultValue = pin.defaultValue;
                if (const ScriptGraphCanvasPinDefaultOverrideUVE* overrideValue =
                        FindPinDefaultOverrideUVE(node.id, pin.name); overrideValue != nullptr) {
                    defaultValue = overrideValue->value;
                }
                nodeSnapshot.pins.push_back(ScriptGraphCanvasPinSnapshotUVE{
                    pin.name, pin.direction, pin.type, pin.role, std::move(defaultValue)});
            }
        }
        snapshot.nodes.push_back(std::move(nodeSnapshot));
    }
    snapshot.nodesTruncated = graphNodes.size() > nodeCount;

    const std::size_t linkCount = std::min(graphLinks.size(), kMaximumScriptGraphCanvasEntriesUVE);
    snapshot.links.reserve(linkCount);
    for (std::size_t index = 0U; index < linkCount; ++index) {
        snapshot.links.push_back(ScriptGraphCanvasLinkSnapshotUVE{graphLinks[index]});
    }
    snapshot.linksTruncated = graphLinks.size() > linkCount;

    const std::size_t selectionCount = std::min(m_selection.size(), kMaximumScriptGraphCanvasSelectionUVE);
    snapshot.selectedNodeIds.assign(m_selection.begin(), m_selection.begin() +
                                                   static_cast<std::ptrdiff_t>(selectionCount));

    const std::vector<ScriptNodeTypeDescriptorUVE> descriptors = m_registry->GetNodeTypeDescriptorsUVE();
    std::vector<std::string> palette;
    palette.reserve(descriptors.size());
    for (const ScriptNodeTypeDescriptorUVE& descriptor : descriptors) {
        palette.push_back(descriptor.typeId);
    }
    const std::size_t paletteCount = std::min(palette.size(), kMaximumScriptGraphCanvasEntriesUVE);
    snapshot.paletteNodeTypeIds.assign(palette.begin(), palette.begin() +
                                                    static_cast<std::ptrdiff_t>(paletteCount));
    snapshot.paletteDescriptors.reserve(paletteCount);
    for (std::size_t index = 0U; index < paletteCount; ++index) {
        const ScriptNodeTypeDescriptorUVE& descriptor = descriptors[index];
        ScriptGraphCanvasPaletteEntryUVE entry{};
        entry.typeId = descriptor.typeId;
        entry.displayName = descriptor.displayName;
        entry.category = descriptor.category;
        entry.iconId = descriptor.iconId;
        entry.displayOrder = descriptor.displayOrder;
        entry.presentationFlags = descriptor.presentationFlags;
        const std::size_t pinCount = std::min(descriptor.pins.size(), kMaximumScriptGraphCanvasEntriesUVE);
        entry.pins.reserve(pinCount);
        for (std::size_t pinIndex = 0U; pinIndex < pinCount; ++pinIndex) {
            const ScriptPinDescriptorUVE& pin = descriptor.pins[pinIndex];
            entry.pins.push_back(ScriptGraphCanvasPinSnapshotUVE{
                pin.name, pin.direction, pin.type, pin.role, pin.defaultValue});
        }
        snapshot.paletteDescriptors.push_back(std::move(entry));
    }
    snapshot.paletteTruncated = palette.size() > paletteCount;

    std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(*m_registry);
    const std::size_t diagnosticCount = std::min(diagnostics.size(), kMaximumScriptGraphCanvasEntriesUVE);
    snapshot.diagnostics.assign(diagnostics.begin(), diagnostics.begin() +
                                                   static_cast<std::ptrdiff_t>(diagnosticCount));
    snapshot.diagnosticsTruncated = diagnostics.size() > diagnosticCount;
    snapshot.dirty = m_dirty;
    snapshot.canUndo = !m_undo.empty();
    snapshot.canRedo = !m_redo.empty();
    return snapshot;
}

} // namespace UVE::Scripting
