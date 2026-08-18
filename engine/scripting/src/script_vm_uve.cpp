// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vm_uve.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UVE::Scripting {
namespace {

[[nodiscard]] ScriptVmValueBindingUVE* FindMutableBindingUVE(
    std::vector<ScriptVmValueBindingUVE>& bindings, const std::uint32_t nodeId,
    const std::string& pinName) noexcept {
    const auto iterator = std::find_if(bindings.begin(), bindings.end(),
                                       [nodeId, &pinName](const ScriptVmValueBindingUVE& binding) {
                                           return binding.nodeId == nodeId && binding.pinName == pinName;
                                       });
    return iterator == bindings.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ScriptVmValueBindingUVE* FindBindingUVE(
    const std::vector<ScriptVmValueBindingUVE>& bindings, const std::uint32_t nodeId,
    const std::string& pinName) noexcept {
    const auto iterator = std::find_if(bindings.begin(), bindings.end(),
                                       [nodeId, &pinName](const ScriptVmValueBindingUVE& binding) {
                                           return binding.nodeId == nodeId && binding.pinName == pinName;
                                       });
    return iterator == bindings.end() ? nullptr : &*iterator;
}

[[nodiscard]] ScriptVmExecutionResultUVE MakeNodeFailureUVE(const std::size_t instructionIndex,
                                                             std::string message) {
    ScriptVmExecutionResultUVE result;
    result.status = ScriptVmStatusUVE::NodeExecutionFailed;
    result.diagnostics.push_back({instructionIndex, std::move(message)});
    return result;
}

[[nodiscard]] const float* FindNumberInputUVE(const ScriptVmExecutionContextUVE& context,
                                              const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<float>(&binding->value);
}

[[nodiscard]] const ScriptVector3ValueUVE* FindVector3InputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptVector3ValueUVE>(&binding->value);
}

[[nodiscard]] const bool* FindBooleanInputUVE(const ScriptVmExecutionContextUVE& context,
                                              const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<bool>(&binding->value);
}

[[nodiscard]] bool SetNodeOutputUVE(ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId,
                                    const char* pinName, ScriptVmValueUVE value) {
    return context.SetOutputUVE(nodeId, pinName, std::move(value));
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteFloatNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const float* lhs = FindNumberInputUVE(context, nodeId, "A");
    const float* rhs = FindNumberInputUVE(context, nodeId, "B");
    if (lhs == nullptr || rhs == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Float binary node requires Number inputs A and B.");
    }
    if (!std::isfinite(*lhs) || !std::isfinite(*rhs)) {
        return MakeNodeFailureUVE(instructionIndex, "Float node rejected non-finite input.");
    }

    float result = 0.0F;
    if (instruction.nodeTypeId == "math.float.add") {
        result = *lhs + *rhs;
    } else if (instruction.nodeTypeId == "math.float.subtract") {
        result = *lhs - *rhs;
    } else if (instruction.nodeTypeId == "math.float.multiply") {
        result = *lhs * *rhs;
    } else if (instruction.nodeTypeId == "math.float.divide") {
        if (std::fabs(*rhs) <= 1.0e-6F) {
            return MakeNodeFailureUVE(instructionIndex, "Divide Float rejected a zero-near divisor.");
        }
        result = *lhs / *rhs;
    } else {
        return {};
    }
    if (!std::isfinite(result) || !SetNodeOutputUVE(context, nodeId, "Result", result)) {
        return MakeNodeFailureUVE(instructionIndex, "Float node rejected its result or output capacity.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteBooleanNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "logic.boolean.not") {
        const bool* value = FindBooleanInputUVE(context, nodeId, "Value");
        if (value == nullptr || !SetNodeOutputUVE(context, nodeId, "Result", !*value)) {
            return MakeNodeFailureUVE(instructionIndex, "Not Boolean requires a Boolean Value input or output capacity.");
        }
        return {};
    }

    const bool* lhs = FindBooleanInputUVE(context, nodeId, "A");
    const bool* rhs = FindBooleanInputUVE(context, nodeId, "B");
    if (lhs == nullptr || rhs == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Boolean binary node requires Boolean inputs A and B.");
    }
    bool result = false;
    if (instruction.nodeTypeId == "logic.boolean.and") {
        result = *lhs && *rhs;
    } else if (instruction.nodeTypeId == "logic.boolean.or") {
        result = *lhs || *rhs;
    } else if (instruction.nodeTypeId == "logic.boolean.xor") {
        result = *lhs != *rhs;
    } else {
        return {};
    }
    if (!SetNodeOutputUVE(context, nodeId, "Result", result)) {
        return MakeNodeFailureUVE(instructionIndex, "Boolean node rejected its output capacity.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteVector3NodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "math.vector3.make") {
        const float* x = FindNumberInputUVE(context, nodeId, "X");
        const float* y = FindNumberInputUVE(context, nodeId, "Y");
        const float* z = FindNumberInputUVE(context, nodeId, "Z");
        if (x == nullptr || y == nullptr || z == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Make Vector3 requires Number inputs X, Y, and Z.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3MakeUVE(*x, *y, *z);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Vector", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Make Vector3 rejected its inputs or output capacity.");
        }
        return {};
    }

    if (instruction.nodeTypeId == "math.vector3.add" || instruction.nodeTypeId == "math.vector3.subtract" ||
        instruction.nodeTypeId == "math.vector3.cross" || instruction.nodeTypeId == "math.vector3.dot") {
        const ScriptVector3ValueUVE* lhs = FindVector3InputUVE(context, nodeId, "A");
        const ScriptVector3ValueUVE* rhs = FindVector3InputUVE(context, nodeId, "B");
        if (lhs == nullptr || rhs == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Vector3 binary node requires Vector3 inputs A and B.");
        }
        if (instruction.nodeTypeId == "math.vector3.add") {
            const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3AddUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Add Vector3 rejected its inputs or output capacity.");
            }
        } else if (instruction.nodeTypeId == "math.vector3.subtract") {
            const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3SubtractUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Subtract Vector3 rejected its inputs or output capacity.");
            }
        } else if (instruction.nodeTypeId == "math.vector3.cross") {
            const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3CrossUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Cross Vector3 rejected its inputs or output capacity.");
            }
        } else {
            const ScriptVector3NumberResultUVE evaluated = EvaluateScriptVector3DotUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Dot Vector3 rejected its inputs or output capacity.");
            }
        }
        return {};
    }

    if (instruction.nodeTypeId == "math.vector3.multiply") {
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, "Vector");
        const float* scale = FindNumberInputUVE(context, nodeId, "Scale");
        if (vector == nullptr || scale == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Multiply Vector3 requires Vector3 Vector and Number Scale inputs.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3MultiplyUVE(*vector, *scale);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Multiply Vector3 rejected its inputs or output capacity.");
        }
        return {};
    }

    if (instruction.nodeTypeId == "math.vector3.length") {
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, "Vector");
        if (vector == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Length Vector3 requires a Vector3 Vector input.");
        }
        const ScriptVector3NumberResultUVE evaluated = EvaluateScriptVector3LengthUVE(*vector);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Length", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Length Vector3 rejected its input or output capacity.");
        }
        return {};
    }

    if (instruction.nodeTypeId == "math.vector3.normalize") {
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, "Vector");
        if (vector == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Normalize Vector3 requires a Vector3 Vector input.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3NormalizeUVE(*vector);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Normalize Vector3 rejected its input, zero length, or output capacity.");
        }
        return {};
    }

    return {};
}

