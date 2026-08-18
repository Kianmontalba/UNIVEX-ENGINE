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
    result.diagnostics.push_back({instructionIndex, message});
    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                instructionIndex, 0U, 0U, {}, std::move(message)});
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

[[nodiscard]] const ScriptEntityValueUVE* FindEntityInputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptEntityValueUVE>(&binding->value);
}

[[nodiscard]] const ScriptComponentValueUVE* FindComponentInputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptComponentValueUVE>(&binding->value);
}

[[nodiscard]] bool SetNodeOutputUVE(ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId,
                                    const char* pinName, ScriptVmValueUVE value) {
    return context.SetOutputUVE(nodeId, pinName, std::move(value));
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteFloatNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    float result = 0.0F;
    if (instruction.nodeTypeId == "math.float.abs") {
        const float* value = FindNumberInputUVE(context, nodeId, "Value");
        if (value == nullptr || !std::isfinite(*value)) {
            return MakeNodeFailureUVE(instructionIndex, "Abs Float requires a finite Number Value input.");
        }
        result = std::fabs(*value);
    } else if (instruction.nodeTypeId == "math.float.clamp") {
        const float* value = FindNumberInputUVE(context, nodeId, "Value");
        const float* minimum = FindNumberInputUVE(context, nodeId, "Min");
        const float* maximum = FindNumberInputUVE(context, nodeId, "Max");
        if (value == nullptr || minimum == nullptr || maximum == nullptr ||
            !std::isfinite(*value) || !std::isfinite(*minimum) || !std::isfinite(*maximum)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Clamp Float requires finite Number inputs Value, Min, and Max.");
        }
        if (*minimum > *maximum) {
            return MakeNodeFailureUVE(instructionIndex, "Clamp Float requires Min not greater than Max.");
        }
        result = std::clamp(*value, *minimum, *maximum);
    } else {
        const float* lhs = FindNumberInputUVE(context, nodeId, "A");
        const float* rhs = FindNumberInputUVE(context, nodeId, "B");
        if (lhs == nullptr || rhs == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Float binary node requires Number inputs A and B.");
        }
        if (!std::isfinite(*lhs) || !std::isfinite(*rhs)) {
            return MakeNodeFailureUVE(instructionIndex, "Float node rejected non-finite input.");
        }
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
        } else if (instruction.nodeTypeId == "math.float.modulo") {
            if (std::fabs(*rhs) <= 1.0e-6F) {
                return MakeNodeFailureUVE(instructionIndex, "Modulo Float rejected a zero-near divisor.");
            }
            result = std::fmod(*lhs, *rhs);
        } else if (instruction.nodeTypeId == "math.float.min") {
            result = std::fmin(*lhs, *rhs);
        } else if (instruction.nodeTypeId == "math.float.max") {
            result = std::fmax(*lhs, *rhs);
        } else if (instruction.nodeTypeId == "math.float.power") {
            result = std::pow(*lhs, *rhs);
        } else {
            return {};
        }
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
    if (instruction.nodeTypeId == "logic.boolean.equal" ||
        instruction.nodeTypeId == "logic.boolean.not_equal" ||
        instruction.nodeTypeId == "logic.boolean.greater" ||
        instruction.nodeTypeId == "logic.boolean.less" ||
        instruction.nodeTypeId == "logic.boolean.greater_equal" ||
        instruction.nodeTypeId == "logic.boolean.less_equal") {
        const float* lhs = FindNumberInputUVE(context, nodeId, "A");
        const float* rhs = FindNumberInputUVE(context, nodeId, "B");
        if (lhs == nullptr || rhs == nullptr || !std::isfinite(*lhs) || !std::isfinite(*rhs)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Boolean comparison requires finite Number inputs A and B.");
        }
        bool result = false;
        if (instruction.nodeTypeId == "logic.boolean.equal") {
            result = *lhs == *rhs;
        } else if (instruction.nodeTypeId == "logic.boolean.not_equal") {
            result = *lhs != *rhs;
        } else if (instruction.nodeTypeId == "logic.boolean.greater") {
            result = *lhs > *rhs;
        } else if (instruction.nodeTypeId == "logic.boolean.less") {
            result = *lhs < *rhs;
        } else if (instruction.nodeTypeId == "logic.boolean.greater_equal") {
            result = *lhs >= *rhs;
        } else {
            result = *lhs <= *rhs;
        }
        if (!SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Boolean comparison rejected its output capacity.");
        }
        return {};
    }
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

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteEngineGetTimeNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr || bindings->getTime == nullptr) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "engine.get_time requires a caller-owned engine time binding.");
    }
    float seconds = 0.0F;
    if (!bindings->getTime(bindings->userData, &seconds)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "engine.get_time callback rejected the copied output request.");
    }
    if (!std::isfinite(seconds)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "engine.get_time callback returned a non-finite Number.");
    }
    if (!SetNodeOutputUVE(context, instruction.sourceNodeId, "Value", seconds)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "engine.get_time could not store its bounded Number output.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteEngineLogNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    const float* value = FindNumberInputUVE(context, instruction.sourceNodeId, "Value");
    if (value == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "engine.log requires a Number Value input.");
    }
    if (bindings == nullptr || bindings->log == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "engine.log requires a caller-owned engine call binding.");
    }
    if (!bindings->log(bindings->userData, *value)) {
        return MakeNodeFailureUVE(instructionIndex, "engine.log callback rejected the copied Number value.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteEntityQueryNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const ScriptEntityValueUVE* entity = FindEntityInputUVE(context, nodeId, "Entity");
    const ScriptComponentValueUVE* component = FindComponentInputUVE(context, nodeId, "Component");
    if (entity == nullptr || component == nullptr || !entity->IsValidUVE() || !component->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "Entity query node requires valid Entity and Component inputs.");
    }
    const std::optional<ScriptComponentValueUVE> fact =
        context.FindComponentFactUVE(entity->entity, component->componentTypeId);
    if (!fact.has_value()) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "Entity query node requires a supplied component fact.");
    }
    if (instruction.nodeTypeId == "query.entity.has_component") {
        if (!SetNodeOutputUVE(context, nodeId, "Result", fact->present)) {
            return MakeNodeFailureUVE(instructionIndex, "Has Component rejected its output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "query.entity.get_component") {
        if (!SetNodeOutputUVE(context, nodeId, "Result", *fact)) {
            return MakeNodeFailureUVE(instructionIndex, "Get Component rejected its output capacity.");
        }
        return {};
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
    if (instruction.nodeTypeId == "math.float.abs") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId == "math.float.clamp") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Value") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "Min") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "Max") != nullptr;
    }
    return FindNumberInputUVE(context, instruction.sourceNodeId, "A") != nullptr &&
           FindNumberInputUVE(context, instruction.sourceNodeId, "B") != nullptr;
}

