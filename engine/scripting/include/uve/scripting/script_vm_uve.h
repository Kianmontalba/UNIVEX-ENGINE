// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/entity_uve.h"
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_vector2_value_uve.h"
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

struct ScriptEntityValueUVE final {
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return entity != Scene::kInvalidEntityUVE;
    }

    [[nodiscard]] bool operator==(const ScriptEntityValueUVE&) const = default;
};

struct ScriptComponentValueUVE final {
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    std::string componentTypeId;
    bool present = false;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return !componentTypeId.empty();
    }

    [[nodiscard]] bool IsValidQueryFactUVE() const noexcept {
        return entity != Scene::kInvalidEntityUVE && IsValidUVE();
    }

    [[nodiscard]] bool operator==(const ScriptComponentValueUVE&) const = default;
};

/// Typed VM values are intentionally value-only and bounded by the execution context; append new
/// alternatives so existing float/Vector3/Boolean variant indices remain stable for serialized callers.
using ScriptVmValueUVE =
    std::variant<float, ScriptVector3ValueUVE, bool, ScriptEntityValueUVE, ScriptComponentValueUVE,
                 ScriptVector2ValueUVE>;

struct ScriptVmLocalVariableUVE final {
    std::uint32_t slot = 0U;
    ScriptVmValueUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptVmLocalVariableUVE&) const = default;
};

struct ScriptVmValueBindingUVE final {
    std::uint32_t nodeId = 0U;
    std::string pinName;
    ScriptVmValueUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptVmValueBindingUVE&) const = default;
};

struct ScriptVmFlowControlLatchUVE final {
    std::uint32_t nodeId = 0U;
    bool fired = false;

    [[nodiscard]] bool operator==(const ScriptVmFlowControlLatchUVE&) const = default;
};

struct ScriptVmGateStateUVE final {
    std::uint32_t nodeId = 0U;
    bool open = false;

    [[nodiscard]] bool operator==(const ScriptVmGateStateUVE&) const = default;
};

struct ScriptVmExecutionContextUVE final {
    static constexpr std::size_t kMaximumBindingsUVE = 1024U;
    static constexpr std::size_t kMaximumComponentFactsUVE = 256U;
    static constexpr std::size_t kMaximumLocalVariablesUVE = 256U;
    static constexpr std::size_t kMaximumFlowControlLatchesUVE = 256U;
    static constexpr std::size_t kMaximumGateStatesUVE = 256U;

    std::vector<ScriptVmValueBindingUVE> inputs;
    std::vector<ScriptVmValueBindingUVE> outputs;
    std::vector<ScriptComponentValueUVE> componentFacts;
    std::vector<ScriptVmLocalVariableUVE> localVariables;
    std::vector<ScriptVmFlowControlLatchUVE> flowControlLatches;
    std::vector<ScriptVmGateStateUVE> gateStates;

    [[nodiscard]] bool InitializeLocalVariableUVE(std::uint32_t slot, ScriptVmValueUVE value);
    [[nodiscard]] bool SetLocalVariableUVE(std::uint32_t slot, ScriptVmValueUVE value);
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindLocalVariableUVE(std::uint32_t slot) const;

    [[nodiscard]] bool InitializeDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] bool TryConsumeDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] bool ResetDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] std::optional<bool> FindDoOnceLatchUVE(std::uint32_t nodeId) const;
    [[nodiscard]] bool InitializeGateStateUVE(std::uint32_t nodeId);
    [[nodiscard]] bool SetGateStateUVE(std::uint32_t nodeId, bool open);
    [[nodiscard]] std::optional<bool> FindGateStateUVE(std::uint32_t nodeId) const;

    [[nodiscard]] bool SetInputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] bool SetOutputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindInputUVE(std::uint32_t nodeId,
                                                                const std::string& pinName) const;
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindOutputUVE(std::uint32_t nodeId,
                                                                 const std::string& pinName) const;
    [[nodiscard]] bool SetComponentFactUVE(Scene::EntityUVE entity, std::string componentTypeId,
                                            bool present);
    [[nodiscard]] std::optional<ScriptComponentValueUVE> FindComponentFactUVE(
        Scene::EntityUVE entity, const std::string& componentTypeId) const;
    void ClearOutputsUVE() noexcept;

    [[nodiscard]] bool operator==(const ScriptVmExecutionContextUVE&) const = default;
};

struct ScriptVmDiagnosticUVE final {
    std::size_t instructionIndex = 0U;
    std::string message;
};

enum class ScriptVmTraceEventKindUVE : std::uint8_t {
    NodeExecuted = 0,
    ValueTransferred,
    QueryFactsRefreshed,
    Completed,
    Failed,
    StagedValueTransferred,
};

struct ScriptVmTraceEventUVE final {
    ScriptVmTraceEventKindUVE kind = ScriptVmTraceEventKindUVE::Failed;
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    std::size_t instructionIndex = 0U;
    std::uint32_t sourceNodeId = 0U;
    std::uint32_t targetNodeId = 0U;
    std::string nodeTypeId;
    std::string message;

    [[nodiscard]] bool operator==(const ScriptVmTraceEventUVE&) const = default;
};

using ScriptEngineLogFunctionUVE = bool (*)(void* userData, float value) noexcept;
using ScriptEngineGetTimeFunctionUVE = bool (*)(void* userData, float* outSeconds) noexcept;

struct ScriptEngineCallBindingsUVE final {
    ScriptEngineLogFunctionUVE log = nullptr;
    void* userData = nullptr;
    ScriptEngineGetTimeFunctionUVE getTime = nullptr;
};

struct ScriptVmExecutionOptionsUVE final {
    std::size_t instructionBudget = 4096U;
    const ScriptEngineCallBindingsUVE* engineCallBindings = nullptr;
};

struct ScriptVmExecutionResultUVE final {
    static constexpr std::size_t kMaximumTraceEventsUVE = 512U;
    static constexpr std::size_t kMaximumTraceMessageBytesUVE = 256U;

    ScriptVmStatusUVE status = ScriptVmStatusUVE::Completed;
    std::size_t instructionsExecuted = 0U;
    std::vector<ScriptVmDiagnosticUVE> diagnostics;
    std::vector<ScriptVmTraceEventUVE> trace;
    bool traceTruncated = false;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return status == ScriptVmStatusUVE::Completed && diagnostics.empty();
    }

    void AppendTraceEventUVE(ScriptVmTraceEventUVE event);
    void PrependTraceEventsUVE(std::vector<ScriptVmTraceEventUVE> prefix, bool prefixTruncated = false);

    [[nodiscard]] bool operator==(const ScriptVmExecutionResultUVE&) const = default;
};

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionOptionsUVE options = {});

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionContextUVE& context,
    ScriptVmExecutionOptionsUVE options = {});

} // namespace UVE::Scripting
