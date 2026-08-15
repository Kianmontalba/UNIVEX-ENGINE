// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/entity_uve.h"
#include "uve/scripting/script_vm_uve.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace UVE::Scripting {

struct ScriptRuntimeInstanceUVE final {
    Scene::EntityUVE entity;
    ScriptBytecodeProgramUVE program;
    bool enabled = true;
};

struct ScriptRuntimeTickResultUVE final {
    Scene::EntityUVE entity;
    ScriptVmExecutionResultUVE execution;
};

class ScriptRuntimeUVE final {
public:
    static constexpr std::size_t kMaximumInstancesUVE = 4096U;

    ScriptRuntimeUVE() = default;
    ScriptRuntimeUVE(const ScriptRuntimeUVE&) = delete;
    ScriptRuntimeUVE& operator=(const ScriptRuntimeUVE&) = delete;

    [[nodiscard]] bool AttachUVE(Scene::EntityUVE entity, ScriptBytecodeProgramUVE program);
    [[nodiscard]] bool DetachUVE(Scene::EntityUVE entity) noexcept;
    [[nodiscard]] bool SetEnabledUVE(Scene::EntityUVE entity, bool enabled) noexcept;
    [[nodiscard]] bool HasInstanceUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept;
    [[nodiscard]] std::vector<ScriptRuntimeTickResultUVE> TickUVE(
        ScriptVmExecutionOptionsUVE options = {}) const;

private:
    std::unordered_map<Scene::EntityUVE, ScriptRuntimeInstanceUVE> m_instances;
};

} // namespace UVE::Scripting
