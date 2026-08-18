// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_source_mapping_uve.h"

#include <algorithm>

namespace UVE::Scripting {

ScriptSourceMappingUVE::ScriptSourceMappingUVE(const ScriptGraphUVE& graph) noexcept
    : m_graph(&graph) {}

bool ScriptSourceMappingUVE::AddWatchUVE(std::string watchId, std::string expression) {
    if (watchId.empty() || expression.empty() || m_watches.size() >= kMaximumScriptWatchesUVE) {
        return false;
    }
    for (const auto& pair : m_watches) {
        if (pair.first == watchId) {
            return false;
        }
    }
    m_watches.emplace_back(std::move(watchId), std::move(expression));
    return true;
}

bool ScriptSourceMappingUVE::RemoveWatchUVE(std::string watchId) {
    const auto it = std::remove_if(m_watches.begin(), m_watches.end(),
                                   [&watchId](const auto& pair) { return pair.first == watchId; });
    if (it == m_watches.end()) {
        return false;
    }
    m_watches.erase(it, m_watches.end());
    return true;
}

ScriptDebugPresentationSnapshotUVE ScriptSourceMappingUVE::BuildPresentationUVE(
    const ScriptDebuggerUVE& debugger) const {
    ScriptDebugPresentationSnapshotUVE snapshot;
    if (m_graph == nullptr) {
        snapshot.available = false;
        return snapshot;
    }

    const ScriptDebuggerSnapshotUVE debuggerSnapshot = debugger.GetSnapshotUVE();
    snapshot.available = true;
    snapshot.state = debuggerSnapshot.state;
    snapshot.instructionIndex = debuggerSnapshot.instructionIndex;
    snapshot.activeNodeId = debuggerSnapshot.sourceNodeId;
    snapshot.trace = debuggerSnapshot.trace;
    snapshot.traceTruncated = debuggerSnapshot.traceTruncated;

    const auto& nodes = m_graph->GetNodesUVE();
    snapshot.entries.reserve(nodes.size());

    for (const ScriptNodeUVE& node : nodes) {
        ScriptSourceMappingEntryUVE entry;
        entry.nodeId = node.id;
        entry.nodeTypeId = node.typeId;
        entry.sourceLabel = "Node #" + std::to_string(node.id) + " [" + node.typeId + "]";
        
        entry.hasBreakpoint = false;
        for (const std::uint32_t bpId : debuggerSnapshot.breakpointNodeIds) {
            if (bpId == node.id) {
                entry.hasBreakpoint = true;
                break;
            }
        }
        entry.activeBreakpoint = (debuggerSnapshot.sourceNodeId == node.id &&
                                  debuggerSnapshot.state == ScriptDebuggerStateUVE::Paused);
        if (entry.activeBreakpoint) {
            snapshot.sourceLabel = entry.sourceLabel;
        }

        snapshot.entries.push_back(entry);
    }

    if (snapshot.sourceLabel.empty() && !nodes.empty()) {
        snapshot.sourceLabel = "Graph [" + std::to_string(nodes.size()) + " nodes]";
    }

    snapshot.watches.reserve(m_watches.size());
    for (const auto& [watchId, expression] : m_watches) {
        ScriptWatchValueUVE watchValue;
        watchValue.watchId = watchId;
        watchValue.expression = expression;
        watchValue.value = "<evaluated: " + expression + ">";
        watchValue.valid = true;
        snapshot.watches.push_back(watchValue);
    }

    return snapshot;
}

} // namespace UVE::Scripting
