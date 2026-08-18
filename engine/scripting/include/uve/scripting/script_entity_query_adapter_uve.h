// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scripting/script_vm_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace UVE::Scripting {

struct ScriptEntityComponentTypeBindingUVE final {
    std::string scriptTypeId;
    std::type_index nativeType{typeid(void)};

    ScriptEntityComponentTypeBindingUVE() = default;
    ScriptEntityComponentTypeBindingUVE(std::string typeId, const std::type_index type)
        : scriptTypeId(std::move(typeId)), nativeType(type) {}

    [[nodiscard]] bool operator==(const ScriptEntityComponentTypeBindingUVE&) const = default;
};

enum class ScriptEntityQueryAdapterCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidEntity,
    EntityNotAlive,
    InvalidBinding,
    DuplicateBinding,
    CapacityExceeded,
};

struct ScriptEntityQueryAdapterResultUVE final {
    ScriptEntityQueryAdapterCodeUVE code = ScriptEntityQueryAdapterCodeUVE::InvalidBinding;
    std::size_t factsWritten = 0U;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return code == ScriptEntityQueryAdapterCodeUVE::Applied;
    }
};

/// Populates the VM's copied entity-component facts from an ECS snapshot without transferring
/// scene-manager pointers or component storage into bytecode execution. Bindings are caller-owned
/// and must provide stable script IDs for native std::type_index values.
class ScriptEntityQueryAdapterUVE final {
public:
    static constexpr std::size_t kMaximumBindingsUVE = 64U;

    [[nodiscard]] static ScriptEntityQueryAdapterResultUVE PopulateComponentFactsUVE(
        const Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE entity,
        const std::vector<ScriptEntityComponentTypeBindingUVE>& bindings,
        ScriptVmExecutionContextUVE& context);
};

} // namespace UVE::Scripting
