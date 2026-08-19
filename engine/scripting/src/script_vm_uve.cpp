// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vm_uve.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iterator>
#include <type_traits>
#include <utility>

namespace UVE::Scripting {
namespace {

[[nodiscard]] float RandomUnitFromSeedUVE(const float seed) noexcept {
    std::uint32_t state = std::bit_cast<std::uint32_t>(seed) ^ 0x9E3779B9U;
    state ^= state >> 16U;
    state *= 0x7FEB352DU;
    state ^= state >> 15U;
    state *= 0x846CA68BU;
    state ^= state >> 16U;
    return static_cast<float>(state) / 4294967296.0F;
}

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
[[nodiscard]] bool IsFiniteScriptVmValueUVE(const ScriptVmValueUVE& value) noexcept {
    return std::visit(
        [](const auto& typedValue) noexcept {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, float>) {
                return std::isfinite(typedValue);
            } else if constexpr (std::is_same_v<ValueType, bool>) {
                return true;
            } else if constexpr (std::is_same_v<ValueType, ScriptVector3ValueUVE>) {
                return std::isfinite(typedValue.value.x) && std::isfinite(typedValue.value.y) &&
                       std::isfinite(typedValue.value.z);
            } else if constexpr (std::is_same_v<ValueType, ScriptVector2ValueUVE>) {
                return std::isfinite(typedValue.value.x) && std::isfinite(typedValue.value.y);
            } else if constexpr (std::is_same_v<ValueType, ScriptRotationValueUVE>) {
                return Math::IsFiniteUVE(typedValue.value);
            } else if constexpr (std::is_same_v<ValueType, ScriptTransformValueUVE>) {
                return std::isfinite(typedValue.position.value.x) && std::isfinite(typedValue.position.value.y) &&
                       std::isfinite(typedValue.position.value.z) && Math::IsFiniteUVE(typedValue.rotation.value) &&
                       std::isfinite(typedValue.scale.value.x) && std::isfinite(typedValue.scale.value.y) &&
                       std::isfinite(typedValue.scale.value.z);
            } else {
                return typedValue.IsValidUVE();
            }
        },
        value);
}

[[nodiscard]] bool IsSupportedLocalVariableValueUVE(const ScriptVmValueUVE& value) noexcept {
    return std::holds_alternative<float>(value) || std::holds_alternative<bool>(value) ||
           std::holds_alternative<ScriptVector3ValueUVE>(value) ||
           std::holds_alternative<ScriptRotationValueUVE>(value) ||
           std::holds_alternative<ScriptTransformValueUVE>(value);
}

[[nodiscard]] ScriptVmLocalVariableUVE* FindMutableLocalVariableUVE(
    std::vector<ScriptVmLocalVariableUVE>& variables, const std::uint32_t slot) noexcept {
    const auto iterator = std::find_if(variables.begin(), variables.end(),
                                       [slot](const ScriptVmLocalVariableUVE& variable) {
                                           return variable.slot == slot;
                                       });
    return iterator == variables.end() ? nullptr : &*iterator;
}
[[nodiscard]] const ScriptVmLocalVariableUVE* FindLocalVariableRecordUVE(
    const std::vector<ScriptVmLocalVariableUVE>& variables, const std::uint32_t slot) noexcept {
    const auto iterator = std::find_if(variables.begin(), variables.end(),
                                       [slot](const ScriptVmLocalVariableUVE& variable) {
                                           return variable.slot == slot;
                                       });
    return iterator == variables.end() ? nullptr : &*iterator;
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

[[nodiscard]] const ScriptVector2ValueUVE* FindVector2InputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptVector2ValueUVE>(&binding->value);
}

[[nodiscard]] const ScriptRotationValueUVE* FindRotationInputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptRotationValueUVE>(&binding->value);
}