[[nodiscard]] bool HasRequiredBooleanInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId == "logic.boolean.equal" ||
        instruction.nodeTypeId == "logic.boolean.not_equal" ||
        instruction.nodeTypeId == "logic.boolean.greater" ||
        instruction.nodeTypeId == "logic.boolean.less" ||
        instruction.nodeTypeId == "logic.boolean.greater_equal" ||
        instruction.nodeTypeId == "logic.boolean.less_equal") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "A") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "B") != nullptr;
    }
    if (instruction.nodeTypeId == "logic.boolean.not") {
        return FindBooleanInputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId.rfind("logic.boolean.", 0U) != 0U) {
        return true;
    }
    return FindBooleanInputUVE(context, instruction.sourceNodeId, "A") != nullptr &&
           FindBooleanInputUVE(context, instruction.sourceNodeId, "B") != nullptr;
}

[[nodiscard]] bool HasRequiredEntityQueryInputsUVE(
    const ScriptIrInstructionUVE& instruction, const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId.rfind("query.entity.", 0U) != 0U) {
        return true;
    }
    return FindEntityInputUVE(context, instruction.sourceNodeId, "Entity") != nullptr &&
           FindComponentInputUVE(context, instruction.sourceNodeId, "Component") != nullptr;
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

[[nodiscard]] bool ContainsControlFlowUVE(const ScriptBytecodeProgramUVE& program) noexcept {
    return std::any_of(program.instructions.begin(), program.instructions.end(), [](const ScriptIrInstructionUVE& instruction) {
        return instruction.kind == ScriptIrInstructionKindUVE::ConditionalJump ||
               instruction.kind == ScriptIrInstructionKindUVE::SequenceDispatch;
    });
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteControlFlowProgramUVE(
    const ScriptBytecodeProgramUVE& program, ScriptVmExecutionContextUVE& context,
    const ScriptVmExecutionOptionsUVE options) {
    ScriptVmExecutionResultUVE result;
    std::size_t instructionIndex = 0U;
    std::optional<std::size_t> sequenceContinuation;
    while (instructionIndex < program.instructions.size()) {
        if (result.instructionsExecuted >= options.instructionBudget) {
            result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
            result.diagnostics.push_back({instructionIndex, "Instruction budget exceeded."});
            result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                         instructionIndex, 0U, 0U, {}, "Instruction budget exceeded."});
            return result;
        }

        const ScriptIrInstructionUVE& instruction = program.instructions[instructionIndex];
        if (instruction.kind == ScriptIrInstructionKindUVE::SequenceDispatch) {
            if (instruction.firstTargetInstructionIndex > program.instructions.size() ||
                instruction.secondTargetInstructionIndex > program.instructions.size()) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "SequenceDispatch target is outside the bytecode instruction range.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            ++result.instructionsExecuted;
            result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                         Scene::kInvalidEntityUVE, instructionIndex,
                                         instruction.sourceNodeId, instruction.targetNodeId,
                                         instruction.nodeTypeId, "SequenceDispatch selected ordered execution targets."});
            if (instruction.firstTargetInstructionIndex == program.instructions.size()) {
                instructionIndex = instruction.secondTargetInstructionIndex;
            } else {
                sequenceContinuation = instruction.secondTargetInstructionIndex;
                instructionIndex = instruction.firstTargetInstructionIndex;
            }
            continue;
        }
        if (instruction.kind == ScriptIrInstructionKindUVE::ConditionalJump) {
            if (instruction.trueTargetInstructionIndex > program.instructions.size() ||
                instruction.falseTargetInstructionIndex > program.instructions.size()) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "ConditionalJump target is outside the bytecode instruction range.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            const char* conditionPin = instruction.sourcePinName.empty() ? "Condition" : instruction.sourcePinName.c_str();
            const bool* condition = FindBooleanInputUVE(context, instruction.sourceNodeId, conditionPin);
            if (condition == nullptr) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "ConditionalJump requires a Boolean condition input.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            ++result.instructionsExecuted;
            const std::size_t target = *condition ? instruction.trueTargetInstructionIndex
                                                   : instruction.falseTargetInstructionIndex;
            result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                         Scene::kInvalidEntityUVE, instructionIndex,
                                         instruction.sourceNodeId, instruction.targetNodeId,
                                         instruction.nodeTypeId,
                                         *condition ? "ConditionalJump evaluated true."
                                                    : "ConditionalJump evaluated false."});
            instructionIndex = target;
            continue;
        }

        if (instruction.kind == ScriptIrInstructionKindUVE::TransferValue) {
            const ScriptVmValueBindingUVE* output =
                FindBindingUVE(context.outputs, instruction.sourceNodeId, instruction.sourcePinName);
            if (output == nullptr) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "TransferValue requires a typed source output binding.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            if (!context.SetInputUVE(instruction.targetNodeId, instruction.targetPinName, output->value)) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "TransferValue could not publish a typed input or input capacity was exhausted.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            ++result.instructionsExecuted;
            result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::ValueTransferred,
                                         Scene::kInvalidEntityUVE, instructionIndex,
                                         instruction.sourceNodeId, instruction.targetNodeId, {}, {}});
            if (sequenceContinuation.has_value()) {
                instructionIndex = *sequenceContinuation;
                sequenceContinuation.reset();
            } else {
                ++instructionIndex;
            }
            continue;
        }

        if (instruction.kind != ScriptIrInstructionKindUVE::ExecuteNode) {
            ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(instructionIndex, "Invalid instruction kind.");
            failure.instructionsExecuted = result.instructionsExecuted;
            failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
            return failure;
        }

        const bool isVector3Node = instruction.nodeTypeId.rfind("math.vector3.", 0U) == 0U;
        const bool isFloatNode = instruction.nodeTypeId.rfind("math.float.", 0U) == 0U;
        const bool isBooleanNode = instruction.nodeTypeId.rfind("logic.boolean.", 0U) == 0U;
        const bool isEntityQueryNode = instruction.nodeTypeId.rfind("query.entity.", 0U) == 0U;
        const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
        const bool isEngineGetTimeNode = instruction.nodeTypeId == "engine.get_time";
        if ((isVector3Node && !HasRequiredVector3InputsUVE(instruction, context)) ||
            (isFloatNode && !HasRequiredFloatInputsUVE(instruction, context)) ||
            (isBooleanNode && !HasRequiredBooleanInputsUVE(instruction, context)) ||
            (isEntityQueryNode && !HasRequiredEntityQueryInputsUVE(instruction, context)) ||
            (isEngineLogNode && FindNumberInputUVE(context, instruction.sourceNodeId, "Value") == nullptr)) {
            ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                instructionIndex, "Control-flow execution could not resolve typed node inputs.");
            failure.instructionsExecuted = result.instructionsExecuted;
            failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
            return failure;
        }
        ScriptVmExecutionResultUVE nodeResult;
        if (isVector3Node) {
            nodeResult = ExecuteVector3NodeUVE(instruction, instructionIndex, context);
        } else if (isFloatNode) {
            nodeResult = ExecuteFloatNodeUVE(instruction, instructionIndex, context);
        } else if (isBooleanNode) {
            nodeResult = ExecuteBooleanNodeUVE(instruction, instructionIndex, context);
        } else if (isEntityQueryNode) {
            nodeResult = ExecuteEntityQueryNodeUVE(instruction, instructionIndex, context);
        } else if (isEngineLogNode) {
            nodeResult = ExecuteEngineLogNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isEngineGetTimeNode) {
            nodeResult = ExecuteEngineGetTimeNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        }
        if (!nodeResult.IsSuccessUVE()) {
            nodeResult.instructionsExecuted = result.instructionsExecuted;
            nodeResult.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
            return nodeResult;
        }
        ++result.instructionsExecuted;
        result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                     Scene::kInvalidEntityUVE, instructionIndex,
                                     instruction.sourceNodeId, instruction.targetNodeId,
                                     instruction.nodeTypeId, {}});
        if (sequenceContinuation.has_value()) {
            instructionIndex = *sequenceContinuation;
            sequenceContinuation.reset();
        } else {
            ++instructionIndex;
        }
    }
    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Completed, Scene::kInvalidEntityUVE,
                                 result.instructionsExecuted, 0U, 0U, {}, {}});
    return result;
}

