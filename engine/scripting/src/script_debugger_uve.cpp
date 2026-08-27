// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_debugger_uve.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UVE::Scripting {

bool ScriptDebuggerUVE::AttachUVE(ScriptBytecodeProgramUVE program) {
    m_context.reset();
    return AttachProgramUVE(std::move(program));
}

bool ScriptDebuggerUVE::AttachWithContextUVE(ScriptBytecodeProgramUVE program,
                                             ScriptVmExecutionContextUVE context) {
    m_context = std::move(context);
    return AttachProgramUVE(std::move(program));
}

bool ScriptDebuggerUVE::AttachProgramUVE(ScriptBytecodeProgramUVE program) {
    m_trace.clear();
    m_traceTruncated = false;
    if ((program.version != ScriptBytecodeProgramUVE::kLegacyVersionUVE &&
         program.version != ScriptBytecodeProgramUVE::kConditionalJumpVersionUVE &&
         program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) ||
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
    m_sequenceContinuationTarget.reset();
    m_executionPowered = false;
    m_state = ScriptDebuggerStateUVE::Running;
    return true;
}

void ScriptDebuggerUVE::DetachUVE() noexcept {
    m_program = {};
    m_instructionIndex = 0U;
    m_executedInstructions = 0U;
    m_pauseReason.clear();
    m_trace.clear();
    m_traceTruncated = false;
    m_skipCurrentBreakpoint = false;
    m_sequenceContinuationTarget.reset();
    m_context.reset();
    m_executionPowered = false;
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
            MarkCompletedUVE();
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
        MarkCompletedUVE();
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
        MarkCompletedUVE();
    } else if (!ExecuteOneUVE()) {
        // ExecuteOneUVE publishes the fault state and reason.
    } else if (m_instructionIndex >= m_program.instructions.size()) {
        MarkCompletedUVE();
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
        MarkCompletedUVE();
        return true;
    }
    const std::size_t instructionIndex = m_instructionIndex;
    const ScriptIrInstructionUVE& instruction = m_program.instructions[instructionIndex];
    const ScriptIrInstructionKindUVE kind = instruction.kind;
    if (kind == ScriptIrInstructionKindUVE::FlowControlDispatch) {
        if (instruction.trueTargetInstructionIndex > m_program.instructions.size() ||
            instruction.falseTargetInstructionIndex > m_program.instructions.size() ||
            instruction.defaultTargetInstructionIndex > m_program.instructions.size()) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = "FlowControlDispatch target is outside the bytecode instruction range.";
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        }
        const auto failFlow = [&](std::string reason) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = std::move(reason);
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        };
        const bool isExecutionAction = instruction.sourcePinName == "In" &&
                                       instruction.nodeTypeId.rfind("flow.", 0U) != 0U;
        if (!isExecutionAction) {
            m_executionPowered = true;
        }
        const bool isContextFreeNode = instruction.nodeTypeId == "flow.return" || instruction.nodeTypeId == "flow.event";
        if (!isContextFreeNode && !m_context.has_value()) {
            return failFlow("FlowControlDispatch node requires an attached execution context.");
        }
        std::size_t target = instruction.defaultTargetInstructionIndex;
        std::string message;
        if (instruction.nodeTypeId == "flow.return") {
            target = m_program.instructions.size();
            message = "Return terminated execution.";
        } else if (instruction.nodeTypeId == "flow.event") {
            target = instruction.trueTargetInstructionIndex;
            message = "Event fired Then.";
        } else if (instruction.nodeTypeId == "flow.do_once") {
            if (instruction.sourcePinName == "Reset") {
                if (!m_context->ResetDoOnceLatchUVE(instruction.sourceNodeId)) {
                    return failFlow("Do Once could not reset its bounded latch state.");
                }
                target = m_program.instructions.size();
                message = "Do Once latch reset.";
            } else if (m_context->TryConsumeDoOnceLatchUVE(instruction.sourceNodeId)) {
                target = instruction.trueTargetInstructionIndex;
                message = "Do Once fired Then.";
            } else {
                target = instruction.falseTargetInstructionIndex;
                message = "Do Once suppressed a repeated execution.";
            }
        } else if (instruction.nodeTypeId == "flow.gate") {
            if (instruction.sourcePinName == "Open" || instruction.sourcePinName == "Close") {
                const bool open = instruction.sourcePinName == "Open";
                if (!m_context->SetGateStateUVE(instruction.sourceNodeId, open)) {
                    return failFlow("Gate exceeded its bounded state capacity.");
                }
                target = m_program.instructions.size();
                message = open ? "Gate opened." : "Gate closed.";
            } else {
                const std::optional<bool> open = m_context->FindGateStateUVE(instruction.sourceNodeId);
                target = open.value_or(false) ? instruction.trueTargetInstructionIndex
                                              : instruction.falseTargetInstructionIndex;
                message = open.value_or(false) ? "Gate routed through Exit." : "Gate suppressed a closed input.";
            }
        } else if (instruction.nodeTypeId == "flow.switch") {
            const auto valueBinding = m_context->FindInputUVE(instruction.sourceNodeId, "Value");
            const float* value = valueBinding.has_value() ? std::get_if<float>(&*valueBinding) : nullptr;
            if (value == nullptr || !std::isfinite(*value)) {
                target = instruction.defaultTargetInstructionIndex;
                message = "Switch selected Default because Value was unavailable or non-finite.";
            } else if (std::fabs(*value) <= 1.0e-6F) {
                target = instruction.trueTargetInstructionIndex;
                message = "Switch selected Case0.";
            } else {
                target = instruction.falseTargetInstructionIndex;
                message = "Switch selected Case1.";
            }
        } else if (instruction.nodeTypeId == "flow.loop" || instruction.nodeTypeId == "flow.for_loop") {
            const auto countBinding = m_context->FindInputUVE(instruction.sourceNodeId, "Count");
            const float* count = countBinding.has_value() ? std::get_if<float>(&*countBinding) : nullptr;
            if (count == nullptr || !std::isfinite(*count) || *count < 0.0F ||
                *count > static_cast<float>(ScriptVmExecutionContextUVE::kMaximumLoopIterationsUVE) ||
                std::floor(*count) != *count) {
                return failFlow("Loop Count must be a finite non-negative integer within the bounded iteration limit.");
            }
            if (!m_context->InitializeLoopStateUVE(instruction.sourceNodeId)) {
                return failFlow("Loop exceeded its bounded state capacity.");
            }
            const ScriptVmLoopStateUVE state = m_context->FindLoopStateUVE(instruction.sourceNodeId).value_or(
                ScriptVmLoopStateUVE{instruction.sourceNodeId, 0U, false});
            const std::uint32_t countValue = static_cast<std::uint32_t>(*count);
            if (countValue == 0U || (state.active && state.iteration >= countValue)) {
                if (!m_context->SetLoopStateUVE(instruction.sourceNodeId, 0U, false)) {
                    return failFlow("Loop could not reset its bounded state.");
                }
                target = instruction.falseTargetInstructionIndex;
                message = "Loop completed.";
            } else {
                if (instruction.nodeTypeId == "flow.for_loop" &&
                    !m_context->SetOutputUVE(instruction.sourceNodeId, "Index", static_cast<float>(state.iteration))) {
                    return failFlow("For Loop could not publish its bounded Index output.");
                }
                if (!m_context->SetLoopStateUVE(instruction.sourceNodeId, state.iteration + 1U, true)) {
                    return failFlow("Loop could not advance its bounded iteration state.");
                }
                target = instruction.trueTargetInstructionIndex;
                message = instruction.nodeTypeId == "flow.for_loop" ? "For Loop dispatched Body." : "Loop dispatched Body.";
            }
        } else if (instruction.nodeTypeId == "flow.while_loop") {
            const auto conditionBinding = m_context->FindInputUVE(instruction.sourceNodeId, "Condition");
            const bool* condition = conditionBinding.has_value() ? std::get_if<bool>(&*conditionBinding) : nullptr;
            if (condition == nullptr) {
                return failFlow("While Loop requires a Boolean Condition input.");
            }
            if (!m_context->InitializeLoopStateUVE(instruction.sourceNodeId)) {
                return failFlow("While Loop exceeded its bounded state capacity.");
            }
            const ScriptVmLoopStateUVE state = m_context->FindLoopStateUVE(instruction.sourceNodeId).value_or(
                ScriptVmLoopStateUVE{instruction.sourceNodeId, 0U, false});
            if (!*condition) {
                if (!m_context->SetLoopStateUVE(instruction.sourceNodeId, 0U, false)) {
                    return failFlow("While Loop could not reset its bounded state.");
                }
                target = instruction.falseTargetInstructionIndex;
                message = "While Loop completed because Condition was false.";
            } else if (state.iteration >= ScriptVmExecutionContextUVE::kMaximumLoopIterationsUVE) {
                return failFlow("While Loop exceeded its bounded iteration limit.");
            } else {
                if (!m_context->SetLoopStateUVE(instruction.sourceNodeId, state.iteration + 1U, true)) {
                    return failFlow("While Loop could not advance its bounded iteration state.");
                }
                target = instruction.trueTargetInstructionIndex;
                message = "While Loop dispatched Body.";
            }
        } else if (instruction.nodeTypeId == "flow.delay") {
            const auto framesBinding = m_context->FindInputUVE(instruction.sourceNodeId, "Frames");
            const float* frames = framesBinding.has_value() ? std::get_if<float>(&*framesBinding) : nullptr;
            if (frames == nullptr || !std::isfinite(*frames) || *frames < 0.0F ||
                *frames > static_cast<float>(ScriptVmExecutionContextUVE::kMaximumDelayFramesUVE) ||
                std::floor(*frames) != *frames) {
                return failFlow("Delay Frames must be a finite non-negative integer within the bounded frame limit.");
            }
            if (!m_context->InitializeDelayStateUVE(instruction.sourceNodeId)) {
                return failFlow("Delay exceeded its bounded state capacity.");
            }
            const ScriptVmDelayStateUVE state = m_context->FindDelayStateUVE(instruction.sourceNodeId).value_or(
                ScriptVmDelayStateUVE{instruction.sourceNodeId, 0U, false});
            const std::uint32_t frameCount = static_cast<std::uint32_t>(*frames);
            if (!state.armed && frameCount == 0U) {
                target = instruction.trueTargetInstructionIndex;
                message = "Delay dispatched Then immediately.";
            } else if (!state.armed) {
                if (!m_context->SetDelayStateUVE(instruction.sourceNodeId, frameCount, true)) {
                    return failFlow("Delay could not arm its bounded frame state.");
                }
                target = instructionIndex;
                message = "Delay yielded until the next debugger step.";
            } else if (state.remainingFrames > 1U) {
                if (!m_context->SetDelayStateUVE(instruction.sourceNodeId, state.remainingFrames - 1U, true)) {
                    return failFlow("Delay could not advance its bounded frame state.");
                }
                target = instructionIndex;
                message = "Delay yielded while its bounded frame state remained active.";
            } else {
                if (!m_context->SetDelayStateUVE(instruction.sourceNodeId, 0U, false)) {
                    return failFlow("Delay could not complete its bounded frame state.");
                }
                target = instruction.trueTargetInstructionIndex;
                message = "Delay dispatched Then.";
            }
        } else if (isExecutionAction) {
            if (!m_executionPowered) {
                target = m_program.instructions.size();
                message = "Action skipped because no execution wire powered this node.";
            } else {
                target = instruction.trueTargetInstructionIndex;
                message = "Action executed and dispatched Then.";
            }
        } else {
            return failFlow("Unknown FlowControlDispatch node type.");
        }
        m_instructionIndex = target;
        ++m_executedInstructions;
        AppendTraceEventUVE({(!isExecutionAction || m_executionPowered)
                                 ? ScriptVmTraceEventKindUVE::NodeExecuted
                                 : ScriptVmTraceEventKindUVE::NodeSkipped,
                             Scene::kInvalidEntityUVE, instructionIndex, instruction.sourceNodeId,
                             instruction.targetNodeId, instruction.nodeTypeId, std::move(message)});
        return true;
    }
    if (kind == ScriptIrInstructionKindUVE::SequenceDispatch) {
        if (instruction.firstTargetInstructionIndex > m_program.instructions.size() ||
            instruction.secondTargetInstructionIndex > m_program.instructions.size()) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = "SequenceDispatch target is outside the bytecode instruction range.";
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        }
        ++m_executedInstructions;
        AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                             Scene::kInvalidEntityUVE, instructionIndex, instruction.sourceNodeId,
                             instruction.targetNodeId, instruction.nodeTypeId,
                             "SequenceDispatch selected ordered execution targets."});
        if (instruction.firstTargetInstructionIndex == m_program.instructions.size()) {
            m_instructionIndex = instruction.secondTargetInstructionIndex;
        } else {
            m_sequenceContinuationTarget = instruction.secondTargetInstructionIndex;
            m_instructionIndex = instruction.firstTargetInstructionIndex;
        }
        return true;
    }
    if (kind == ScriptIrInstructionKindUVE::ConditionalJump) {
        if (instruction.trueTargetInstructionIndex > m_program.instructions.size() ||
            instruction.falseTargetInstructionIndex > m_program.instructions.size()) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = "ConditionalJump target is outside the bytecode instruction range.";
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        }
        if (!m_context.has_value()) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = "ConditionalJump requires an attached execution context.";
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        }
        const std::string& conditionPin = instruction.sourcePinName.empty()
            ? std::string{"Condition"}
            : instruction.sourcePinName;
        const auto conditionBinding = m_context->FindInputUVE(instruction.sourceNodeId, conditionPin);
        const bool* condition = conditionBinding.has_value() ? std::get_if<bool>(&*conditionBinding) : nullptr;
        if (condition == nullptr) {
            m_state = ScriptDebuggerStateUVE::Faulted;
            m_pauseReason = "ConditionalJump requires a Boolean condition input.";
            AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                 instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                                 instruction.nodeTypeId, m_pauseReason});
            return false;
        }
        m_instructionIndex = *condition ? instruction.trueTargetInstructionIndex
                                        : instruction.falseTargetInstructionIndex;
        ++m_executedInstructions;
        AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                             Scene::kInvalidEntityUVE, instructionIndex, instruction.sourceNodeId,
                             instruction.targetNodeId, instruction.nodeTypeId,
                             *condition ? "ConditionalJump evaluated true."
                                        : "ConditionalJump evaluated false."});
        return true;
    }
    if (kind != ScriptIrInstructionKindUVE::ExecuteNode && kind != ScriptIrInstructionKindUVE::TransferValue) {
        m_state = ScriptDebuggerStateUVE::Faulted;
        m_pauseReason = "Invalid instruction kind.";
        AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                             instructionIndex, instruction.sourceNodeId, instruction.targetNodeId,
                             instruction.nodeTypeId, m_pauseReason});
        return false;
    }
    if (m_sequenceContinuationTarget.has_value()) {
        m_instructionIndex = *m_sequenceContinuationTarget;
        m_sequenceContinuationTarget.reset();
    } else {
        ++m_instructionIndex;
    }
    ++m_executedInstructions;
    AppendTraceEventUVE({kind == ScriptIrInstructionKindUVE::ExecuteNode
                             ? ScriptVmTraceEventKindUVE::NodeExecuted
                             : (instruction.isStagedTransfer
                                    ? ScriptVmTraceEventKindUVE::StagedValueTransferred
                                    : ScriptVmTraceEventKindUVE::ValueTransferred),
                         Scene::kInvalidEntityUVE, instructionIndex, instruction.sourceNodeId,
                         instruction.targetNodeId, instruction.nodeTypeId, {}});
    return true;
}