[[nodiscard]] bool HasRequiredFloatInputsUVE(const ScriptIrInstructionUVE& instruction,
                                              const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId.rfind("math.float.", 0U) != 0U) {
        return true;
    }
    return FindNumberInputUVE(context, instruction.sourceNodeId, "A") != nullptr &&
           FindNumberInputUVE(context, instruction.sourceNodeId, "B") != nullptr;
}

[[nodiscard]] bool HasRequiredBooleanInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId == "logic.boolean.not") {
        return FindBooleanInputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId.rfind("logic.boolean.", 0U) != 0U) {
        return true;
    }
    return FindBooleanInputUVE(context, instruction.sourceNodeId, "A") != nullptr &&
           FindBooleanInputUVE(context, instruction.sourceNodeId, "B") != nullptr;
}

[[nodiscard]] bool HasRequiredVector3InputsUVE(const ScriptIrInstructionUVE& instruction,
                                                const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "math.vector3.make") {
        return FindNumberInputUVE(context, nodeId, "X") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Y") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Z") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector3.add" || instruction.nodeTypeId == "math.vector3.subtract" ||
        instruction.nodeTypeId == "math.vector3.cross" || instruction.nodeTypeId == "math.vector3.dot") {
        return FindVector3InputUVE(context, nodeId, "A") != nullptr &&
               FindVector3InputUVE(context, nodeId, "B") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector3.multiply") {
        return FindVector3InputUVE(context, nodeId, "Vector") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Scale") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector3.length" || instruction.nodeTypeId == "math.vector3.normalize") {
        return FindVector3InputUVE(context, nodeId, "Vector") != nullptr;
    }
    return true;
}