ScriptVmExecutionResultUVE ExecuteValidatedProgramUVE(const ScriptBytecodeProgramUVE& program,
                                                       ScriptVmExecutionContextUVE* context,
                                                       ScriptVmExecutionOptionsUVE options) {
    ScriptVmExecutionResultUVE result;
    if (program.version != ScriptBytecodeProgramUVE::kLegacyVersionUVE &&
        program.version != ScriptBytecodeProgramUVE::kConditionalJumpVersionUVE &&
        program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) {
        result.status = ScriptVmStatusUVE::InvalidInstruction;
        result.diagnostics.push_back({0U, "Unsupported bytecode version."});
        result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                     0U, 0U, 0U, {}, "Unsupported bytecode version."});
        return result;
    }
    if (options.instructionBudget > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        options.instructionBudget = ScriptBytecodeProgramUVE::kMaximumInstructionsUVE;
    }
    if (context != nullptr && ContainsControlFlowUVE(program)) {
        return ExecuteControlFlowProgramUVE(program, *context, options);
    }
    if (context == nullptr) {
        for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
            if (result.instructionsExecuted >= options.instructionBudget) {
                result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                result.diagnostics.push_back({index, "Instruction budget exceeded."});
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, 0U, 0U, {}, "Instruction budget exceeded."});
                return result;
            }
            const ScriptIrInstructionKindUVE kind = program.instructions[index].kind;
            if (kind == ScriptIrInstructionKindUVE::ExecuteNode &&
                (program.instructions[index].nodeTypeId == "engine.log" ||
                 program.instructions[index].nodeTypeId == "engine.get_time")) {
                result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                result.diagnostics.push_back({index, "engine call requires a caller-owned execution context and binding."});
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, program.instructions[index].sourceNodeId, 0U,
                                             program.instructions[index].nodeTypeId,
                                             "engine call requires a caller-owned execution context and binding."});
                return result;
            }
            if (kind != ScriptIrInstructionKindUVE::ExecuteNode && kind != ScriptIrInstructionKindUVE::TransferValue) {
                result.status = ScriptVmStatusUVE::InvalidInstruction;
                result.diagnostics.push_back({index, "Invalid instruction kind."});
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, 0U, 0U, {}, "Invalid instruction kind."});
                return result;
            }
            ++result.instructionsExecuted;
            const ScriptIrInstructionUVE& instruction = program.instructions[index];
            result.AppendTraceEventUVE({kind == ScriptIrInstructionKindUVE::ExecuteNode
                                             ? ScriptVmTraceEventKindUVE::NodeExecuted
                                             : ScriptVmTraceEventKindUVE::ValueTransferred,
                                         Scene::kInvalidEntityUVE, index, instruction.sourceNodeId,
                                         instruction.targetNodeId, instruction.nodeTypeId, {}});
        }
        result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Completed, Scene::kInvalidEntityUVE,
                                     program.instructions.size(), 0U, 0U, {}, {}});
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
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, instruction.sourceNodeId, instruction.targetNodeId,
                                             instruction.nodeTypeId, "Invalid instruction kind."});
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
                    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                                 index, instruction.sourceNodeId, instruction.targetNodeId,
                                                 {}, "Instruction budget exceeded."});
                    return result;
                }
                if (!context->SetInputUVE(instruction.targetNodeId, instruction.targetPinName, output->value)) {
                    ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                        index, "TransferValue could not publish a typed input or input capacity was exhausted.");
                    failure.instructionsExecuted = result.instructionsExecuted;
                    failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                    return failure;
                }
                result.AppendTraceEventUVE({instruction.isStagedTransfer
                                                 ? ScriptVmTraceEventKindUVE::StagedValueTransferred
                                                 : ScriptVmTraceEventKindUVE::ValueTransferred,
                                             Scene::kInvalidEntityUVE, index, instruction.sourceNodeId,
                                             instruction.targetNodeId, {}, {}});
            } else {
                const bool isVector3Node = instruction.nodeTypeId.rfind("math.vector3.", 0U) == 0U;
                const bool isFloatNode = instruction.nodeTypeId.rfind("math.float.", 0U) == 0U;
                const bool isBooleanNode = instruction.nodeTypeId.rfind("logic.boolean.", 0U) == 0U;
                const bool isEntityQueryNode = instruction.nodeTypeId.rfind("query.entity.", 0U) == 0U;
                const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
                const bool isEngineGetTimeNode = instruction.nodeTypeId == "engine.get_time";
                if ((isVector3Node && !HasRequiredVector3InputsUVE(instruction, *context)) ||
                    (isFloatNode && !HasRequiredFloatInputsUVE(instruction, *context)) ||
                    (isBooleanNode && !HasRequiredBooleanInputsUVE(instruction, *context)) ||
                    (isEntityQueryNode && !HasRequiredEntityQueryInputsUVE(instruction, *context)) ||
                    (isEngineLogNode && FindNumberInputUVE(*context, instruction.sourceNodeId, "Value") == nullptr)) {
                    continue;
                }
                if (result.instructionsExecuted >= options.instructionBudget) {
                    result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                    result.diagnostics.push_back({index, "Instruction budget exceeded."});
                    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                                 index, instruction.sourceNodeId, instruction.targetNodeId,
                                                 instruction.nodeTypeId, "Instruction budget exceeded."});
                    return result;
                }
                ScriptVmExecutionResultUVE nodeResult;
                if (isVector3Node) {
                    nodeResult = ExecuteVector3NodeUVE(instruction, index, *context);
                } else if (isFloatNode) {
                    nodeResult = ExecuteFloatNodeUVE(instruction, index, *context);
                } else if (isBooleanNode) {
                    nodeResult = ExecuteBooleanNodeUVE(instruction, index, *context);
                } else if (isEntityQueryNode) {
                    nodeResult = ExecuteEntityQueryNodeUVE(instruction, index, *context);
                } else if (isEngineLogNode) {
                    nodeResult = ExecuteEngineLogNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isEngineGetTimeNode) {
                    nodeResult = ExecuteEngineGetTimeNodeUVE(instruction, index, *context, options.engineCallBindings);
                }
                if (!nodeResult.IsSuccessUVE()) {
                    nodeResult.instructionsExecuted = result.instructionsExecuted;
                    nodeResult.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                    return nodeResult;
                }
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                             Scene::kInvalidEntityUVE, index, instruction.sourceNodeId,
                                             instruction.targetNodeId, instruction.nodeTypeId, {}});
            }
            ++result.instructionsExecuted;
            completed[index] = true;
            ++completedCount;
            madeProgress = true;
        }
        if (!madeProgress) {
            for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
                if (!completed[index]) {
                    ScriptVmExecutionResultUVE failure =
                        MakeNodeFailureUVE(index, "VM could not resolve typed node dependencies.");
                    failure.instructionsExecuted = result.instructionsExecuted;
                    failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                    return failure;
                }
            }
        }
    }
    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Completed, Scene::kInvalidEntityUVE,
                                 result.instructionsExecuted, 0U, 0U, {}, {}});
    return result;
}

} // namespace

