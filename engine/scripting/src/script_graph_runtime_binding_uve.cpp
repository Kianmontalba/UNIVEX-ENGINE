// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_graph_runtime_binding_uve.h"

#include <utility>

namespace UVE::Scripting {

ScriptGraphRuntimeBindingResultUVE ScriptGraphRuntimeBindingUVE::BindUVE(
    const ScriptGraphUVE& graph, const ScriptNodeRegistryUVE& registry,
    ScriptRuntimeUVE& runtime, const Scene::EntityUVE entity) {
    ScriptGraphRuntimeBindingResultUVE result;
    if (entity == Scene::kInvalidEntityUVE) {
        result.code = ScriptGraphRuntimeBindingCodeUVE::InvalidEntity;
        result.runtimeCode = ScriptRuntimeAttachCodeUVE::InvalidEntity;
        result.message = "Graph runtime binding requires a valid generational entity.";
        return result;
    }

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    if (!compiled.IsSuccessUVE()) {
        result.code = ScriptGraphRuntimeBindingCodeUVE::CompileRejected;
        result.compileDiagnostics = compiled.diagnostics;
        result.message = "Graph runtime binding was rejected by native graph compilation.";
        return result;
    }

    std::vector<ScriptBytecodeDiagnosticUVE> loweringDiagnostics;
    std::optional<ScriptBytecodeProgramUVE> program =
        LowerIrToBytecodeUVE(*compiled.program, loweringDiagnostics);
    if (!program.has_value()) {
        result.code = ScriptGraphRuntimeBindingCodeUVE::LoweringRejected;
        result.bytecodeDiagnostics = std::move(loweringDiagnostics);
        result.message = "Graph runtime binding was rejected during native bytecode lowering.";
        return result;
    }

    const ScriptRuntimeAttachResultUVE attached = runtime.AttachDetailedUVE(entity, std::move(*program));
    result.runtimeCode = attached.code;
    result.bytecodeDiagnostics = attached.diagnostics;
    result.message = attached.message;
    if (!attached.IsAcceptedUVE()) {
        result.code = ScriptGraphRuntimeBindingCodeUVE::RuntimeRejected;
        return result;
    }
    result.code = ScriptGraphRuntimeBindingCodeUVE::Accepted;
    return result;
}

} // namespace UVE::Scripting
