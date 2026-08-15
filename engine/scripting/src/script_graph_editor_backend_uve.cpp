// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_graph_editor_backend_uve.h"

#include <utility>

namespace UVE::Scripting {

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::ApplyEditUVE(ScriptGraphUVE candidate) {
    if (m_undo.size() >= kMaximumHistoryUVE) {
        m_undo.erase(m_undo.begin());
    }
    m_undo.push_back(m_graph);
    m_graph = std::move(candidate);
    m_redo.clear();
    return {ScriptGraphCommandCodeUVE::Applied, "Graph edit applied."};
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::AddNodeUVE(ScriptNodeUVE node) {
    ScriptGraphUVE candidate = m_graph;
    if (!candidate.AddNodeUVE(std::move(node))) {
        return {ScriptGraphCommandCodeUVE::Rejected, "Node edit was rejected."};
    }
    return ApplyEditUVE(std::move(candidate));
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::RemoveNodeUVE(const std::uint32_t nodeId) {
    ScriptGraphUVE candidate = m_graph;
    if (!candidate.RemoveNodeUVE(nodeId)) {
        return {ScriptGraphCommandCodeUVE::Rejected, "Node was not present."};
    }
    return ApplyEditUVE(std::move(candidate));
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::AddLinkUVE(ScriptLinkUVE link) {
    ScriptGraphUVE candidate = m_graph;
    if (!candidate.AddLinkUVE(std::move(link))) {
        return {ScriptGraphCommandCodeUVE::Rejected, "Link edit was rejected."};
    }
    return ApplyEditUVE(std::move(candidate));
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::RemoveLinkUVE(const ScriptLinkUVE& link) {
    ScriptGraphUVE candidate = m_graph;
    if (!candidate.RemoveLinkUVE(link)) {
        return {ScriptGraphCommandCodeUVE::Rejected, "Link was not present."};
    }
    return ApplyEditUVE(std::move(candidate));
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::UndoUVE() {
    if (m_undo.empty()) {
        return {ScriptGraphCommandCodeUVE::NoHistory, "No undo history is available."};
    }
    if (m_redo.size() >= kMaximumHistoryUVE) {
        m_redo.erase(m_redo.begin());
    }
    m_redo.push_back(m_graph);
    m_graph = std::move(m_undo.back());
    m_undo.pop_back();
    return {ScriptGraphCommandCodeUVE::Applied, "Graph edit undone."};
}

ScriptGraphCommandResultUVE ScriptGraphEditorBackendUVE::RedoUVE() {
    if (m_redo.empty()) {
        return {ScriptGraphCommandCodeUVE::NoHistory, "No redo history is available."};
    }
    if (m_undo.size() >= kMaximumHistoryUVE) {
        m_undo.erase(m_undo.begin());
    }
    m_undo.push_back(m_graph);
    m_graph = std::move(m_redo.back());
    m_redo.pop_back();
    return {ScriptGraphCommandCodeUVE::Applied, "Graph edit redone."};
}

const ScriptGraphUVE& ScriptGraphEditorBackendUVE::GetGraphUVE() const noexcept {
    return m_graph;
}

std::size_t ScriptGraphEditorBackendUVE::GetUndoCountUVE() const noexcept {
    return m_undo.size();
}

std::size_t ScriptGraphEditorBackendUVE::GetRedoCountUVE() const noexcept {
    return m_redo.size();
}

} // namespace UVE::Scripting