ScriptVmExecutionResultUVE ExecuteValidatedProgramUVE(const ScriptBytecodeProgramUVE& program,
                                                       ScriptVmExecutionContextUVE* context,
                                                       ScriptVmExecutionOptionsUVE options) {
    ScriptVmExecutionResultUVE result;
    if (program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) {
        result.status = ScriptVmStatusUVE::InvalidInstruction;
        result.diagnostics.push_back({0U, "Unsupported bytecode version."});
        return result;
    }
    if (options.instructionBudget > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        options.instructionBudget = ScriptBytecodeProgramUVE::kMaximumInstructionsUVE;
    }
    if (context == nullptr) {
        for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
            if (result.instructionsExecuted >= options.instructionBudget) {
                result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                result.diagnostics.push_back({index, "Instruction budget exceeded."});
                return result;
            }
            const ScriptIrInstructionKindUVE kind = program.instructions[index].kind;
            if (kind != ScriptIrInstructionKindUVE::ExecuteNode && kind != ScriptIrInstructionKindUVE::TransferValue) {
                result.status = ScriptVmStatusUVE::InvalidInstruction;
                result.diagnostics.push_back({index, "Invalid instruction kind."});
                return result;
            }
            ++result.instructionsExecuted;
        }
        return result;
    }

    std::vector<bool> completed(program.instructions.size(), false);
    std::size_t completedCount = 0U;
    while (completedCount < program.instructions.size()) {
        bool madeProgress = false;
        for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
            if (completed[index]) {
                continue;
            }
            const ScriptIrInstructionUVE& instruction = program.instructions[index];
            if (instruction.kind != ScriptIrInstructionKindUVE::ExecuteNode &&
                instruction.kind != ScriptIrInstructionKindUVE::TransferValue) {
                result.status = ScriptVmStatusUVE::InvalidInstruction;
                result.diagnostics.push_back({index, "Invalid instruction kind."});
                return result;
            }
            if (instruction.kind == ScriptIrInstructionKindUVE::TransferValue) {
                const ScriptVmValueBindingUVE* output =
                    FindBindingUVE(context->outputs, instruction.sourceNodeId, instruction.sourcePinName);
                if (output == nullptr) {
                    continue;
                }
                if (result.instructionsExecuted >= options.instructionBudget) {
                    result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                    result.diagnostics.push_back({index, "Instruction budget exceeded."});
                    return result;
                }
                if (!context->SetInputUVE(instruction.targetNodeId, instruction.targetPinName, output->value)) {
                    ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                        index, "TransferValue could not publish a typed input or input capacity was exhausted.");
                    failure.instructionsExecuted = result.instructionsExecuted;
                    return failure;
                }
            } else {
                const bool isVector3Node = instruction.nodeTypeId.rfind("math.vector3.", 0U) == 0U;
                const bool isFloatNode = instruction.nodeTypeId.rfind("math.float.", 0U) == 0U;
                const bool isBooleanNode = instruction.nodeTypeId.rfind("logic.boolean.", 0U) == 0U;
                if ((isVector3Node && !HasRequiredVector3InputsUVE(instruction, *context)) ||
                    (isFloatNode && !HasRequiredFloatInputsUVE(instruction, *context)) ||
                    (isBooleanNode && !HasRequiredBooleanInputsUVE(instruction, *context))) {
                    continue;
                }
                if (result.instructionsExecuted >= options.instructionBudget) {
                    result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                    result.diagnostics.push_back({index, "Instruction budget exceeded."});
                    return result;
                }
                ScriptVmExecutionResultUVE nodeResult;
                if (isVector3Node) {
                    nodeResult = ExecuteVector3NodeUVE(instruction, index, *context);
                } else if (isFloatNode) {
                    nodeResult = ExecuteFloatNodeUVE(instruction, index, *context);
                } else if (isBooleanNode) {
                    nodeResult = ExecuteBooleanNodeUVE(instruction, index, *context);
                }
                if (!nodeResult.IsSuccessUVE()) {
                    nodeResult.instructionsExecuted = result.instructionsExecuted;
                    return nodeResult;
                }
            }
            ++result.instructionsExecuted;
            completed[index] = true;
            ++completedCount;
            madeProgress = true;
        }
        if (!madeProgress) {
            for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
                if (!completed[index]) {
                    return MakeNodeFailureUVE(index, "VM could not resolve typed node dependencies.");
                }
            }
        }
    }
    return result;
}

} // namespace

