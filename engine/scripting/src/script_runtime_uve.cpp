// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_runtime_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsFiniteScriptVector3UVE(const ScriptVector3ValueUVE& value) noexcept {
    return std::isfinite(value.value.x) && std::isfinite(value.value.y) &&
           std::isfinite(value.value.z);
}

[[nodiscard]] bool IsFiniteScriptVmValueUVE(const ScriptVmValueUVE& value) noexcept {
    return std::visit(
        [](const auto& typedValue) noexcept {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, float>) {
                return std::isfinite(typedValue);
            } else if constexpr (std::is_same_v<ValueType, bool>) {
                return true;
            } else {
                return IsFiniteScriptVector3UVE(typedValue);
            }
        },
        value);
}

[[nodiscard]] bool IsValidScriptVmBindingsUVE(const ScriptVmExecutionContextUVE& context) noexcept {
    const auto validBinding = [](const ScriptVmValueBindingUVE& binding) noexcept {
        return binding.nodeId != 0U && !binding.pinName.empty() && IsFiniteScriptVmValueUVE(binding.value);
    };
    return context.inputs.size() <= ScriptVmExecutionContextUVE::kMaximumBindingsUVE &&
           context.outputs.size() <= ScriptVmExecutionContextUVE::kMaximumBindingsUVE &&
           std::all_of(context.inputs.begin(), context.inputs.end(), validBinding) &&
           std::all_of(context.outputs.begin(), context.outputs.end(), validBinding);
}

[[nodiscard]] std::vector<ScriptBytecodeDiagnosticUVE> ValidateRuntimeProgramUVE(
    const ScriptBytecodeProgramUVE& program) {
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    if (program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) {
        diagnostics.push_back({ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion, 0U,
                               "Runtime program version is unsupported."});
        return diagnostics;
    }
    if (program.instructions.size() > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        diagnostics.push_back({ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded,
                               ScriptBytecodeProgramUVE::kMaximumInstructionsUVE,
                               "Runtime program exceeds the instruction limit."});
        return diagnostics;
    }
    for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
        const ScriptIrInstructionKindUVE kind = program.instructions[index].kind;
        if (kind != ScriptIrInstructionKindUVE::ExecuteNode && kind != ScriptIrInstructionKindUVE::TransferValue) {
            diagnostics.push_back({ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, index,
                                   "Runtime program contains an unsupported instruction kind."});
            return diagnostics;
        }
    }
    return diagnostics;
}

} // namespace

ScriptRuntimeAttachResultUVE ScriptRuntimeUVE::AttachDetailedUVE(const Scene::EntityUVE entity,
                                                                    ScriptBytecodeProgramUVE program) {
    if (entity == Scene::kInvalidEntityUVE) {
        return {ScriptRuntimeAttachCodeUVE::InvalidEntity, {},
                "Runtime attachment rejected because the entity handle is invalid."};
    }
    const std::vector<ScriptBytecodeDiagnosticUVE> diagnostics = ValidateRuntimeProgramUVE(program);
    if (!diagnostics.empty()) {
        return {ScriptRuntimeAttachCodeUVE::InvalidProgram, diagnostics,
                "Runtime attachment rejected because the bytecode program is invalid."};
    }
    if (m_instances.size() >= kMaximumInstancesUVE) {
        return {ScriptRuntimeAttachCodeUVE::CapacityExceeded, {},
                "Runtime attachment rejected because instance capacity is exhausted."};
    }
    if (m_instances.contains(entity)) {
        return {ScriptRuntimeAttachCodeUVE::DuplicateInstance, {},
                "Runtime attachment rejected because the entity already has an active instance."};
    }
    m_instances.emplace(entity, ScriptRuntimeInstanceUVE{entity, std::move(program), {}, 1U, true});
    return {ScriptRuntimeAttachCodeUVE::Accepted, {}, "Runtime instance attached."};
}

bool ScriptRuntimeUVE::AttachUVE(const Scene::EntityUVE entity, ScriptBytecodeProgramUVE program) {
    return AttachDetailedUVE(entity, std::move(program)).IsAcceptedUVE();
}