[[nodiscard]] const ScriptTransformValueUVE* FindTransformInputUVE(
    const ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId, const char* pinName) {
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, pinName);
    return binding == nullptr ? nullptr : std::get_if<ScriptTransformValueUVE>(&binding->value);
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

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteConversionNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "convert.number_to_boolean") {
        const float* value = FindNumberInputUVE(context, nodeId, "Value");
        if (value == nullptr || !std::isfinite(*value)) {
            return MakeNodeFailureUVE(instructionIndex, "Number to Boolean requires a finite Number Value.");
        }
        if (!SetNodeOutputUVE(context, nodeId, "Result", *value != 0.0F)) {
            return MakeNodeFailureUVE(instructionIndex, "Number to Boolean rejected its output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "convert.boolean_to_number") {
        const bool* value = FindBooleanInputUVE(context, nodeId, "Value");
        if (value == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Boolean to Number requires a Boolean Value.");
        }
        if (!SetNodeOutputUVE(context, nodeId, "Result", *value ? 1.0F : 0.0F)) {
            return MakeNodeFailureUVE(instructionIndex, "Boolean to Number rejected its output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "convert.vector2_to_vector3") {
        const ScriptVector2ValueUVE* vector = FindVector2InputUVE(context, nodeId, "Vector");
        const float* z = FindNumberInputUVE(context, nodeId, "Z");
        if (vector == nullptr || (z != nullptr && !std::isfinite(*z))) {
            return MakeNodeFailureUVE(instructionIndex, "Vector2 to Vector3 requires a finite Vector and optional finite Z.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3MakeUVE(
            vector->value.x, vector->value.y, z == nullptr ? 0.0F : *z);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Vector2 to Vector3 rejected its input or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "convert.vector3_to_vector2") {
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, "Vector");
        if (vector == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Vector3 to Vector2 requires a finite Vector input.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2MakeUVE(vector->value.x, vector->value.y);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Vector3 to Vector2 rejected its input or output capacity.");
        }
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown conversion node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteVariableNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const bool isNumber = instruction.nodeTypeId.ends_with("_number");
    const bool isBoolean = instruction.nodeTypeId.ends_with("_boolean");
    const bool isVector3 = instruction.nodeTypeId.ends_with("_vector3");
    const bool isMake = instruction.nodeTypeId.rfind("variable.make_", 0U) == 0U;
    const bool isSet = instruction.nodeTypeId.rfind("variable.set_", 0U) == 0U;
    const bool isGet = instruction.nodeTypeId.rfind("variable.get_", 0U) == 0U;
    if ((!isNumber && !isBoolean && !isVector3) || (!isMake && !isSet && !isGet)) {
        return {};
    }
    const float* slotInput = FindNumberInputUVE(context, nodeId, "Slot");
    if (slotInput == nullptr || !std::isfinite(*slotInput) || *slotInput < 0.0F ||
        *slotInput > static_cast<float>(ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE - 1U) ||
        std::floor(*slotInput) != *slotInput) {
        return MakeNodeFailureUVE(instructionIndex, "Variable node requires an integral finite Slot within local capacity.");
    }
    const std::uint32_t slot = static_cast<std::uint32_t>(*slotInput);
    const auto setResult = [&](ScriptVmValueUVE value) -> ScriptVmExecutionResultUVE {
        if (!context.SetOutputUVE(nodeId, "Result", value)) {
            return MakeNodeFailureUVE(instructionIndex, "Variable node rejected its Result output capacity.");
        }
        return {};
    };
    const auto rejectType = [&]() {
        return MakeNodeFailureUVE(instructionIndex, "Variable node found a missing or type-incompatible local slot.");
    };
    if (isNumber) {
        if (isMake) {
            const float* value = FindNumberInputUVE(context, nodeId, "Value");
            if (value == nullptr || !std::isfinite(*value)) {
                return MakeNodeFailureUVE(instructionIndex, "Make Number Variable requires a finite Value.");
            }
            ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(context.localVariables, slot);
            if (existing == nullptr) {
                if (context.localVariables.size() >= ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE ||
                    !context.InitializeLocalVariableUVE(slot, *value)) {
                    return MakeNodeFailureUVE(instructionIndex, "Make Number Variable rejected local capacity.");
                }
                return setResult(*value);
            }
            if (!std::holds_alternative<float>(existing->value)) {
                return rejectType();
            }
            return setResult(existing->value);
        }
        if (isSet) {
            const float* value = FindNumberInputUVE(context, nodeId, "Value");
            if (value == nullptr || !std::isfinite(*value)) {
                return MakeNodeFailureUVE(instructionIndex, "Set Number Variable requires a finite Value.");
            }
            if (!context.SetLocalVariableUVE(slot, *value)) {
                return rejectType();
            }
            return setResult(*value);
        }
        const auto value = context.FindLocalVariableUVE(slot);
        return value.has_value() && std::holds_alternative<float>(*value) ? setResult(*value) : rejectType();
    }
    if (isBoolean) {
        if (isMake) {
            const bool* value = FindBooleanInputUVE(context, nodeId, "Value");
            if (value == nullptr) {
                return MakeNodeFailureUVE(instructionIndex, "Make Boolean Variable requires a Boolean Value.");
            }
            ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(context.localVariables, slot);
            if (existing == nullptr) {
                if (context.localVariables.size() >= ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE ||
                    !context.InitializeLocalVariableUVE(slot, *value)) {
                    return MakeNodeFailureUVE(instructionIndex, "Make Boolean Variable rejected local capacity.");
                }
                return setResult(*value);
            }
            if (!std::holds_alternative<bool>(existing->value)) {
                return rejectType();
            }
            return setResult(existing->value);
        }
        if (isSet) {
            const bool* value = FindBooleanInputUVE(context, nodeId, "Value");
            if (value == nullptr || !context.SetLocalVariableUVE(slot, *value)) {
                return rejectType();
            }
            return setResult(*value);
        }
        const auto value = context.FindLocalVariableUVE(slot);
        return value.has_value() && std::holds_alternative<bool>(*value) ? setResult(*value) : rejectType();
    }
    if (isMake) {
        const ScriptVector3ValueUVE* value = FindVector3InputUVE(context, nodeId, "Value");
        if (value == nullptr || !std::isfinite(value->value.x) || !std::isfinite(value->value.y) ||
            !std::isfinite(value->value.z)) {
            return MakeNodeFailureUVE(instructionIndex, "Make Vector3 Variable requires a finite Value.");
        }
        ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(context.localVariables, slot);
        if (existing == nullptr) {
            if (context.localVariables.size() >= ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE ||
                !context.InitializeLocalVariableUVE(slot, *value)) {
                return MakeNodeFailureUVE(instructionIndex, "Make Vector3 Variable rejected local capacity.");
            }
            return setResult(*value);
        }
        return std::holds_alternative<ScriptVector3ValueUVE>(existing->value) ? setResult(existing->value) : rejectType();
    }
    if (isSet) {
        const ScriptVector3ValueUVE* value = FindVector3InputUVE(context, nodeId, "Value");
        if (value == nullptr || !context.SetLocalVariableUVE(slot, *value)) {
            return rejectType();
        }
        return setResult(*value);
    }
    const auto value = context.FindLocalVariableUVE(slot);
    return value.has_value() && std::holds_alternative<ScriptVector3ValueUVE>(*value) ? setResult(*value) : rejectType();
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteFloatNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    float result = 0.0F;
    const std::string& nodeTypeId = instruction.nodeTypeId;
    if (nodeTypeId == "math.float.abs" || nodeTypeId == "math.float.sin" ||
        nodeTypeId == "math.float.cos" || nodeTypeId == "math.float.tan" ||
        nodeTypeId == "math.float.sqrt") {
        const float* value = FindNumberInputUVE(context, nodeId, "Value");
        if (value == nullptr || !std::isfinite(*value)) {
            return MakeNodeFailureUVE(instructionIndex, "Float unary node requires a finite Number Value input.");
        }
        if (nodeTypeId == "math.float.abs") {
            result = std::fabs(*value);
        } else if (nodeTypeId == "math.float.sin") {
            result = std::sin(*value);
        } else if (nodeTypeId == "math.float.cos") {
            result = std::cos(*value);
        } else if (nodeTypeId == "math.float.tan") {
            result = std::tan(*value);
        } else {
            if (*value < 0.0F) {
                return MakeNodeFailureUVE(instructionIndex, "Sqrt Float requires a non-negative Value.");
            }
            result = std::sqrt(*value);
        }
    } else if (nodeTypeId == "math.float.clamp") {
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
    } else if (nodeTypeId == "math.float.lerp") {
        const float* lhs = FindNumberInputUVE(context, nodeId, "A");
        const float* rhs = FindNumberInputUVE(context, nodeId, "B");
        const float* alpha = FindNumberInputUVE(context, nodeId, "Alpha");
        if (lhs == nullptr || rhs == nullptr || alpha == nullptr || !std::isfinite(*lhs) ||
            !std::isfinite(*rhs) || !std::isfinite(*alpha)) {
            return MakeNodeFailureUVE(instructionIndex, "Lerp Float requires finite Number inputs A, B, and Alpha.");
        }
        result = *lhs + ((*rhs - *lhs) * *alpha);
    } else if (nodeTypeId == "math.float.remap") {
        const float* value = FindNumberInputUVE(context, nodeId, "Value");
        const float* fromMin = FindNumberInputUVE(context, nodeId, "FromMin");
        const float* fromMax = FindNumberInputUVE(context, nodeId, "FromMax");
        const float* toMin = FindNumberInputUVE(context, nodeId, "ToMin");
        const float* toMax = FindNumberInputUVE(context, nodeId, "ToMax");
        if (value == nullptr || fromMin == nullptr || fromMax == nullptr || toMin == nullptr ||
            toMax == nullptr || !std::isfinite(*value) || !std::isfinite(*fromMin) ||
            !std::isfinite(*fromMax) || !std::isfinite(*toMin) || !std::isfinite(*toMax)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Remap Float requires finite Number inputs Value, ranges, and targets.");
        }
        if (*fromMin == *fromMax) {
            return MakeNodeFailureUVE(instructionIndex, "Remap Float requires a non-zero source range.");
        }
        const float alpha = (*value - *fromMin) / (*fromMax - *fromMin);
        result = *toMin + ((*toMax - *toMin) * alpha);
    } else if (nodeTypeId == "math.float.random") {
        const float* seed = FindNumberInputUVE(context, nodeId, "Seed");
        if (seed == nullptr || !std::isfinite(*seed)) {
            return MakeNodeFailureUVE(instructionIndex, "Random Float requires a finite Number Seed.");
        }
        result = RandomUnitFromSeedUVE(*seed);
    } else if (nodeTypeId == "math.float.random_range") {
        const float* seed = FindNumberInputUVE(context, nodeId, "Seed");
        const float* minimum = FindNumberInputUVE(context, nodeId, "Min");
        const float* maximum = FindNumberInputUVE(context, nodeId, "Max");
        if (seed == nullptr || minimum == nullptr || maximum == nullptr || !std::isfinite(*seed) ||
            !std::isfinite(*minimum) || !std::isfinite(*maximum)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Random Range Float requires finite Number Seed, Min, and Max.");
        }
        if (*minimum > *maximum) {
            return MakeNodeFailureUVE(instructionIndex, "Random Range Float requires Min not greater than Max.");
        }
        result = *minimum + ((*maximum - *minimum) * RandomUnitFromSeedUVE(*seed));
    } else {
        const float* lhs = FindNumberInputUVE(context, nodeId, "A");
        const float* rhs = FindNumberInputUVE(context, nodeId, "B");
        if (lhs == nullptr || rhs == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Float binary node requires Number inputs A and B.");
        }
        if (!std::isfinite(*lhs) || !std::isfinite(*rhs)) {
            return MakeNodeFailureUVE(instructionIndex, "Float node rejected non-finite input.");
        }
        if (nodeTypeId == "math.float.add") {
            result = *lhs + *rhs;
        } else if (nodeTypeId == "math.float.subtract") {
            result = *lhs - *rhs;
        } else if (nodeTypeId == "math.float.multiply") {
            result = *lhs * *rhs;
        } else if (nodeTypeId == "math.float.divide") {
            if (std::fabs(*rhs) <= 1.0e-6F) {
                return MakeNodeFailureUVE(instructionIndex, "Divide Float rejected a zero-near divisor.");
            }
            result = *lhs / *rhs;
        } else if (nodeTypeId == "math.float.modulo") {
            if (std::fabs(*rhs) <= 1.0e-6F) {
                return MakeNodeFailureUVE(instructionIndex, "Modulo Float rejected a zero-near divisor.");
            }
            result = std::fmod(*lhs, *rhs);
        } else if (nodeTypeId == "math.float.min") {
            result = std::fmin(*lhs, *rhs);
        } else if (nodeTypeId == "math.float.max") {
            result = std::fmax(*lhs, *rhs);
        } else if (nodeTypeId == "math.float.power") {
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

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteVector2NodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "math.vector2.make") {
        const float* x = FindNumberInputUVE(context, nodeId, "X");
        const float* y = FindNumberInputUVE(context, nodeId, "Y");
        if (x == nullptr || y == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Make Vector2 requires Number inputs X and Y.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2MakeUVE(*x, *y);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Vector", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Make Vector2 rejected its inputs or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.add" || instruction.nodeTypeId == "math.vector2.subtract" ||
        instruction.nodeTypeId == "math.vector2.dot" || instruction.nodeTypeId == "math.vector2.distance") {
        const ScriptVector2ValueUVE* lhs = FindVector2InputUVE(context, nodeId, "A");
        const ScriptVector2ValueUVE* rhs = FindVector2InputUVE(context, nodeId, "B");
        if (lhs == nullptr || rhs == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Vector2 binary node requires Vector2 inputs A and B.");
        }
        if (instruction.nodeTypeId == "math.vector2.add" || instruction.nodeTypeId == "math.vector2.subtract") {
            const ScriptVector2ValueResultUVE evaluated =
                instruction.nodeTypeId == "math.vector2.add"
                    ? EvaluateScriptVector2AddUVE(*lhs, *rhs)
                    : EvaluateScriptVector2SubtractUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Vector2 binary node rejected its inputs or output capacity.");
            }
        } else if (instruction.nodeTypeId == "math.vector2.dot") {
            const ScriptVector2NumberResultUVE evaluated = EvaluateScriptVector2DotUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Dot Vector2 rejected its inputs or output capacity.");
            }
        } else {
            const ScriptVector2NumberResultUVE evaluated = EvaluateScriptVector2DistanceUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Distance", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Distance Vector2 rejected its inputs or output capacity.");
            }
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.multiply") {
        const ScriptVector2ValueUVE* vector = FindVector2InputUVE(context, nodeId, "Vector");
        const float* scale = FindNumberInputUVE(context, nodeId, "Scale");
        if (vector == nullptr || scale == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Multiply Vector2 requires Vector2 Vector and Number Scale inputs.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2MultiplyUVE(*vector, *scale);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Multiply Vector2 rejected its inputs or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.length") {
        const ScriptVector2ValueUVE* vector = FindVector2InputUVE(context, nodeId, "Vector");
        if (vector == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Length Vector2 requires a Vector2 Vector input.");
        }
        const ScriptVector2NumberResultUVE evaluated = EvaluateScriptVector2LengthUVE(*vector);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Length", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Length Vector2 rejected its input or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.normalize") {
        const ScriptVector2ValueUVE* vector = FindVector2InputUVE(context, nodeId, "Vector");
        if (vector == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Normalize Vector2 requires a Vector2 Vector input.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2NormalizeUVE(*vector);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Normalize Vector2 rejected its input, zero length, or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.direction") {
        const ScriptVector2ValueUVE* from = FindVector2InputUVE(context, nodeId, "From");
        const ScriptVector2ValueUVE* to = FindVector2InputUVE(context, nodeId, "To");
        if (from == nullptr || to == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Direction Vector2 requires Vector2 inputs From and To.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2DirectionUVE(*from, *to);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Direction Vector2 rejected its inputs, zero length, or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector2.lerp") {
        const ScriptVector2ValueUVE* lhs = FindVector2InputUVE(context, nodeId, "A");
        const ScriptVector2ValueUVE* rhs = FindVector2InputUVE(context, nodeId, "B");
        const float* alpha = FindNumberInputUVE(context, nodeId, "Alpha");
        if (lhs == nullptr || rhs == nullptr || alpha == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Lerp Vector2 requires Vector2 inputs A and B plus Number Alpha.");
        }
        const ScriptVector2ValueResultUVE evaluated = EvaluateScriptVector2LerpUVE(*lhs, *rhs, *alpha);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Lerp Vector2 rejected its inputs or output capacity.");
        }
        return {};
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteRotationNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& type = instruction.nodeTypeId;
    if (type == "math.rotation.make") {
        const ScriptVector3ValueUVE* axis = FindVector3InputUVE(context, nodeId, "Axis");
        const float* radians = FindNumberInputUVE(context, nodeId, "Radians");
        if (axis == nullptr || radians == nullptr) return MakeNodeFailureUVE(instructionIndex, "Make Rotation requires Axis and Radians.");
        const auto value = EvaluateScriptRotationMakeUVE(*axis, *radians);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Rotation", value.value)) return MakeNodeFailureUVE(instructionIndex, "Make Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.break") {
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        if (rotation == nullptr) return MakeNodeFailureUVE(instructionIndex, "Break Rotation requires a Rotation input.");
        const auto value = EvaluateScriptRotationBreakUVE(*rotation);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Axis", value.axis) ||
            !SetNodeOutputUVE(context, nodeId, "Radians", value.radians)) return MakeNodeFailureUVE(instructionIndex, "Break Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.degrees" || type == "math.rotation.radians") {
        const char* inputName = type == "math.rotation.degrees" ? "Radians" : "Degrees";
        const char* outputName = type == "math.rotation.degrees" ? "Degrees" : "Radians";
        const float* input = FindNumberInputUVE(context, nodeId, inputName);
        if (input == nullptr) return MakeNodeFailureUVE(instructionIndex, "Rotation angle conversion requires a Number input.");
        const auto value = type == "math.rotation.degrees" ? EvaluateScriptRotationDegreesUVE(*input)
                                                              : EvaluateScriptRotationRadiansUVE(*input);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, outputName, value.value)) return MakeNodeFailureUVE(instructionIndex, "Rotation angle conversion rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.euler") {
        const ScriptVector3ValueUVE* radians = FindVector3InputUVE(context, nodeId, "Radians");
        if (radians == nullptr) return MakeNodeFailureUVE(instructionIndex, "Euler Rotation requires Vector3 Radians.");
        const auto value = EvaluateScriptRotationEulerUVE(*radians);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Rotation", value.value)) return MakeNodeFailureUVE(instructionIndex, "Euler Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.quaternion") {
        const float* x = FindNumberInputUVE(context, nodeId, "X");
        const float* y = FindNumberInputUVE(context, nodeId, "Y");
        const float* z = FindNumberInputUVE(context, nodeId, "Z");
        const float* w = FindNumberInputUVE(context, nodeId, "W");
        if (x == nullptr || y == nullptr || z == nullptr || w == nullptr) return MakeNodeFailureUVE(instructionIndex, "Quaternion Rotation requires Number X, Y, Z, and W.");
        const auto value = EvaluateScriptRotationQuaternionUVE(*x, *y, *z, *w);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Rotation", value.value)) return MakeNodeFailureUVE(instructionIndex, "Quaternion Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.look_at") {
        const ScriptVector3ValueUVE* direction = FindVector3InputUVE(context, nodeId, "Direction");
        const ScriptVector3ValueUVE* up = FindVector3InputUVE(context, nodeId, "Up");
        if (direction == nullptr || up == nullptr) return MakeNodeFailureUVE(instructionIndex, "Look At Rotation requires Direction and Up Vector3 inputs.");
        const auto value = EvaluateScriptRotationLookAtUVE(*direction, *up);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Rotation", value.value)) return MakeNodeFailureUVE(instructionIndex, "Look At Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.slerp") {
        const ScriptRotationValueUVE* lhs = FindRotationInputUVE(context, nodeId, "A");
        const ScriptRotationValueUVE* rhs = FindRotationInputUVE(context, nodeId, "B");
        const float* alpha = FindNumberInputUVE(context, nodeId, "Alpha");
        if (lhs == nullptr || rhs == nullptr || alpha == nullptr) return MakeNodeFailureUVE(instructionIndex, "Slerp Rotation requires Rotations A/B and Number Alpha.");
        const auto value = EvaluateScriptRotationSlerpUVE(*lhs, *rhs, *alpha);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Slerp Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.rotation.rotate") {
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, "Vector");
        if (rotation == nullptr || vector == nullptr) return MakeNodeFailureUVE(instructionIndex, "Rotate Vector requires Rotation and Vector3 inputs.");
        const auto value = EvaluateScriptRotationRotateUVE(*rotation, *vector);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Rotate Vector rejected its input or output capacity.");
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown rotation node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteTransformNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& type = instruction.nodeTypeId;
    if (type == "math.transform.make") {
        const ScriptVector3ValueUVE* position = FindVector3InputUVE(context, nodeId, "Position");
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        const ScriptVector3ValueUVE* scale = FindVector3InputUVE(context, nodeId, "Scale");
        if (position == nullptr || rotation == nullptr || scale == nullptr) return MakeNodeFailureUVE(instructionIndex, "Make Transform requires Position, Rotation, and Scale inputs.");
        const auto value = EvaluateScriptTransformMakeUVE(*position, *rotation, *scale);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Transform", value.value)) return MakeNodeFailureUVE(instructionIndex, "Make Transform rejected its inputs or output capacity.");
        return {};
    }
    if (type == "math.transform.break") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        if (transform == nullptr) return MakeNodeFailureUVE(instructionIndex, "Break Transform requires a Transform input.");
        const auto value = EvaluateScriptTransformBreakUVE(*transform);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Position", value.value.position) ||
            !SetNodeOutputUVE(context, nodeId, "Rotation", value.value.rotation) ||
            !SetNodeOutputUVE(context, nodeId, "Scale", value.value.scale)) return MakeNodeFailureUVE(instructionIndex, "Break Transform rejected its input or output capacity.");
        return {};
    }
    if (type == "math.transform.get_position" || type == "math.transform.get_scale") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        if (transform == nullptr) return MakeNodeFailureUVE(instructionIndex, "Transform component access requires a Transform input.");
        const auto value = type == "math.transform.get_position" ? EvaluateScriptTransformGetPositionUVE(*transform)
                                                                    : EvaluateScriptTransformGetScaleUVE(*transform);
        const char* outputName = type == "math.transform.get_position" ? "Position" : "Scale";
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, outputName, value.value)) return MakeNodeFailureUVE(instructionIndex, "Transform component access rejected its input or output capacity.");
        return {};
    }
    if (type == "math.transform.get_rotation") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        if (transform == nullptr) return MakeNodeFailureUVE(instructionIndex, "Get Transform Rotation requires a Transform input.");
        const auto value = EvaluateScriptTransformGetRotationUVE(*transform);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Rotation", value.value)) return MakeNodeFailureUVE(instructionIndex, "Get Transform Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.transform.set_position" || type == "math.transform.set_scale") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        const ScriptVector3ValueUVE* valueInput = FindVector3InputUVE(context, nodeId, type == "math.transform.set_position" ? "Position" : "Scale");
        if (transform == nullptr || valueInput == nullptr) return MakeNodeFailureUVE(instructionIndex, "Set Transform component requires Transform and Vector3 inputs.");
        const auto value = type == "math.transform.set_position" ? EvaluateScriptTransformSetPositionUVE(*transform, *valueInput)
                                                                    : EvaluateScriptTransformSetScaleUVE(*transform, *valueInput);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Set Transform component rejected its input or output capacity.");
        return {};
    }
    if (type == "math.transform.set_rotation") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        if (transform == nullptr || rotation == nullptr) return MakeNodeFailureUVE(instructionIndex, "Set Transform Rotation requires Transform and Rotation inputs.");
        const auto value = EvaluateScriptTransformSetRotationUVE(*transform, *rotation);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Set Transform Rotation rejected its input or output capacity.");
        return {};
    }
    if (type == "math.transform.translate" || type == "math.transform.transform_point") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        const ScriptVector3ValueUVE* vector = FindVector3InputUVE(context, nodeId, type == "math.transform.translate" ? "Translation" : "Point");
        if (transform == nullptr || vector == nullptr) return MakeNodeFailureUVE(instructionIndex, "Transform vector operation requires Transform and Vector3 inputs.");
        if (type == "math.transform.translate") {
            const auto value = EvaluateScriptTransformTranslateUVE(*transform, *vector);
            if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Translate Transform rejected its input or output capacity.");
        } else {
            const auto value = EvaluateScriptTransformPointUVE(*transform, *vector);
            if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Transform Point rejected its input or output capacity.");
        }
        return {};
    }
    if (type == "math.transform.rotate") {
        const ScriptTransformValueUVE* transform = FindTransformInputUVE(context, nodeId, "Transform");
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        if (transform == nullptr || rotation == nullptr) return MakeNodeFailureUVE(instructionIndex, "Rotate Transform requires Transform and Rotation inputs.");
        const auto value = EvaluateScriptTransformRotateUVE(*transform, *rotation);
        if (!value.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", value.value)) return MakeNodeFailureUVE(instructionIndex, "Rotate Transform rejected its input or output capacity.");
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown transform node type.");
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
        instruction.nodeTypeId == "math.vector3.cross" || instruction.nodeTypeId == "math.vector3.dot" ||
        instruction.nodeTypeId == "math.vector3.distance") {
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
        } else if (instruction.nodeTypeId == "math.vector3.dot") {
            const ScriptVector3NumberResultUVE evaluated = EvaluateScriptVector3DotUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Dot Vector3 rejected its inputs or output capacity.");
            }
        } else {
            const ScriptVector3NumberResultUVE evaluated = EvaluateScriptVector3DistanceUVE(*lhs, *rhs);
            if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Distance", evaluated.value)) {
                return MakeNodeFailureUVE(instructionIndex, "Distance Vector3 rejected its inputs or output capacity.");
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
    if (instruction.nodeTypeId == "math.vector3.direction") {
        const ScriptVector3ValueUVE* from = FindVector3InputUVE(context, nodeId, "From");
        const ScriptVector3ValueUVE* to = FindVector3InputUVE(context, nodeId, "To");
        if (from == nullptr || to == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Direction Vector3 requires Vector3 inputs From and To.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3DirectionUVE(*from, *to);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Direction Vector3 rejected its inputs, zero length, or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "math.vector3.lerp") {
        const ScriptVector3ValueUVE* lhs = FindVector3InputUVE(context, nodeId, "A");
        const ScriptVector3ValueUVE* rhs = FindVector3InputUVE(context, nodeId, "B");
        const float* alpha = FindNumberInputUVE(context, nodeId, "Alpha");
        if (lhs == nullptr || rhs == nullptr || alpha == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Lerp Vector3 requires Vector3 inputs A and B plus Number Alpha.");
        }
        const ScriptVector3ValueResultUVE evaluated = EvaluateScriptVector3LerpUVE(*lhs, *rhs, *alpha);
        if (!evaluated.IsAppliedUVE() || !SetNodeOutputUVE(context, nodeId, "Result", evaluated.value)) {
            return MakeNodeFailureUVE(instructionIndex, "Lerp Vector3 rejected its inputs or output capacity.");
        }
        return {};
    }

    return {};
}

[[nodiscard]] bool HasRequiredVector2InputsUVE(const ScriptIrInstructionUVE& instruction,
                                                  const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "math.vector2.make") {
        return FindNumberInputUVE(context, nodeId, "X") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Y") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector2.add" || instruction.nodeTypeId == "math.vector2.subtract" ||
        instruction.nodeTypeId == "math.vector2.dot" || instruction.nodeTypeId == "math.vector2.distance") {
        return FindVector2InputUVE(context, nodeId, "A") != nullptr &&
               FindVector2InputUVE(context, nodeId, "B") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector2.multiply") {
        return FindVector2InputUVE(context, nodeId, "Vector") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Scale") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector2.direction") {
        return FindVector2InputUVE(context, nodeId, "From") != nullptr &&
               FindVector2InputUVE(context, nodeId, "To") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector2.lerp") {
        return FindVector2InputUVE(context, nodeId, "A") != nullptr &&
               FindVector2InputUVE(context, nodeId, "B") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Alpha") != nullptr;
    }
    return FindVector2InputUVE(context, nodeId, "Vector") != nullptr;
}

