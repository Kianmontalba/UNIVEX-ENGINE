// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptGraphCommandCodeUVE : std::uint8_t {
    Applied = 0,
    Rejected,
    NoHistory,
};

struct ScriptGraphCommandResultUVE final {
    ScriptGraphCommandCodeUVE code = ScriptGraphCommandCodeUVE::Rejected;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return code == ScriptGraphCommandCodeUVE::Applied;
    }
};

class ScriptGraphEditorBackendUVE final {
public:
    static constexpr std::size_t kMaximumHistoryUVE = 128U;

    ScriptGraphEditorBackendUVE() = default;
    ScriptGraphEditorBackendUVE(const ScriptGraphEditorBackendUVE&) = delete;
    ScriptGraphEditorBackendUVE& operator=(const ScriptGraphEditorBackendUVE&) = delete;

    [[nodiscard]] ScriptGraphCommandResultUVE AddNodeUVE(ScriptNodeUVE node);
    [[nodiscard]] ScriptGraphCommandResultUVE RemoveNodeUVE(std::uint32_t nodeId);
    [[nodiscard]] ScriptGraphCommandResultUVE AddLinkUVE(ScriptLinkUVE link);
    [[nodiscard]] ScriptGraphCommandResultUVE RemoveLinkUVE(const ScriptLinkUVE& link);
    [[nodiscard]] ScriptGraphCommandResultUVE UndoUVE();
    [[nodiscard]] ScriptGraphCommandResultUVE RedoUVE();

    [[nodiscard]] const ScriptGraphUVE& GetGraphUVE() const noexcept;
    [[nodiscard]] std::size_t GetUndoCountUVE() const noexcept;
    [[nodiscard]] std::size_t GetRedoCountUVE() const noexcept;

private:
    [[nodiscard]] ScriptGraphCommandResultUVE ApplyEditUVE(ScriptGraphUVE candidate);

    ScriptGraphUVE m_graph;
    std::vector<ScriptGraphUVE> m_undo;
    std::vector<ScriptGraphUVE> m_redo;
};

} // namespace UVE::Scripting
