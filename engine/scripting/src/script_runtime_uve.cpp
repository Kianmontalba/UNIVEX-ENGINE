// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_runtime_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Scripting {
namespace {

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

bool ScriptRuntimeUVE::AttachUVE(const Scene::EntityUVE entity, ScriptBytecodeProgramUVE program) {
    if (entity == Scene::kInvalidEntityUVE || !ValidateRuntimeProgramUVE(program).empty() ||
        m_instances.size() >= kMaximumInstancesUVE || m_instances.contains(entity)) {
        return false;
    }
    m_instances.emplace(entity, ScriptRuntimeInstanceUVE{entity, std::move(program), 1U, true});
    return true;
}

ScriptRuntimeReloadResultUVE ScriptRuntimeUVE::ReloadUVE(const Scene::EntityUVE entity,
                                                          ScriptBytecodeProgramUVE program) {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return {ScriptRuntimeReloadCodeUVE::NoActiveInstance, 0U, false, {},
                "Runtime reload rejected because no active instance exists."};
    }
    const std::vector<ScriptBytecodeDiagnosticUVE> diagnostics = ValidateRuntimeProgramUVE(program);
    if (!diagnostics.empty()) {
        return {ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram, iterator->second.generation, true,
                diagnostics, "Runtime reload rejected; last-known-good program was retained."};
    }
    iterator->second.program = std::move(program);
    if (iterator->second.generation < std::numeric_limits<std::uint64_t>::max()) {
        ++iterator->second.generation;
    }
    return {ScriptRuntimeReloadCodeUVE::Accepted, iterator->second.generation, false, {},
            "Runtime program reload accepted."};
}

bool ScriptRuntimeUVE::DetachUVE(const Scene::EntityUVE entity) noexcept {
    return m_instances.erase(entity) != 0U;
}

bool ScriptRuntimeUVE::SetEnabledUVE(const Scene::EntityUVE entity, const bool enabled) noexcept {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return false;
    }
    iterator->second.enabled = enabled;
    return true;
}

bool ScriptRuntimeUVE::HasInstanceUVE(const Scene::EntityUVE entity) const noexcept {
    return m_instances.contains(entity);
}

std::size_t ScriptRuntimeUVE::GetInstanceCountUVE() const noexcept {
    return m_instances.size();
}

std::vector<ScriptRuntimeTickResultUVE> ScriptRuntimeUVE::TickUVE(
    const ScriptVmExecutionOptionsUVE options) const {
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
    std::vector<ScriptRuntimeTickResultUVE> results;
    results.reserve(entities.size());
    for (const Scene::EntityUVE entity : entities) {
        const auto iterator = m_instances.find(entity);
        results.push_back({entity, ExecuteScriptBytecodeUVE(iterator->second.program, options)});
    }
    return results;
}

} // namespace UVE::Scripting
