// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_debugger_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Scripting {

bool ScriptDebuggerUVE::AttachUVE(ScriptBytecodeProgramUVE program) {
    if (program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE ||
        program.instructions.size() > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        m_state = ScriptDebuggerStateUVE::Faulted;
        m_pauseReason = "The debugger rejected an unsupported or oversized bytecode program.";
        return false;
    }
    m_program = std::move(program);
    m_instructionIndex = 0U;
    m_executedInstructions = 0U;
    m_pauseReason.clear();
    m_skipCurrentBreakpoint = false;
    m_state = ScriptDebuggerStateUVE::Running;
    return true;
}

void ScriptDebuggerUVE::DetachUVE() noexcept {
    m_program = {};
    m_instructionIndex = 0U;
    m_executedInstructions = 0U;
    m_pauseReason.clear();
    m_skipCurrentBreakpoint = false;
    m_state = ScriptDebuggerStateUVE::Detached;
}

bool ScriptDebuggerUVE::SetBreakpointUVE(const std::uint32_t sourceNodeId, const bool enabled) {
    if (sourceNodeId == 0U) {
        return false;
    }
    if (!enabled) {
        m_breakpoints.erase(sourceNodeId);
        return true;
    }
    if (!m_breakpoints.contains(sourceNodeId) && m_breakpoints.size() >= kMaximumBreakpointsUVE) {
        return false;
    }
    m_breakpoints.insert(sourceNodeId);
    return true;
}

ScriptDebuggerSnapshotUVE ScriptDebuggerUVE::ContinueUVE(const std::size_t instructionBudget) {
    if (m_state == ScriptDebuggerStateUVE::Detached || m_state == ScriptDebuggerStateUVE::Faulted ||
        m_state == ScriptDebuggerStateUVE::Completed) {
        return MakeSnapshotUVE();
    }
    const bool wasPaused = m_state == ScriptDebuggerStateUVE::Paused;
    m_state = ScriptDebuggerStateUVE::Running;
    m_skipCurrentBreakpoint = wasPaused;
    const std::size_t budget = std::min(instructionBudget, kMaximumExecutionBudgetUVE);
    for (std::size_t executed = 0U; executed < budget; ++executed) {
        if (m_instructionIndex >= m_program.instructions.size()) {
            m_state = ScriptDebuggerStateUVE::Completed;
            m_pauseReason = "Program completed.";
            break;
        }
        const std::uint32_t sourceNodeId = m_program.instructions[m_instructionIndex].sourceNodeId;
        if (IsBreakpointUVE(sourceNodeId) && !m_skipCurrentBreakpoint) {
            m_state = ScriptDebuggerStateUVE::Paused;
            m_pauseReason = "Breakpoint reached.";
            break;
        }
        m_skipCurrentBreakpoint = false;
        if (!ExecuteOneUVE()) {
            break;
        }
    }
    if (m_state == ScriptDebuggerStateUVE::Running && m_instructionIndex >= m_program.instructions.size()) {
        m_state = ScriptDebuggerStateUVE::Completed;
        m_pauseReason = "Program completed.";
    }
    return MakeSnapshotUVE();
}

ScriptDebuggerSnapshotUVE ScriptDebuggerUVE::StepUVE() {
    if (m_state == ScriptDebuggerStateUVE::Detached || m_state == ScriptDebuggerStateUVE::Faulted ||
        m_state == ScriptDebuggerStateUVE::Completed) {
        return MakeSnapshotUVE();
    }
    m_state = ScriptDebuggerStateUVE::Running;
    m_skipCurrentBreakpoint = true;
    if (m_instructionIndex >= m_program.instructions.size()) {
        m_state = ScriptDebuggerStateUVE::Completed;
        m_pauseReason = "Program completed.";
    } else if (!ExecuteOneUVE()) {
        // ExecuteOneUVE publishes the fault state and reason.
    } else if (m_instructionIndex >= m_program.instructions.size()) {
        m_state = ScriptDebuggerStateUVE::Completed;
        m_pauseReason = "Program completed.";
    } else {
        m_state = ScriptDebuggerStateUVE::Paused;
        m_pauseReason = "Stepped one instruction.";
    }
    m_skipCurrentBreakpoint = false;
    return MakeSnapshotUVE();
}

ScriptDebuggerSnapshotUVE ScriptDebuggerUVE::GetSnapshotUVE() const {
    return MakeSnapshotUVE();
}

bool ScriptDebuggerUVE::IsBreakpointUVE(const std::uint32_t sourceNodeId) const noexcept {
    return sourceNodeId != 0U && m_breakpoints.contains(sourceNodeId);
}

bool ScriptDebuggerUVE::ExecuteOneUVE() {
    if (m_instructionIndex >= m_program.instructions.size()) {
        m_state = ScriptDebuggerStateUVE::Completed;
        m_pauseReason = "Program completed.";
        return true;
    }
    const ScriptIrInstructionKindUVE kind = m_program.instructions[m_instructionIndex].kind;
    if (kind != ScriptIrInstructionKindUVE::ExecuteNode && kind != ScriptIrInstructionKindUVE::TransferValue) {
        m_state = ScriptDebuggerStateUVE::Faulted;
        m_pauseReason = "Invalid instruction kind.";
        return false;
    }
    ++m_instructionIndex;
    ++m_executedInstructions;
    return true;
}

ScriptDebuggerSnapshotUVE ScriptDebuggerUVE::MakeSnapshotUVE() const {
    std::vector<std::uint32_t> breakpointNodeIds(m_breakpoints.cbegin(), m_breakpoints.cend());
    std::sort(breakpointNodeIds.begin(), breakpointNodeIds.end());
    const std::uint32_t sourceNodeId = m_instructionIndex < m_program.instructions.size()
        ? m_program.instructions[m_instructionIndex].sourceNodeId
        : 0U;
    return {m_state, m_instructionIndex, sourceNodeId, m_executedInstructions, m_pauseReason,
            std::move(breakpointNodeIds)};
}

} // namespace UVE::Scripting
