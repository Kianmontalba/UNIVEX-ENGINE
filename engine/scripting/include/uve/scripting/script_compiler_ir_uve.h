// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptIrInstructionKindUVE : std::uint8_t {
    ExecuteNode = 0,
    TransferValue = 1,
};

struct ScriptIrInstructionUVE final {
    ScriptIrInstructionKindUVE kind = ScriptIrInstructionKindUVE::ExecuteNode;
    std::uint32_t sourceNodeId = 0U;
    std::uint32_t targetNodeId = 0U;
    std::string nodeTypeId;
    std::string sourcePinName;
    std::string targetPinName;
};

struct ScriptIrProgramUVE final {
    static constexpr std::uint32_t kCurrentVersionUVE = 1U;

    std::uint32_t version = kCurrentVersionUVE;
    std::vector<ScriptIrInstructionUVE> instructions;
    std::vector<std::uint32_t> sourceNodeIds;
};

struct ScriptIrCompileResultUVE final {
    std::optional<ScriptIrProgramUVE> program;
    std::vector<ScriptValidationDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return program.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] ScriptIrCompileResultUVE CompileScriptGraphToIrUVE(
    const ScriptGraphUVE& graph,
    const ScriptNodeRegistryUVE& registry);

} // namespace UVE::Scripting
