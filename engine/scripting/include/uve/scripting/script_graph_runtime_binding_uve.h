// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_runtime_uve.h"

#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptGraphRuntimeBindingCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidEntity,
    CompileRejected,
    LoweringRejected,
    RuntimeRejected,
};

/// Native transactional seam from a validated graph resource to the existing bytecode/runtime path.
/// Compilation and lowering complete before ScriptRuntimeUVE is mutated; managed code does not call this seam.
struct ScriptGraphRuntimeBindingResultUVE final {
    ScriptGraphRuntimeBindingCodeUVE code = ScriptGraphRuntimeBindingCodeUVE::CompileRejected;
    ScriptRuntimeAttachCodeUVE runtimeCode = ScriptRuntimeAttachCodeUVE::InvalidProgram;
    std::vector<ScriptValidationDiagnosticUVE> compileDiagnostics;
    std::vector<ScriptBytecodeDiagnosticUVE> bytecodeDiagnostics;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptGraphRuntimeBindingCodeUVE::Accepted;
    }
};

class ScriptGraphRuntimeBindingUVE final {
public:
    [[nodiscard]] static ScriptGraphRuntimeBindingResultUVE BindUVE(
        const ScriptGraphUVE& graph, const ScriptNodeRegistryUVE& registry,
        ScriptRuntimeUVE& runtime, Scene::EntityUVE entity);
};

} // namespace UVE::Scripting
