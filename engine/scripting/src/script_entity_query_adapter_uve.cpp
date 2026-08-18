// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_entity_query_adapter_uve.h"

#include <algorithm>
#include <typeinfo>
#include <utility>

namespace UVE::Scripting {

ScriptEntityQueryAdapterResultUVE ScriptEntityQueryAdapterUVE::PopulateComponentFactsUVE(
    const Scene::IEntityManagerUVE& entityManager, const Scene::EntityUVE entity,
    const std::vector<ScriptEntityComponentTypeBindingUVE>& bindings,
    ScriptVmExecutionContextUVE& context) {
    if (entity == Scene::kInvalidEntityUVE) {
        return {ScriptEntityQueryAdapterCodeUVE::InvalidEntity, 0U,
                "Entity query adapter rejected the invalid entity handle."};
    }
    if (!entityManager.IsAliveUVE(entity)) {
        return {ScriptEntityQueryAdapterCodeUVE::EntityNotAlive, 0U,
                "Entity query adapter rejected an entity that is not alive."};
    }
    if (bindings.size() > kMaximumBindingsUVE) {
        return {ScriptEntityQueryAdapterCodeUVE::CapacityExceeded, 0U,
                "Entity query adapter rejected bindings above its bounded capacity."};
    }
    if (context.componentFacts.size() > ScriptVmExecutionContextUVE::kMaximumComponentFactsUVE) {
        return {ScriptEntityQueryAdapterCodeUVE::CapacityExceeded, 0U,
                "Entity query adapter rejected a VM fact table above its bounded capacity."};
    }

    for (std::size_t firstIndex = 0U; firstIndex < bindings.size(); ++firstIndex) {
        const ScriptEntityComponentTypeBindingUVE& first = bindings[firstIndex];
        if (first.scriptTypeId.empty() || first.nativeType == std::type_index(typeid(void))) {
            return {ScriptEntityQueryAdapterCodeUVE::InvalidBinding, 0U,
                    "Entity query adapter rejected an empty script ID or void native type binding."};
        }
        for (std::size_t secondIndex = firstIndex + 1U; secondIndex < bindings.size(); ++secondIndex) {
            const ScriptEntityComponentTypeBindingUVE& second = bindings[secondIndex];
            if (first.scriptTypeId == second.scriptTypeId || first.nativeType == second.nativeType) {
                return {ScriptEntityQueryAdapterCodeUVE::DuplicateBinding, 0U,
                        "Entity query adapter rejected duplicate script or native component bindings."};
            }
        }
    }

    const std::vector<std::type_index> componentTypes = entityManager.GetComponentTypesUVE(entity);
    std::size_t newFactCount = 0U;
    for (const ScriptEntityComponentTypeBindingUVE& binding : bindings) {
        if (!context.FindComponentFactUVE(entity, binding.scriptTypeId).has_value()) {
            ++newFactCount;
        }
    }
    if (context.componentFacts.size() + newFactCount > ScriptVmExecutionContextUVE::kMaximumComponentFactsUVE) {
        return {ScriptEntityQueryAdapterCodeUVE::CapacityExceeded, 0U,
                "Entity query adapter rejected because the VM fact table would exceed capacity."};
    }

    std::vector<ScriptComponentValueUVE> stagedFacts;
    stagedFacts.reserve(bindings.size());
    for (const ScriptEntityComponentTypeBindingUVE& binding : bindings) {
        const bool present = std::find(componentTypes.begin(), componentTypes.end(), binding.nativeType) !=
                             componentTypes.end();
        stagedFacts.push_back({entity, binding.scriptTypeId, present});
    }
    for (const ScriptComponentValueUVE& fact : stagedFacts) {
        if (!context.SetComponentFactUVE(fact.entity, fact.componentTypeId, fact.present)) {
            return {ScriptEntityQueryAdapterCodeUVE::CapacityExceeded, 0U,
                    "Entity query adapter could not commit its staged VM facts."};
        }
    }
    return {ScriptEntityQueryAdapterCodeUVE::Applied, stagedFacts.size(),
            "Entity query adapter populated copied component facts."};
}

} // namespace UVE::Scripting

// EOF
