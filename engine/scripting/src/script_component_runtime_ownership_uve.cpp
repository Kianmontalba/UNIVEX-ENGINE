// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_component_runtime_ownership_uve.h"

namespace UVE::Scripting {

ScriptComponentRuntimeOwnershipResultUVE ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
    const Scene::ScriptComponentUVE& component, const ScriptGraphUVE& graph,
    const ScriptNodeRegistryUVE& registry, ScriptRuntimeUVE& runtime, const Scene::EntityUVE entity) {
    ScriptComponentRuntimeOwnershipResultUVE result;
    if (entity == Scene::kInvalidEntityUVE) {
        result.code = ScriptComponentRuntimeOwnershipCodeUVE::InvalidEntity;
        result.message = "Script runtime ownership requires a valid generational entity.";
        return result;
    }
    if (!Scene::IsScriptComponentValidUVE(component)) {
        result.code = ScriptComponentRuntimeOwnershipCodeUVE::InvalidComponent;
        result.message = "Script runtime ownership rejected an invalid script component path.";
        return result;
    }
    if (component.scriptAssetPath.empty()) {
        result.detach = runtime.DetachDetailedUVE(entity);
        result.code = ScriptComponentRuntimeOwnershipCodeUVE::Detached;
        result.message = result.detach.code == ScriptRuntimeDetachCodeUVE::Applied
                             ? "Empty ScriptComponent path detached the runtime instance."
                             : "Empty ScriptComponent path left the runtime detached.";
        return result;
    }
    if (runtime.HasInstanceUVE(entity)) {
        result.code = ScriptComponentRuntimeOwnershipCodeUVE::DuplicateRuntime;
        result.message = "Script runtime ownership rejected a replacement while an instance is active.";
        return result;
    }

    result.binding = ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, entity);
    if (result.binding.IsAcceptedUVE()) {
        result.code = ScriptComponentRuntimeOwnershipCodeUVE::Attached;
        result.message = "Validated ScriptComponent path attached the compiled graph to ScriptRuntimeUVE.";
        return result;
    }
    result.code = result.binding.code == ScriptGraphRuntimeBindingCodeUVE::RuntimeRejected
                      ? ScriptComponentRuntimeOwnershipCodeUVE::RuntimeRejected
                      : ScriptComponentRuntimeOwnershipCodeUVE::GraphRejected;
    result.message = result.binding.message;
    return result;
}

} // namespace UVE::Scripting