[[nodiscard]] bool HasRequiredConversionInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                     const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "convert.number_to_boolean") {
        return FindNumberInputUVE(context, nodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId == "convert.boolean_to_number") {
        return FindBooleanInputUVE(context, nodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId == "convert.vector2_to_vector3") {
        return FindVector2InputUVE(context, nodeId, "Vector") != nullptr;
    }
    if (instruction.nodeTypeId == "convert.vector3_to_vector2") {
        return FindVector3InputUVE(context, nodeId, "Vector") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredVariableInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                  const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId.rfind("variable.", 0U) != 0U) {
        return true;
    }
    if (FindNumberInputUVE(context, instruction.sourceNodeId, "Slot") == nullptr) {
        return false;
    }
    const bool requiresValue = instruction.nodeTypeId.rfind("variable.make_", 0U) == 0U ||
                               instruction.nodeTypeId.rfind("variable.set_", 0U) == 0U;
    if (!requiresValue || instruction.nodeTypeId.rfind("variable.get_", 0U) == 0U) {
        return true;
    }
    if (instruction.nodeTypeId.ends_with("_number")) {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    if (instruction.nodeTypeId.ends_with("_boolean")) {
        return FindBooleanInputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    return FindVector3InputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
}

[[nodiscard]] bool HasRequiredFloatInputsUVE(const ScriptIrInstructionUVE& instruction,
                                              const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId.rfind("math.float.", 0U) != 0U) {
        return true;
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& nodeTypeId = instruction.nodeTypeId;
    if (nodeTypeId == "math.float.abs" || nodeTypeId == "math.float.sin" ||
        nodeTypeId == "math.float.cos" || nodeTypeId == "math.float.tan" ||
        nodeTypeId == "math.float.sqrt" || nodeTypeId == "math.float.random") {
        return FindNumberInputUVE(context, nodeId, nodeTypeId == "math.float.random" ? "Seed" : "Value") != nullptr;
    }
    if (nodeTypeId == "math.float.clamp") {
        return FindNumberInputUVE(context, nodeId, "Value") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Min") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max") != nullptr;
    }
    if (nodeTypeId == "math.float.lerp") {
        return FindNumberInputUVE(context, nodeId, "A") != nullptr &&
               FindNumberInputUVE(context, nodeId, "B") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Alpha") != nullptr;
    }
    if (nodeTypeId == "math.float.remap") {
        return FindNumberInputUVE(context, nodeId, "Value") != nullptr &&
               FindNumberInputUVE(context, nodeId, "FromMin") != nullptr &&
               FindNumberInputUVE(context, nodeId, "FromMax") != nullptr &&
               FindNumberInputUVE(context, nodeId, "ToMin") != nullptr &&
               FindNumberInputUVE(context, nodeId, "ToMax") != nullptr;
    }
    if (nodeTypeId == "math.float.random_range") {
        return FindNumberInputUVE(context, nodeId, "Seed") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Min") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max") != nullptr;
    }
    return FindNumberInputUVE(context, nodeId, "A") != nullptr &&
           FindNumberInputUVE(context, nodeId, "B") != nullptr;
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

[[nodiscard]] bool HasRequiredRotationInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                   const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& type = instruction.nodeTypeId;
    if (type == "math.rotation.make") {
        return FindVector3InputUVE(context, nodeId, "Axis") != nullptr && FindNumberInputUVE(context, nodeId, "Radians") != nullptr;
    }
    if (type == "math.rotation.break") return FindRotationInputUVE(context, nodeId, "Rotation") != nullptr;
    if (type == "math.rotation.degrees") return FindNumberInputUVE(context, nodeId, "Radians") != nullptr;
    if (type == "math.rotation.radians") return FindNumberInputUVE(context, nodeId, "Degrees") != nullptr;
    if (type == "math.rotation.euler") return FindVector3InputUVE(context, nodeId, "Radians") != nullptr;
    if (type == "math.rotation.quaternion") {
        return FindNumberInputUVE(context, nodeId, "X") != nullptr && FindNumberInputUVE(context, nodeId, "Y") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Z") != nullptr && FindNumberInputUVE(context, nodeId, "W") != nullptr;
    }
    if (type == "math.rotation.look_at") {
        return FindVector3InputUVE(context, nodeId, "Direction") != nullptr && FindVector3InputUVE(context, nodeId, "Up") != nullptr;
    }
    if (type == "math.rotation.slerp") {
        return FindRotationInputUVE(context, nodeId, "A") != nullptr && FindRotationInputUVE(context, nodeId, "B") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Alpha") != nullptr;
    }
    if (type == "math.rotation.rotate") {
        return FindRotationInputUVE(context, nodeId, "Rotation") != nullptr && FindVector3InputUVE(context, nodeId, "Vector") != nullptr;
    }
    return true;
}

[[nodiscard]] bool HasRequiredTransformInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                     const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& type = instruction.nodeTypeId;
    if (type == "math.transform.make") {
        return FindVector3InputUVE(context, nodeId, "Position") != nullptr && FindRotationInputUVE(context, nodeId, "Rotation") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Scale") != nullptr;
    }
    if (type == "math.transform.break" || type == "math.transform.get_position" || type == "math.transform.get_rotation" ||
        type == "math.transform.get_scale") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr;
    }
    if (type == "math.transform.set_position") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr && FindVector3InputUVE(context, nodeId, "Position") != nullptr;
    }
    if (type == "math.transform.set_rotation" || type == "math.transform.rotate") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr && FindRotationInputUVE(context, nodeId, "Rotation") != nullptr;
    }
    if (type == "math.transform.set_scale") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr && FindVector3InputUVE(context, nodeId, "Scale") != nullptr;
    }
    if (type == "math.transform.translate") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr && FindVector3InputUVE(context, nodeId, "Translation") != nullptr;
    }
    if (type == "math.transform.transform_point") {
        return FindTransformInputUVE(context, nodeId, "Transform") != nullptr && FindVector3InputUVE(context, nodeId, "Point") != nullptr;
    }
    return true;
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
        instruction.nodeTypeId == "math.vector3.cross" || instruction.nodeTypeId == "math.vector3.dot" ||
        instruction.nodeTypeId == "math.vector3.distance") {
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
    if (instruction.nodeTypeId == "math.vector3.direction") {
        return FindVector3InputUVE(context, nodeId, "From") != nullptr &&
               FindVector3InputUVE(context, nodeId, "To") != nullptr;
    }
    if (instruction.nodeTypeId == "math.vector3.lerp") {
        return FindVector3InputUVE(context, nodeId, "A") != nullptr &&
               FindVector3InputUVE(context, nodeId, "B") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Alpha") != nullptr;
    }
    return true;
}

