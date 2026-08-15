// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_runtime_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Scripting {

bool ScriptRuntimeUVE::AttachUVE(const Scene::EntityUVE entity, ScriptBytecodeProgramUVE program) {
    if (entity == Scene::kInvalidEntityUVE || program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE ||
        program.instructions.size() > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE ||
        m_instances.size() >= kMaximumInstancesUVE || m_instances.contains(entity)) {
        return false;
    }
    m_instances.emplace(entity, ScriptRuntimeInstanceUVE{entity, std::move(program), true});
    return true;
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