ScriptRuntimeReloadResultUVE ScriptRuntimeUVE::ReloadUVE(const Scene::EntityUVE entity,
                                                          ScriptBytecodeProgramUVE program) {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return {ScriptRuntimeReloadCodeUVE::NoActiveInstance, 0U, false, false, {},
                "Runtime reload rejected because no active instance exists."};
    }
    const std::vector<ScriptBytecodeDiagnosticUVE> diagnostics = ValidateRuntimeProgramUVE(program);
    if (!diagnostics.empty()) {
        return {ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram, iterator->second.generation, true, false,
                diagnostics, "Runtime reload rejected; last-known-good program was retained."};
    }
    const bool compatibleStatePreserved = iterator->second.program.version == program.version;
    iterator->second.program = std::move(program);
    if (!compatibleStatePreserved) {
        iterator->second.state = {};
    }
    if (iterator->second.generation < std::numeric_limits<std::uint64_t>::max()) {
        ++iterator->second.generation;
    }
    return {ScriptRuntimeReloadCodeUVE::Accepted, iterator->second.generation, false, compatibleStatePreserved, {},
            compatibleStatePreserved ? "Runtime program reload accepted; compatible state preserved."
                                      : "Runtime program reload accepted; incompatible state was reset."};
}

ScriptRuntimeDetachResultUVE ScriptRuntimeUVE::DetachDetailedUVE(const Scene::EntityUVE entity) noexcept {
    if (m_instances.erase(entity) == 0U) {
        return {ScriptRuntimeDetachCodeUVE::NoActiveInstance,
                "Runtime detachment rejected because no active instance matches the entity handle."};
    }
    return {ScriptRuntimeDetachCodeUVE::Applied, "Runtime instance detached."};
}

bool ScriptRuntimeUVE::DetachUVE(const Scene::EntityUVE entity) noexcept {
    return DetachDetailedUVE(entity).IsAcceptedUVE();
}

ScriptRuntimeEnabledUpdateResultUVE ScriptRuntimeUVE::SetEnabledDetailedUVE(
    const Scene::EntityUVE entity, const bool enabled) noexcept {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return {ScriptRuntimeEnabledUpdateCodeUVE::NoActiveInstance,
                "Enabled-state update rejected because no active instance exists."};
    }
    if (iterator->second.enabled == enabled) {
        return {ScriptRuntimeEnabledUpdateCodeUVE::Unchanged, "Enabled state is unchanged."};
    }
    iterator->second.enabled = enabled;
    return {ScriptRuntimeEnabledUpdateCodeUVE::Applied, "Enabled state updated."};
}

bool ScriptRuntimeUVE::SetEnabledUVE(const Scene::EntityUVE entity, const bool enabled) noexcept {
    return SetEnabledDetailedUVE(entity, enabled).IsAcceptedUVE();
}

ScriptRuntimeStateUpdateResultUVE ScriptRuntimeUVE::SetStateDetailedUVE(const Scene::EntityUVE entity,
                                                                            ScriptRuntimeStateUVE state) {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return {ScriptRuntimeStateUpdateCodeUVE::NoActiveInstance,
                "State update rejected because no active instance exists."};
    }
    if (state.values.size() > kMaximumStateValuesUVE) {
        return {ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded,
                "State update rejected because the scalar state vector exceeds its bounded capacity."};
    }
    if (state.vector3Values.size() > kMaximumStateVector3ValuesUVE) {
        return {ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded,
                "State update rejected because the typed Vector3 state exceeds its bounded capacity."};
    }
    if (state.executionContext.inputs.size() > kMaximumStateVmBindingsUVE ||
        state.executionContext.outputs.size() > kMaximumStateVmBindingsUVE) {
        return {ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded,
                "State update rejected because the VM binding context exceeds its bounded capacity."};
    }
    if (!IsValidScriptVmBindingsUVE(state.executionContext)) {
        return {ScriptRuntimeStateUpdateCodeUVE::InvalidVmBinding,
                "State update rejected because a VM binding has an invalid identity or non-finite value."};
    }
    if (std::any_of(state.vector3Values.begin(), state.vector3Values.end(),
                    [](const ScriptVector3ValueUVE& value) { return !IsFiniteScriptVector3UVE(value); })) {
        return {ScriptRuntimeStateUpdateCodeUVE::NonFiniteVector3,
                "State update rejected because a typed Vector3 value contains a non-finite component."};
    }
    if (iterator->second.state == state) {
        return {ScriptRuntimeStateUpdateCodeUVE::Unchanged, "Runtime state is unchanged."};
    }
    iterator->second.state = std::move(state);
    return {ScriptRuntimeStateUpdateCodeUVE::Applied, "Runtime state updated."};
}