[[nodiscard]] bool ContainsControlFlowUVE(const ScriptBytecodeProgramUVE& program) noexcept {
    return std::any_of(program.instructions.begin(), program.instructions.end(), [](const ScriptIrInstructionUVE& instruction) {
        return instruction.kind == ScriptIrInstructionKindUVE::ConditionalJump ||
               instruction.kind == ScriptIrInstructionKindUVE::SequenceDispatch ||
               instruction.kind == ScriptIrInstructionKindUVE::FlowControlDispatch;
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
        if (instruction.kind == ScriptIrInstructionKindUVE::FlowControlDispatch) {
            if (instruction.trueTargetInstructionIndex > program.instructions.size() ||
                instruction.falseTargetInstructionIndex > program.instructions.size() ||
                instruction.defaultTargetInstructionIndex > program.instructions.size()) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "FlowControlDispatch target is outside the bytecode instruction range.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }

            const std::string& sourcePinName = instruction.sourcePinName;
            std::size_t target = instruction.defaultTargetInstructionIndex;
            std::string message;
            const auto flowFailure = [&](std::string failureMessage) {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(instructionIndex, std::move(failureMessage));
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            };
            if (instruction.nodeTypeId == "flow.return") {
                target = program.instructions.size();
                message = "Return terminated execution.";
            } else if (instruction.nodeTypeId == "flow.do_once") {
                if (sourcePinName == "Reset") {
                    if (!context.ResetDoOnceLatchUVE(instruction.sourceNodeId)) {
                        ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                            instructionIndex, "Do Once could not initialize its bounded latch state.");
                        failure.instructionsExecuted = result.instructionsExecuted;
                        failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                        return failure;
                    }
                    message = "Do Once latch reset.";
                } else {
                    if (!context.InitializeDoOnceLatchUVE(instruction.sourceNodeId)) {
                        ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                            instructionIndex, "Do Once exceeded its bounded latch-state capacity.");
                        failure.instructionsExecuted = result.instructionsExecuted;
                        failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                        return failure;
                    }
                    const bool shouldFire = context.TryConsumeDoOnceLatchUVE(instruction.sourceNodeId);
                    target = shouldFire ? instruction.trueTargetInstructionIndex
                                        : instruction.defaultTargetInstructionIndex;
                    message = shouldFire ? "Do Once fired Then." : "Do Once suppressed a repeated execution.";
                }
            } else if (instruction.nodeTypeId == "flow.gate") {
                if (sourcePinName == "Open" || sourcePinName == "Close") {
                    if (!context.SetGateStateUVE(instruction.sourceNodeId, sourcePinName == "Open")) {
                        ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                            instructionIndex, "Gate exceeded its bounded state capacity.");
                        failure.instructionsExecuted = result.instructionsExecuted;
                        failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                        return failure;
                    }
                    message = sourcePinName == "Open" ? "Gate opened." : "Gate closed.";
                } else {
                    if (!context.InitializeGateStateUVE(instruction.sourceNodeId)) {
                        ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                            instructionIndex, "Gate exceeded its bounded state capacity.");
                        failure.instructionsExecuted = result.instructionsExecuted;
                        failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                        return failure;
                    }
                    const std::optional<bool> open = context.FindGateStateUVE(instruction.sourceNodeId);
                    target = open.value_or(false) ? instruction.trueTargetInstructionIndex
                                                  : instruction.falseTargetInstructionIndex;
                    message = open.value_or(false) ? "Gate routed through Exit." : "Gate suppressed a closed input.";
                }
            } else if (instruction.nodeTypeId == "flow.switch") {
                const float* value = FindNumberInputUVE(context, instruction.sourceNodeId, "Value");
                if (value == nullptr || !std::isfinite(*value)) {
                    target = instruction.defaultTargetInstructionIndex;
                    message = "Switch selected Default because Value was unavailable or non-finite.";
                } else if (std::fabs(*value) <= 1.0e-6F) {
                    target = instruction.trueTargetInstructionIndex;
                    message = "Switch selected Case0.";
                } else {
                    target = instruction.falseTargetInstructionIndex;
                    message = "Switch selected Case1.";
                }
            } else if (instruction.nodeTypeId == "flow.event") {
                target = instruction.trueTargetInstructionIndex;
                message = "Event fired Then.";
            } else if (instruction.nodeTypeId == "flow.loop" || instruction.nodeTypeId == "flow.for_loop") {
                const float* count = FindNumberInputUVE(context, instruction.sourceNodeId, "Count");
                if (count == nullptr || !std::isfinite(*count) || *count < 0.0F ||
                    *count > static_cast<float>(ScriptVmExecutionContextUVE::kMaximumLoopIterationsUVE) ||
                    std::floor(*count) != *count) {
                    return flowFailure("Loop Count must be a finite non-negative integer within the bounded iteration limit.");
                }
                if (!context.InitializeLoopStateUVE(instruction.sourceNodeId)) {
                    return flowFailure("Loop exceeded its bounded state capacity.");
                }
                const ScriptVmLoopStateUVE state = context.FindLoopStateUVE(instruction.sourceNodeId).value_or(
                    ScriptVmLoopStateUVE{instruction.sourceNodeId, 0U, false});
                const std::uint32_t countValue = static_cast<std::uint32_t>(*count);
                if (countValue == 0U || (state.active && state.iteration >= countValue)) {
                    if (!context.SetLoopStateUVE(instruction.sourceNodeId, 0U, false)) {
                        return flowFailure("Loop could not reset its bounded state.");
                    }
                    target = instruction.falseTargetInstructionIndex;
                    message = "Loop completed.";
                } else {
                    if (instruction.nodeTypeId == "flow.for_loop" &&
                        !context.SetOutputUVE(instruction.sourceNodeId, "Index", static_cast<float>(state.iteration))) {
                        return flowFailure("For Loop could not publish its bounded Index output.");
                    }
                    if (!context.SetLoopStateUVE(instruction.sourceNodeId, state.iteration + 1U, true)) {
                        return flowFailure("Loop could not advance its bounded iteration state.");
                    }
                    target = instruction.trueTargetInstructionIndex;
                    message = instruction.nodeTypeId == "flow.for_loop" ? "For Loop dispatched Body." : "Loop dispatched Body.";
                }
            } else if (instruction.nodeTypeId == "flow.while_loop") {
                const bool* condition = FindBooleanInputUVE(context, instruction.sourceNodeId, "Condition");
                if (condition == nullptr) {
                    return flowFailure("While Loop requires a Boolean Condition input.");
                }
                if (!context.InitializeLoopStateUVE(instruction.sourceNodeId)) {
                    return flowFailure("While Loop exceeded its bounded state capacity.");
                }
                const ScriptVmLoopStateUVE state = context.FindLoopStateUVE(instruction.sourceNodeId).value_or(
                    ScriptVmLoopStateUVE{instruction.sourceNodeId, 0U, false});
                if (!*condition) {
                    if (!context.SetLoopStateUVE(instruction.sourceNodeId, 0U, false)) {
                        return flowFailure("While Loop could not reset its bounded state.");
                    }
                    target = instruction.falseTargetInstructionIndex;
                    message = "While Loop completed because Condition was false.";
                } else if (state.iteration >= ScriptVmExecutionContextUVE::kMaximumLoopIterationsUVE) {
                    return flowFailure("While Loop exceeded its bounded iteration limit.");
                } else {
                    if (!context.SetLoopStateUVE(instruction.sourceNodeId, state.iteration + 1U, true)) {
                        return flowFailure("While Loop could not advance its bounded iteration state.");
                    }
                    target = instruction.trueTargetInstructionIndex;
                    message = "While Loop dispatched Body.";
                }
            } else if (instruction.nodeTypeId == "flow.delay") {
                const float* frames = FindNumberInputUVE(context, instruction.sourceNodeId, "Frames");
                if (frames == nullptr || !std::isfinite(*frames) || *frames < 0.0F ||
                    *frames > static_cast<float>(ScriptVmExecutionContextUVE::kMaximumDelayFramesUVE) ||
                    std::floor(*frames) != *frames) {
                    return flowFailure("Delay Frames must be a finite non-negative integer within the bounded frame limit.");
                }
                if (!context.InitializeDelayStateUVE(instruction.sourceNodeId)) {
                    return flowFailure("Delay exceeded its bounded state capacity.");
                }
                const ScriptVmDelayStateUVE state = context.FindDelayStateUVE(instruction.sourceNodeId).value_or(
                    ScriptVmDelayStateUVE{instruction.sourceNodeId, 0U, false});
                const std::uint32_t frameCount = static_cast<std::uint32_t>(*frames);
                if (!state.armed && frameCount == 0U) {
                    target = instruction.trueTargetInstructionIndex;
                    message = "Delay dispatched Then immediately.";
                } else if (!state.armed) {
                    if (!context.SetDelayStateUVE(instruction.sourceNodeId, frameCount, true)) {
                        return flowFailure("Delay could not arm its bounded frame state.");
                    }
                    target = program.instructions.size();
                    message = "Delay yielded until the next runtime tick.";
                } else if (state.remainingFrames > 1U) {
                    if (!context.SetDelayStateUVE(instruction.sourceNodeId, state.remainingFrames - 1U, true)) {
                        return flowFailure("Delay could not advance its bounded frame state.");
                    }
                    target = program.instructions.size();
                    message = "Delay yielded while its bounded frame state remained active.";
                } else {
                    if (!context.SetDelayStateUVE(instruction.sourceNodeId, 0U, false)) {
                        return flowFailure("Delay could not complete its bounded frame state.");
                    }
                    target = instruction.trueTargetInstructionIndex;
                    message = "Delay dispatched Then.";
                }
            } else {
                ScriptVmExecutionResultUVE failure = MakeNodeFailureUVE(
                    instructionIndex, "Unknown FlowControlDispatch node type.");
                failure.instructionsExecuted = result.instructionsExecuted;
                failure.PrependTraceEventsUVE(std::move(result.trace), result.traceTruncated);
                return failure;
            }
            ++result.instructionsExecuted;
            result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                         Scene::kInvalidEntityUVE, instructionIndex,
                                         instruction.sourceNodeId, instruction.targetNodeId,
                                         instruction.nodeTypeId, std::move(message)});
            instructionIndex = target;
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

        const bool isVariableNode = instruction.nodeTypeId.rfind("variable.", 0U) == 0U;
        const bool isConversionNode = instruction.nodeTypeId.rfind("convert.", 0U) == 0U;
        const bool isVector2Node = instruction.nodeTypeId.rfind("math.vector2.", 0U) == 0U;
        const bool isVector3Node = instruction.nodeTypeId.rfind("math.vector3.", 0U) == 0U;
        const bool isRotationNode = instruction.nodeTypeId.rfind("math.rotation.", 0U) == 0U;
        const bool isTransformNode = instruction.nodeTypeId.rfind("math.transform.", 0U) == 0U;
        const bool isFloatNode = instruction.nodeTypeId.rfind("math.float.", 0U) == 0U;
        const bool isBooleanNode = instruction.nodeTypeId.rfind("logic.boolean.", 0U) == 0U;
        const bool isEntityQueryNode = instruction.nodeTypeId.rfind("query.entity.", 0U) == 0U;
        const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
        const bool isEngineGetTimeNode = instruction.nodeTypeId == "engine.get_time";
        if ((isVariableNode && !HasRequiredVariableInputsUVE(instruction, context)) ||
            (isConversionNode && !HasRequiredConversionInputsUVE(instruction, context)) ||
            (isVector2Node && !HasRequiredVector2InputsUVE(instruction, context)) ||
            (isVector3Node && !HasRequiredVector3InputsUVE(instruction, context)) ||
            (isRotationNode && !HasRequiredRotationInputsUVE(instruction, context)) ||
            (isTransformNode && !HasRequiredTransformInputsUVE(instruction, context)) ||
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
        if (isVariableNode) {
            nodeResult = ExecuteVariableNodeUVE(instruction, instructionIndex, context);
        } else if (isConversionNode) {
            nodeResult = ExecuteConversionNodeUVE(instruction, instructionIndex, context);
        } else if (isVector2Node) {
            nodeResult = ExecuteVector2NodeUVE(instruction, instructionIndex, context);
        } else if (isVector3Node) {
            nodeResult = ExecuteVector3NodeUVE(instruction, instructionIndex, context);
        } else if (isRotationNode) {
            nodeResult = ExecuteRotationNodeUVE(instruction, instructionIndex, context);
        } else if (isTransformNode) {
            nodeResult = ExecuteTransformNodeUVE(instruction, instructionIndex, context);
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
        std::vector<ScriptVmFlowControlLatchUVE> localLatches;
        std::vector<ScriptVmGateStateUVE> localGates;
        std::size_t index = 0U;
        while (index < program.instructions.size()) {
            if (result.instructionsExecuted >= options.instructionBudget) {
                result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
                result.diagnostics.push_back({index, "Instruction budget exceeded."});
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, 0U, 0U, {}, "Instruction budget exceeded."});
                return result;
            }
            const ScriptIrInstructionUVE& instruction = program.instructions[index];
            const ScriptIrInstructionKindUVE kind = instruction.kind;
            if (kind == ScriptIrInstructionKindUVE::FlowControlDispatch) {
                if (instruction.trueTargetInstructionIndex > program.instructions.size() ||
                    instruction.falseTargetInstructionIndex > program.instructions.size() ||
                    instruction.defaultTargetInstructionIndex > program.instructions.size()) {
                    result.status = ScriptVmStatusUVE::InvalidInstruction;
                    result.diagnostics.push_back({index, "FlowControlDispatch target is outside the bytecode instruction range."});
                    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                                 index, instruction.sourceNodeId, instruction.targetNodeId,
                                                 instruction.nodeTypeId, "FlowControlDispatch target is outside the bytecode instruction range."});
                    return result;
                }
                std::size_t target = instruction.defaultTargetInstructionIndex;
                std::string message;
                if (instruction.nodeTypeId == "flow.return") {
                    target = program.instructions.size();
                    message = "Return terminated execution.";
                } else if (instruction.nodeTypeId == "flow.do_once") {
                    auto latch = std::find_if(localLatches.begin(), localLatches.end(),
                                              [&](const ScriptVmFlowControlLatchUVE& value) {
                                                  return value.nodeId == instruction.sourceNodeId;
                                              });
                    if (latch == localLatches.end()) {
                        if (localLatches.size() >= ScriptVmExecutionContextUVE::kMaximumFlowControlLatchesUVE) {
                            result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                            result.diagnostics.push_back({index, "Do Once exceeded its bounded latch-state capacity."});
                            return result;
                        }
                        localLatches.push_back({instruction.sourceNodeId, false});
                        latch = std::prev(localLatches.end());
                    }
                    if (instruction.sourcePinName == "Reset") {
                        latch->fired = false;
                        message = "Do Once latch reset.";
                    } else if (!latch->fired) {
                        latch->fired = true;
                        target = instruction.trueTargetInstructionIndex;
                        message = "Do Once fired Then.";
                    } else {
                        message = "Do Once suppressed a repeated execution.";
                    }
                } else if (instruction.nodeTypeId == "flow.gate") {
                    auto gate = std::find_if(localGates.begin(), localGates.end(),
                                             [&](const ScriptVmGateStateUVE& value) {
                                                 return value.nodeId == instruction.sourceNodeId;
                                             });
                    if (gate == localGates.end()) {
                        if (localGates.size() >= ScriptVmExecutionContextUVE::kMaximumGateStatesUVE) {
                            result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                            result.diagnostics.push_back({index, "Gate exceeded its bounded state capacity."});
                            return result;
                        }
                        localGates.push_back({instruction.sourceNodeId, false});
                        gate = std::prev(localGates.end());
                    }
                    if (instruction.sourcePinName == "Open" || instruction.sourcePinName == "Close") {
                        gate->open = instruction.sourcePinName == "Open";
                        message = gate->open ? "Gate opened." : "Gate closed.";
                    } else if (gate->open) {
                        target = instruction.trueTargetInstructionIndex;
                        message = "Gate routed through Exit.";
                    } else {
                        target = instruction.falseTargetInstructionIndex;
                        message = "Gate suppressed a closed input.";
                    }
                } else if (instruction.nodeTypeId == "flow.switch") {
                    message = "Switch selected Default because Value was unavailable without a VM context.";
                } else if (instruction.nodeTypeId == "flow.event") {
                    target = instruction.trueTargetInstructionIndex;
                    message = "Event fired Then.";
                } else if (instruction.nodeTypeId == "flow.loop" || instruction.nodeTypeId == "flow.for_loop" ||
                           instruction.nodeTypeId == "flow.while_loop") {
                    result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                    result.diagnostics.push_back({index, "Loop nodes require a persistent VM context with their bounded data inputs."});
                    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                                 index, instruction.sourceNodeId, instruction.targetNodeId,
                                                 instruction.nodeTypeId, "Loop nodes require a persistent VM context with their bounded data inputs."});
                    return result;
                } else if (instruction.nodeTypeId == "flow.delay") {
                    result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                    result.diagnostics.push_back({index, "Delay requires a persistent VM context to preserve frame state."});
                    result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                                 index, instruction.sourceNodeId, instruction.targetNodeId,
                                                 instruction.nodeTypeId, "Delay requires a persistent VM context to preserve frame state."});
                    return result;
                } else {
                    result.status = ScriptVmStatusUVE::InvalidInstruction;
                    result.diagnostics.push_back({index, "Unknown FlowControlDispatch node type."});
                    return result;
                }
                ++result.instructionsExecuted;
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::NodeExecuted,
                                             Scene::kInvalidEntityUVE, index, instruction.sourceNodeId,
                                             instruction.targetNodeId, instruction.nodeTypeId, std::move(message)});
                index = target;
                continue;
            }
            if (kind == ScriptIrInstructionKindUVE::ExecuteNode &&
                (instruction.nodeTypeId == "engine.log" || instruction.nodeTypeId == "engine.get_time")) {
                result.status = ScriptVmStatusUVE::NodeExecutionFailed;
                result.diagnostics.push_back({index, "engine call requires a caller-owned execution context and binding."});
                result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Failed, Scene::kInvalidEntityUVE,
                                             index, instruction.sourceNodeId, 0U, instruction.nodeTypeId,
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
            result.AppendTraceEventUVE({kind == ScriptIrInstructionKindUVE::ExecuteNode
                                             ? ScriptVmTraceEventKindUVE::NodeExecuted
                                             : ScriptVmTraceEventKindUVE::ValueTransferred,
                                         Scene::kInvalidEntityUVE, index, instruction.sourceNodeId,
                                         instruction.targetNodeId, instruction.nodeTypeId, {}});
            ++index;
        }
        result.AppendTraceEventUVE({ScriptVmTraceEventKindUVE::Completed, Scene::kInvalidEntityUVE,
                                     result.instructionsExecuted, 0U, 0U, {}, {}});
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
                const bool isVariableNode = instruction.nodeTypeId.rfind("variable.", 0U) == 0U;
                const bool isConversionNode = instruction.nodeTypeId.rfind("convert.", 0U) == 0U;
                const bool isVector2Node = instruction.nodeTypeId.rfind("math.vector2.", 0U) == 0U;
                const bool isVector3Node = instruction.nodeTypeId.rfind("math.vector3.", 0U) == 0U;
                const bool isRotationNode = instruction.nodeTypeId.rfind("math.rotation.", 0U) == 0U;
                const bool isTransformNode = instruction.nodeTypeId.rfind("math.transform.", 0U) == 0U;
                const bool isFloatNode = instruction.nodeTypeId.rfind("math.float.", 0U) == 0U;
                const bool isBooleanNode = instruction.nodeTypeId.rfind("logic.boolean.", 0U) == 0U;
                const bool isEntityQueryNode = instruction.nodeTypeId.rfind("query.entity.", 0U) == 0U;
                const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
                const bool isEngineGetTimeNode = instruction.nodeTypeId == "engine.get_time";
                if ((isVariableNode && !HasRequiredVariableInputsUVE(instruction, *context)) ||
                    (isConversionNode && !HasRequiredConversionInputsUVE(instruction, *context)) ||
                    (isVector2Node && !HasRequiredVector2InputsUVE(instruction, *context)) ||
                    (isVector3Node && !HasRequiredVector3InputsUVE(instruction, *context)) ||
                    (isRotationNode && !HasRequiredRotationInputsUVE(instruction, *context)) ||
                    (isTransformNode && !HasRequiredTransformInputsUVE(instruction, *context)) ||
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
                if (isVariableNode) {
                    nodeResult = ExecuteVariableNodeUVE(instruction, index, *context);
                } else if (isConversionNode) {
                    nodeResult = ExecuteConversionNodeUVE(instruction, index, *context);
                } else if (isVector2Node) {
                    nodeResult = ExecuteVector2NodeUVE(instruction, index, *context);
                } else if (isVector3Node) {
                    nodeResult = ExecuteVector3NodeUVE(instruction, index, *context);
                } else if (isRotationNode) {
                    nodeResult = ExecuteRotationNodeUVE(instruction, index, *context);
                } else if (isTransformNode) {
                    nodeResult = ExecuteTransformNodeUVE(instruction, index, *context);
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

bool ScriptVmExecutionContextUVE::InitializeDoOnceLatchUVE(const std::uint32_t nodeId) {
    if (nodeId == 0U) {
        return false;
    }
    const auto iterator = std::find_if(flowControlLatches.begin(), flowControlLatches.end(),
                                       [nodeId](const ScriptVmFlowControlLatchUVE& latch) {
                                           return latch.nodeId == nodeId;
                                       });
    if (iterator != flowControlLatches.end()) {
        return true;
    }
    if (flowControlLatches.size() >= kMaximumFlowControlLatchesUVE) {
        return false;
    }
    flowControlLatches.push_back({nodeId, false});
    return true;
}

bool ScriptVmExecutionContextUVE::TryConsumeDoOnceLatchUVE(const std::uint32_t nodeId) {
    if (!InitializeDoOnceLatchUVE(nodeId)) {
        return false;
    }
    const auto iterator = std::find_if(flowControlLatches.begin(), flowControlLatches.end(),
                                       [nodeId](const ScriptVmFlowControlLatchUVE& latch) {
                                           return latch.nodeId == nodeId;
                                       });
    if (iterator == flowControlLatches.end() || iterator->fired) {
        return false;
    }
    iterator->fired = true;
    return true;
}

bool ScriptVmExecutionContextUVE::ResetDoOnceLatchUVE(const std::uint32_t nodeId) {
    const auto iterator = std::find_if(flowControlLatches.begin(), flowControlLatches.end(),
                                       [nodeId](const ScriptVmFlowControlLatchUVE& latch) {
                                           return latch.nodeId == nodeId;
                                       });
    if (iterator == flowControlLatches.end()) {
        return InitializeDoOnceLatchUVE(nodeId);
    }
    iterator->fired = false;
    return true;
}

std::optional<bool> ScriptVmExecutionContextUVE::FindDoOnceLatchUVE(const std::uint32_t nodeId) const {
    const auto iterator = std::find_if(flowControlLatches.cbegin(), flowControlLatches.cend(),
                                       [nodeId](const ScriptVmFlowControlLatchUVE& latch) {
                                           return latch.nodeId == nodeId;
                                       });
    return iterator == flowControlLatches.cend() ? std::nullopt : std::optional<bool>(iterator->fired);
}

bool ScriptVmExecutionContextUVE::InitializeGateStateUVE(const std::uint32_t nodeId) {
    if (nodeId == 0U) {
        return false;
    }
    const auto iterator = std::find_if(gateStates.begin(), gateStates.end(),
                                       [nodeId](const ScriptVmGateStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator != gateStates.end()) {
        return true;
    }
    if (gateStates.size() >= kMaximumGateStatesUVE) {
        return false;
    }
    gateStates.push_back({nodeId, false});
    return true;
}

bool ScriptVmExecutionContextUVE::SetGateStateUVE(const std::uint32_t nodeId, const bool open) {
    if (!InitializeGateStateUVE(nodeId)) {
        return false;
    }
    const auto iterator = std::find_if(gateStates.begin(), gateStates.end(),
                                       [nodeId](const ScriptVmGateStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator == gateStates.end()) {
        return false;
    }
    iterator->open = open;
    return true;
}

std::optional<bool> ScriptVmExecutionContextUVE::FindGateStateUVE(const std::uint32_t nodeId) const {
    const auto iterator = std::find_if(gateStates.cbegin(), gateStates.cend(),
                                       [nodeId](const ScriptVmGateStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    return iterator == gateStates.cend() ? std::nullopt : std::optional<bool>(iterator->open);
}

bool ScriptVmExecutionContextUVE::InitializeLoopStateUVE(const std::uint32_t nodeId) {
    if (nodeId == 0U) {
        return false;
    }
    const auto iterator = std::find_if(loopStates.begin(), loopStates.end(),
                                       [nodeId](const ScriptVmLoopStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator != loopStates.end()) {
        return true;
    }
    if (loopStates.size() >= kMaximumLoopStatesUVE) {
        return false;
    }
    loopStates.push_back({nodeId, 0U, false});
    return true;
}

bool ScriptVmExecutionContextUVE::SetLoopStateUVE(const std::uint32_t nodeId,
                                                  const std::uint32_t iteration,
                                                  const bool active) {
    if (iteration > kMaximumLoopIterationsUVE || !InitializeLoopStateUVE(nodeId)) {
        return false;
    }
    const auto iterator = std::find_if(loopStates.begin(), loopStates.end(),
                                       [nodeId](const ScriptVmLoopStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator == loopStates.end()) {
        return false;
    }
    iterator->iteration = iteration;
    iterator->active = active;
    return true;
}

std::optional<ScriptVmLoopStateUVE> ScriptVmExecutionContextUVE::FindLoopStateUVE(const std::uint32_t nodeId) const {
    const auto iterator = std::find_if(loopStates.cbegin(), loopStates.cend(),
                                       [nodeId](const ScriptVmLoopStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    return iterator == loopStates.cend() ? std::nullopt : std::optional<ScriptVmLoopStateUVE>(*iterator);
}

bool ScriptVmExecutionContextUVE::InitializeDelayStateUVE(const std::uint32_t nodeId) {
    if (nodeId == 0U) {
        return false;
    }
    const auto iterator = std::find_if(delayStates.begin(), delayStates.end(),
                                       [nodeId](const ScriptVmDelayStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator != delayStates.end()) {
        return true;
    }
    if (delayStates.size() >= kMaximumDelayStatesUVE) {
        return false;
    }
    delayStates.push_back({nodeId, 0U, false});
    return true;
}

bool ScriptVmExecutionContextUVE::SetDelayStateUVE(const std::uint32_t nodeId,
                                                   const std::uint32_t remainingFrames,
                                                   const bool armed) {
    if (remainingFrames > kMaximumDelayFramesUVE || !InitializeDelayStateUVE(nodeId)) {
        return false;
    }
    const auto iterator = std::find_if(delayStates.begin(), delayStates.end(),
                                       [nodeId](const ScriptVmDelayStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    if (iterator == delayStates.end()) {
        return false;
    }
    iterator->remainingFrames = remainingFrames;
    iterator->armed = armed;
    return true;
}

std::optional<ScriptVmDelayStateUVE> ScriptVmExecutionContextUVE::FindDelayStateUVE(const std::uint32_t nodeId) const {
    const auto iterator = std::find_if(delayStates.cbegin(), delayStates.cend(),
                                       [nodeId](const ScriptVmDelayStateUVE& state) {
                                           return state.nodeId == nodeId;
                                       });
    return iterator == delayStates.cend() ? std::nullopt : std::optional<ScriptVmDelayStateUVE>(*iterator);
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

bool ScriptVmExecutionContextUVE::InitializeLocalVariableUVE(const std::uint32_t slot,
                                                              ScriptVmValueUVE value) {
    if (slot >= kMaximumLocalVariablesUVE || !IsSupportedLocalVariableValueUVE(value) ||
        !IsFiniteScriptVmValueUVE(value)) {
        return false;
    }
    if (ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(localVariables, slot);
        existing != nullptr) {
        return existing->value.index() == value.index();
    }
    if (localVariables.size() >= kMaximumLocalVariablesUVE) {
        return false;
    }
    localVariables.push_back({slot, std::move(value)});
    return true;
}

bool ScriptVmExecutionContextUVE::SetLocalVariableUVE(const std::uint32_t slot,
                                                       ScriptVmValueUVE value) {
    if (slot >= kMaximumLocalVariablesUVE || !IsSupportedLocalVariableValueUVE(value) ||
        !IsFiniteScriptVmValueUVE(value)) {
        return false;
    }
    ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(localVariables, slot);
    if (existing == nullptr || existing->value.index() != value.index()) {
        return false;
    }
    existing->value = std::move(value);
    return true;
}

std::optional<ScriptVmValueUVE> ScriptVmExecutionContextUVE::FindLocalVariableUVE(
    const std::uint32_t slot) const {
    const ScriptVmLocalVariableUVE* variable = FindLocalVariableRecordUVE(localVariables, slot);
    return variable == nullptr ? std::nullopt : std::optional<ScriptVmValueUVE>(variable->value);
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
