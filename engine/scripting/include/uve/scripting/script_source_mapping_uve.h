// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scripting/script_debugger_uve.h"
#include "uve/scripting/script_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

inline constexpr std::size_t kMaximumScriptWatchesUVE = 64U;
inline constexpr std::size_t kMaximumScriptWatchValueBytesUVE = 256U;

struct ScriptWatchValueUVE final {
    std::string watchId;
    std::string expression;
    std::string value;
    bool valid = false;

    [[nodiscard]] bool operator==(const ScriptWatchValueUVE&) const = default;
};

struct ScriptSourceMappingEntryUVE final {
    std::uint32_t nodeId = 0U;
    std::string nodeTypeId;
    std::string sourceLabel;
    bool hasBreakpoint = false;
    bool activeBreakpoint = false;

    [[nodiscard]] bool operator==(const ScriptSourceMappingEntryUVE&) const = default;
};

struct ScriptDebugPresentationSnapshotUVE final {
    bool available = false;
    ScriptDebuggerStateUVE state = ScriptDebuggerStateUVE::Detached;
    std::size_t instructionIndex = 0U;
    std::uint32_t activeNodeId = 0U;
    std::string sourceLabel;
    std::vector<ScriptSourceMappingEntryUVE> entries;
    std::vector<ScriptWatchValueUVE> watches;
    std::vector<ScriptVmTraceEventUVE> trace;
    bool traceTruncated = false;

    [[nodiscard]] bool operator==(const ScriptDebugPresentationSnapshotUVE&) const = default;
};

class ScriptSourceMappingUVE final {
public:
    explicit ScriptSourceMappingUVE(const ScriptGraphUVE& graph) noexcept;
    ScriptSourceMappingUVE(const ScriptSourceMappingUVE&) = delete;
    ScriptSourceMappingUVE& operator=(const ScriptSourceMappingUVE&) = delete;

    [[nodiscard]] bool AddWatchUVE(std::string watchId, std::string expression);
    [[nodiscard]] bool RemoveWatchUVE(std::string watchId);
    [[nodiscard]] ScriptDebugPresentationSnapshotUVE BuildPresentationUVE(
        const ScriptDebuggerUVE& debugger) const;

private:
    const ScriptGraphUVE* m_graph = nullptr;
    std::vector<std::pair<std::string, std::string>> m_watches;
};

} // namespace UVE::Scripting