bool ScriptRuntimeUVE::SetStateUVE(const Scene::EntityUVE entity, ScriptRuntimeStateUVE state) {
    return SetStateDetailedUVE(entity, std::move(state)).IsAcceptedUVE();
}

std::optional<ScriptRuntimeStateUVE> ScriptRuntimeUVE::GetStateUVE(const Scene::EntityUVE entity) const {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return std::nullopt;
    }
    return iterator->second.state;
}

std::vector<ScriptRuntimeInstanceSnapshotUVE> ScriptRuntimeUVE::GetSnapshotUVE() const {
    std::vector<ScriptRuntimeInstanceSnapshotUVE> snapshots;
    snapshots.reserve(m_instances.size());
    for (const auto& [entity, instance] : m_instances) {
        snapshots.push_back({entity,
                             instance.generation,
                             instance.program.version,
                             instance.program.instructions.size(),
                             instance.state.values.size(),
                             instance.enabled});
    }
    std::sort(snapshots.begin(), snapshots.end(), [](const ScriptRuntimeInstanceSnapshotUVE& lhs,
                                                     const ScriptRuntimeInstanceSnapshotUVE& rhs) {
        if (lhs.entity.index != rhs.entity.index) {
            return lhs.entity.index < rhs.entity.index;
        }
        return lhs.entity.generation < rhs.entity.generation;
    });
    return snapshots;
}

bool ScriptRuntimeUVE::HasInstanceUVE(const Scene::EntityUVE entity) const noexcept {
    return m_instances.contains(entity);
}

std::size_t ScriptRuntimeUVE::GetInstanceCountUVE() const noexcept {
    return m_instances.size();
}

ScriptRuntimeTickBatchResultUVE ScriptRuntimeUVE::TickDetailedUVE(
    const ScriptVmExecutionOptionsUVE options) {
    std::vector<Scene::EntityUVE> entities;
    entities.reserve(m_instances.size());
    for (const auto& [entity, instance] : m_instances) {
        if (instance.enabled) {
            entities.push_back(entity);
        }
    }
    std::sort(entities.begin(), entities.end(), [](const Scene::EntityUVE& lhs, const Scene::EntityUVE& rhs) {
        if (lhs.index != rhs.index) {
            return lhs.index < rhs.index;
        }
        return lhs.generation < rhs.generation;
    });

    ScriptRuntimeTickBatchResultUVE batch;
    batch.summary.enabledInstanceCount = entities.size();
    batch.results.reserve(entities.size());
    for (const Scene::EntityUVE entity : entities) {
        const auto iterator = m_instances.find(entity);
        ScriptVmExecutionResultUVE execution = ExecuteScriptBytecodeUVE(
            iterator->second.program, iterator->second.state.executionContext, options);
        switch (execution.status) {
        case ScriptVmStatusUVE::Completed:
            ++batch.summary.completedCount;
            break;
        case ScriptVmStatusUVE::InstructionBudgetExceeded:
            ++batch.summary.instructionBudgetExceededCount;
            break;
        case ScriptVmStatusUVE::InvalidInstruction:
            ++batch.summary.invalidInstructionCount;
            break;
        case ScriptVmStatusUVE::NodeExecutionFailed:
            ++batch.summary.nodeExecutionFailedCount;
            break;
        }
        batch.summary.diagnosticCount += execution.diagnostics.size();
        batch.results.push_back({entity, std::move(execution)});
    }
    return batch;
}

std::vector<ScriptRuntimeTickResultUVE> ScriptRuntimeUVE::TickUVE(
    const ScriptVmExecutionOptionsUVE options) {
    return TickDetailedUVE(options).results;
}

} // namespace UVE::Scripting
