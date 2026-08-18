// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_vm_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace UVE::Scripting {

enum class ScriptDebuggerStateUVE : std::uint8_t {
    Detached = 0,
    Running,
    Paused,
    Completed,
    Faulted,
};

struct ScriptDebuggerSnapshotUVE final {
    ScriptDebuggerStateUVE state = ScriptDebuggerStateUVE::Detached;
    std::size_t instructionIndex = 0U;
    std::uint32_t sourceNodeId = 0U;
    std::size_t executedInstructions = 0U;
    std::string pauseReason;
    std::vector<std::uint32_t> breakpointNodeIds;
    std::vector<ScriptVmTraceEventUVE> trace;
    bool traceTruncated = false;
};

class ScriptDebuggerUVE final {
public:
    static constexpr std::size_t kMaximumBreakpointsUVE = 256U;
    static constexpr std::size_t kMaximumExecutionBudgetUVE = 4096U;
    static constexpr std::size_t kMaximumTraceEventsUVE = ScriptVmExecutionResultUVE::kMaximumTraceEventsUVE;

    ScriptDebuggerUVE() = default;
    ScriptDebuggerUVE(const ScriptDebuggerUVE&) = delete;
    ScriptDebuggerUVE& operator=(const ScriptDebuggerUVE&) = delete;

    [[nodiscard]] bool AttachUVE(ScriptBytecodeProgramUVE program);
    void DetachUVE() noexcept;
    [[nodiscard]] bool SetBreakpointUVE(std::uint32_t sourceNodeId, bool enabled);
    [[nodiscard]] ScriptDebuggerSnapshotUVE ContinueUVE(std::size_t instructionBudget = kMaximumExecutionBudgetUVE);
    [[nodiscard]] ScriptDebuggerSnapshotUVE StepUVE();
    [[nodiscard]] ScriptDebuggerSnapshotUVE GetSnapshotUVE() const;

private:
    [[nodiscard]] bool IsBreakpointUVE(std::uint32_t sourceNodeId) const noexcept;
    [[nodiscard]] ScriptDebuggerSnapshotUVE MakeSnapshotUVE() const;
    [[nodiscard]] bool ExecuteOneUVE();
    void AppendTraceEventUVE(ScriptVmTraceEventUVE event);
    void MarkCompletedUVE();

    ScriptBytecodeProgramUVE m_program;
    ScriptDebuggerStateUVE m_state = ScriptDebuggerStateUVE::Detached;
    std::size_t m_instructionIndex = 0U;
    std::size_t m_executedInstructions = 0U;
    std::string m_pauseReason;
    std::unordered_set<std::uint32_t> m_breakpoints;
    std::vector<ScriptVmTraceEventUVE> m_trace;
    bool m_traceTruncated = false;
    bool m_skipCurrentBreakpoint = false;
};

} // namespace UVE::Scripting
