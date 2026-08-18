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
    ConditionalJump = 2,
    SequenceDispatch = 3,
    FlowControlDispatch = 4,
};

struct ScriptIrInstructionUVE final {
    ScriptIrInstructionKindUVE kind = ScriptIrInstructionKindUVE::ExecuteNode;
    std::uint32_t sourceNodeId = 0U;
    std::uint32_t targetNodeId = 0U;
    std::string nodeTypeId;
    std::string sourcePinName;
    std::string targetPinName;
    std::uint32_t trueTargetInstructionIndex = 0U;
    std::uint32_t falseTargetInstructionIndex = 0U;
    std::uint32_t firstTargetInstructionIndex = 0U;
    std::uint32_t secondTargetInstructionIndex = 0U;
    // True only for compiler-ordered staging transfers; ordinary graph transfers remain false.
    bool isStagedTransfer = false;
    std::uint32_t defaultTargetInstructionIndex = 0U;
};

struct ScriptIrProgramUVE final {
    static constexpr std::size_t kMaximumInstructionsUVE = 256U;
    static constexpr std::uint32_t kCurrentVersionUVE = 5U;

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