bool ScriptVmExecutionContextUVE::SetInputUVE(const std::uint32_t nodeId, std::string pinName,
                                              ScriptVmValueUVE value) {
    if (pinName.empty()) {
        return false;
    }
    if (ScriptVmValueBindingUVE* existing = FindMutableBindingUVE(inputs, nodeId, pinName);
        existing != nullptr) {
        existing->value = std::move(value);
        return true;
    }
    if (inputs.size() >= kMaximumBindingsUVE) {
        return false;
    }
    inputs.push_back({nodeId, std::move(pinName), std::move(value)});
    return true;
}

bool ScriptVmExecutionContextUVE::SetOutputUVE(const std::uint32_t nodeId, std::string pinName,
                                               ScriptVmValueUVE value) {
    if (pinName.empty()) {
        return false;
    }
    if (ScriptVmValueBindingUVE* existing = FindMutableBindingUVE(outputs, nodeId, pinName);
        existing != nullptr) {
        existing->value = std::move(value);
        return true;
    }
    if (outputs.size() >= kMaximumBindingsUVE) {
        return false;
    }
    outputs.push_back({nodeId, std::move(pinName), std::move(value)});
    return true;
}

std::optional<ScriptVmValueUVE> ScriptVmExecutionContextUVE::FindInputUVE(
    const std::uint32_t nodeId, const std::string& pinName) const {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(inputs, nodeId, pinName);
    return binding == nullptr ? std::nullopt : std::optional<ScriptVmValueUVE>(binding->value);
}

std::optional<ScriptVmValueUVE> ScriptVmExecutionContextUVE::FindOutputUVE(
    const std::uint32_t nodeId, const std::string& pinName) const {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(outputs, nodeId, pinName);
    return binding == nullptr ? std::nullopt : std::optional<ScriptVmValueUVE>(binding->value);
}

void ScriptVmExecutionContextUVE::ClearOutputsUVE() noexcept {
    outputs.clear();
}

ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(const ScriptBytecodeProgramUVE& program,
                                                     ScriptVmExecutionOptionsUVE options) {
    return ExecuteValidatedProgramUVE(program, nullptr, options);
}

ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(const ScriptBytecodeProgramUVE& program,
                                                     ScriptVmExecutionContextUVE& context,
                                                     ScriptVmExecutionOptionsUVE options) {
    return ExecuteValidatedProgramUVE(program, &context, options);
}

} // namespace UVE::Scripting
