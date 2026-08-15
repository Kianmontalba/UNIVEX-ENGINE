// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_bytecode_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptVmStatusUVE : std::uint8_t {
    Completed = 0,
    InstructionBudgetExceeded,
    InvalidInstruction,
};

struct ScriptVmDiagnosticUVE final {
    std::size_t instructionIndex = 0U;
    std::string message;
};

struct ScriptVmExecutionOptionsUVE final {
    std::size_t instructionBudget = 4096U;
};

struct ScriptVmExecutionResultUVE final {
    ScriptVmStatusUVE status = ScriptVmStatusUVE::Completed;
    std::size_t instructionsExecuted = 0U;
    std::vector<ScriptVmDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return status == ScriptVmStatusUVE::Completed && diagnostics.empty();
    }
};

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionOptionsUVE options = {});

} // namespace UVE::Scripting