ScriptDebuggerSnapshotUVE ScriptDebuggerUVE::MakeSnapshotUVE() const {
    std::vector<std::uint32_t> breakpointNodeIds(m_breakpoints.cbegin(), m_breakpoints.cend());
    std::sort(breakpointNodeIds.begin(), breakpointNodeIds.end());
    const std::uint32_t sourceNodeId = m_instructionIndex < m_program.instructions.size()
        ? m_program.instructions[m_instructionIndex].sourceNodeId
        : 0U;
    return {m_state, m_instructionIndex, sourceNodeId, m_executedInstructions, m_pauseReason,
            std::move(breakpointNodeIds), m_trace, m_traceTruncated};
}

void ScriptDebuggerUVE::AppendTraceEventUVE(ScriptVmTraceEventUVE event) {
    if (m_trace.size() >= kMaximumTraceEventsUVE) {
        m_traceTruncated = true;
        return;
    }
    if (event.nodeTypeId.size() > ScriptVmExecutionResultUVE::kMaximumTraceMessageBytesUVE) {
        event.nodeTypeId.resize(ScriptVmExecutionResultUVE::kMaximumTraceMessageBytesUVE);
    }
    if (event.message.size() > ScriptVmExecutionResultUVE::kMaximumTraceMessageBytesUVE) {
        event.message.resize(ScriptVmExecutionResultUVE::kMaximumTraceMessageBytesUVE);
    }
    m_trace.push_back(std::move(event));
}

void ScriptDebuggerUVE::MarkCompletedUVE() {
    if (m_state == ScriptDebuggerStateUVE::Completed) {
        return;
    }
    m_state = ScriptDebuggerStateUVE::Completed;
    m_pauseReason = "Program completed.";
    AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Completed, Scene::kInvalidEntityUVE,
                         m_instructionIndex, 0U, 0U, {}, {}});
}

} // namespace UVE::Scripting
