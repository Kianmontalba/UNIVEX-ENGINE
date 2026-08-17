// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>

#include "uve/scene/components/script_component_uve.h"
#include "uve/scripting/script_graph_runtime_binding_uve.h"

namespace UVE::Scripting {

enum class ScriptComponentRuntimeOwnershipCodeUVE : std::uint8_t {
    Attached = 0,
    Detached,
    InvalidEntity,
    InvalidComponent,
    DuplicateRuntime,
    GraphRejected,
    RuntimeRejected,
};

struct ScriptComponentRuntimeOwnershipResultUVE final {
    ScriptComponentRuntimeOwnershipCodeUVE code =
        ScriptComponentRuntimeOwnershipCodeUVE::InvalidComponent;
    ScriptGraphRuntimeBindingResultUVE binding;
    ScriptRuntimeDetachResultUVE detach;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptComponentRuntimeOwnershipCodeUVE::Attached ||
               code == ScriptComponentRuntimeOwnershipCodeUVE::Detached;
    }
};

/// Reconciles one validated ScriptComponentUVE with the existing native ScriptRuntimeUVE lifecycle.
/// The caller supplies the already-resolved graph; this seam validates the component path but never
/// reads the filesystem, resolves an asset, owns bytecode, or transfers ECS/entity ownership.
class ScriptComponentRuntimeOwnershipUVE final {
public:
    [[nodiscard]] static ScriptComponentRuntimeOwnershipResultUVE ReconcileUVE(
        const Scene::ScriptComponentUVE& component, const ScriptGraphUVE& graph,
        const ScriptNodeRegistryUVE& registry, ScriptRuntimeUVE& runtime, Scene::EntityUVE entity);
};

} // namespace UVE::Scripting