void ScriptVmExecutionResultUVE::AppendTraceEventUVE(ScriptVmTraceEventUVE event) {
    if (trace.size() >= kMaximumTraceEventsUVE) {
        traceTruncated = true;
        return;
    }
    if (event.nodeTypeId.size() > kMaximumTraceMessageBytesUVE) {
        event.nodeTypeId.resize(kMaximumTraceMessageBytesUVE);
    }
    if (event.message.size() > kMaximumTraceMessageBytesUVE) {
        event.message.resize(kMaximumTraceMessageBytesUVE);
    }
    trace.push_back(std::move(event));
}

void ScriptVmExecutionResultUVE::PrependTraceEventsUVE(std::vector<ScriptVmTraceEventUVE> prefix,
                                                       const bool prefixTruncated) {
    std::vector<ScriptVmTraceEventUVE> existing = std::move(trace);
    const bool existingTruncated = traceTruncated;
    trace.clear();
    traceTruncated = prefixTruncated || existingTruncated;
    for (ScriptVmTraceEventUVE& event : prefix) {
        AppendTraceEventUVE(std::move(event));
    }
    for (ScriptVmTraceEventUVE& event : existing) {
        AppendTraceEventUVE(std::move(event));
    }
}

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

bool ScriptVmExecutionContextUVE::SetComponentFactUVE(const Scene::EntityUVE entity,
                                                         std::string componentTypeId,
                                                         const bool present) {
    if (entity == Scene::kInvalidEntityUVE || componentTypeId.empty()) {
        return false;
    }
    const auto iterator = std::find_if(componentFacts.begin(), componentFacts.end(),
                                       [entity, &componentTypeId](const ScriptComponentValueUVE& fact) {
                                           return fact.entity == entity && fact.componentTypeId == componentTypeId;
                                       });
    if (iterator != componentFacts.end()) {
        iterator->present = present;
        return true;
    }
    if (componentFacts.size() >= kMaximumComponentFactsUVE) {
        return false;
    }
    componentFacts.push_back({entity, std::move(componentTypeId), present});
    return true;
}

std::optional<ScriptComponentValueUVE> ScriptVmExecutionContextUVE::FindComponentFactUVE(
    const Scene::EntityUVE entity, const std::string& componentTypeId) const {
    const auto iterator = std::find_if(componentFacts.cbegin(), componentFacts.cend(),
                                       [entity, &componentTypeId](const ScriptComponentValueUVE& fact) {
                                           return fact.entity == entity && fact.componentTypeId == componentTypeId;
                                       });
    return iterator == componentFacts.cend() ? std::nullopt
                                             : std::optional<ScriptComponentValueUVE>(*iterator);
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
