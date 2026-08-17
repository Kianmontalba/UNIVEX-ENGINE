// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_vector3_value_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace UVE::Scripting {

enum class ScriptVmStatusUVE : std::uint8_t {
    Completed = 0,
    InstructionBudgetExceeded,
    InvalidInstruction,
    NodeExecutionFailed,
};

using ScriptVmValueUVE = std::variant<float, ScriptVector3ValueUVE>;

struct ScriptVmValueBindingUVE final {
    std::uint32_t nodeId = 0U;
    std::string pinName;
    ScriptVmValueUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptVmValueBindingUVE&) const = default;
};

struct ScriptVmExecutionContextUVE final {
    static constexpr std::size_t kMaximumBindingsUVE = 1024U;

    std::vector<ScriptVmValueBindingUVE> inputs;
    std::vector<ScriptVmValueBindingUVE> outputs;

    [[nodiscard]] bool SetInputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] bool SetOutputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindInputUVE(std::uint32_t nodeId,
                                                                const std::string& pinName) const;
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindOutputUVE(std::uint32_t nodeId,
                                                                 const std::string& pinName) const;
    void ClearOutputsUVE() noexcept;

    [[nodiscard]] bool operator==(const ScriptVmExecutionContextUVE&) const = default;
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

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionContextUVE& context,
    ScriptVmExecutionOptionsUVE options = {});

} // namespace UVE::Scripting
