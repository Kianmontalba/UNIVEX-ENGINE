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
            } else if constexpr (std::is_same_v<ValueType, ScriptArrayValueUVE>) {
                return IsValidScriptArrayValueUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, ScriptMapValueUVE>) {
                return IsValidScriptMapValueUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, ScriptSetValueUVE>) {
                return IsValidScriptSetValueUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, ScriptStructValueUVE>) {
                return IsValidScriptStructValueUVE(typedValue);
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
           std::holds_alternative<ScriptTransformValueUVE>(value) ||
           std::holds_alternative<ScriptArrayValueUVE>(value) ||
           std::holds_alternative<ScriptMapValueUVE>(value) ||
           std::holds_alternative<ScriptSetValueUVE>(value) ||
           std::holds_alternative<ScriptStructValueUVE>(value);
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
    const bool isArray = instruction.nodeTypeId.ends_with("_array");
    const bool isMap = instruction.nodeTypeId.ends_with("_map");
    const bool isSetCollection = instruction.nodeTypeId.ends_with("_set");
    const bool isStruct = instruction.nodeTypeId.ends_with("_struct");
    const bool isMake = instruction.nodeTypeId.rfind("variable.make_", 0U) == 0U;
    const bool isSetOperation = instruction.nodeTypeId.rfind("variable.set_", 0U) == 0U;
    const bool isGet = instruction.nodeTypeId.rfind("variable.get_", 0U) == 0U;
    if ((!isNumber && !isBoolean && !isVector3 && !isArray && !isMap && !isSetCollection && !isStruct) ||
        (!isMake && !isSetOperation && !isGet)) {
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
    const auto executeCollection = [&]<typename CollectionType>(const char* label) {
        const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, nodeId, "Value");
        const CollectionType* input = binding == nullptr ? nullptr : std::get_if<CollectionType>(&binding->value);
        const auto isValid = [](const CollectionType& value) {
            if constexpr (std::is_same_v<CollectionType, ScriptArrayValueUVE>) {
                return IsValidScriptArrayValueUVE(value);
            } else if constexpr (std::is_same_v<CollectionType, ScriptMapValueUVE>) {
                return IsValidScriptMapValueUVE(value);
            } else if constexpr (std::is_same_v<CollectionType, ScriptSetValueUVE>) {
                return IsValidScriptSetValueUVE(value);
            } else {
                return IsValidScriptStructValueUVE(value);
            }
        };
        const auto makeMessage = [label](const char* prefix, const char* suffix) {
            return std::string{prefix} + label + " Variable " + suffix;
        };
        if (isMake) {
            if (input == nullptr || !isValid(*input)) {
                return MakeNodeFailureUVE(instructionIndex, makeMessage("Make ", "requires a valid Value."));
            }
            ScriptVmLocalVariableUVE* existing = FindMutableLocalVariableUVE(context.localVariables, slot);
            if (existing == nullptr) {
                if (context.localVariables.size() >= ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE ||
                    !context.InitializeLocalVariableUVE(slot, *input)) {
                    return MakeNodeFailureUVE(instructionIndex, makeMessage("Make ", "rejected local capacity."));
                }
                return setResult(*input);
            }
            return std::holds_alternative<CollectionType>(existing->value) ? setResult(existing->value) : rejectType();
        }
        if (isSetOperation) {
            if (input == nullptr || !isValid(*input) || !context.SetLocalVariableUVE(slot, *input)) {
                return MakeNodeFailureUVE(instructionIndex, makeMessage("Set ", "requires a valid Value and matching local slot."));
            }
            return setResult(*input);
        }
        const auto value = context.FindLocalVariableUVE(slot);
        return value.has_value() && std::holds_alternative<CollectionType>(*value) ? setResult(*value) : rejectType();
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
        if (isSetOperation) {
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
        if (isSetOperation) {
            const bool* value = FindBooleanInputUVE(context, nodeId, "Value");
            if (value == nullptr || !context.SetLocalVariableUVE(slot, *value)) {
                return rejectType();
            }
            return setResult(*value);
        }
        const auto value = context.FindLocalVariableUVE(slot);
        return value.has_value() && std::holds_alternative<bool>(*value) ? setResult(*value) : rejectType();
    }
    if (isVector3) {
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
        if (isSetOperation) {
            const ScriptVector3ValueUVE* value = FindVector3InputUVE(context, nodeId, "Value");
            if (value == nullptr || !context.SetLocalVariableUVE(slot, *value)) {
                return rejectType();
            }
            return setResult(*value);
        }
        const auto value = context.FindLocalVariableUVE(slot);
        return value.has_value() && std::holds_alternative<ScriptVector3ValueUVE>(*value) ? setResult(*value) : rejectType();
    }
    if (isArray) {
        return executeCollection.template operator()<ScriptArrayValueUVE>("Array");
    }
    if (isMap) {
        return executeCollection.template operator()<ScriptMapValueUVE>("Map");
    }
    if (isSetCollection) {
        return executeCollection.template operator()<ScriptSetValueUVE>("Set");
    }
    if (isStruct) {
        return executeCollection.template operator()<ScriptStructValueUVE>("Struct");
    }
    return {};
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

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteEntityNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Entity node requires caller-owned scene bindings.");
    }
    if (instruction.nodeTypeId == "entity.spawn") {
        if (bindings->spawnEntity == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Spawn Entity requires a caller-owned spawn binding.");
        }
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        if (!bindings->spawnEntity(bindings->userData, &entity) || entity == Scene::kInvalidEntityUVE ||
            !SetNodeOutputUVE(context, nodeId, "Result", ScriptEntityValueUVE{entity})) {
            return MakeNodeFailureUVE(instructionIndex, "Spawn Entity rejected its callback or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "entity.destroy") {
        const ScriptEntityValueUVE* entity = FindEntityInputUVE(context, nodeId, "Entity");
        if (entity == nullptr || !entity->IsValidUVE() || bindings->destroyEntity == nullptr ||
            !bindings->destroyEntity(bindings->userData, entity->entity) ||
            !SetNodeOutputUVE(context, nodeId, "Result", true)) {
            return MakeNodeFailureUVE(instructionIndex, "Destroy Entity rejected its input, callback, or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "entity.find") {
        const ScriptComponentValueUVE* component = FindComponentInputUVE(context, nodeId, "Component");
        if (component == nullptr || !component->IsValidUVE() || bindings->findEntityByComponent == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Find Entity requires a valid Component and find binding.");
        }
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        if (!bindings->findEntityByComponent(bindings->userData, *component, &entity) ||
            entity == Scene::kInvalidEntityUVE || !SetNodeOutputUVE(context, nodeId, "Result", ScriptEntityValueUVE{entity})) {
            return MakeNodeFailureUVE(instructionIndex, "Find Entity rejected its callback or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "entity.get_entity") {
        const float* handle = FindNumberInputUVE(context, nodeId, "Handle");
        if (handle == nullptr || !std::isfinite(*handle) || *handle < 0.0F || std::floor(*handle) != *handle ||
            bindings->getEntityByHandle == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Get Entity requires a finite non-negative integral Handle and binding.");
        }
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        if (!bindings->getEntityByHandle(bindings->userData, *handle, &entity) ||
            entity == Scene::kInvalidEntityUVE || !SetNodeOutputUVE(context, nodeId, "Result", ScriptEntityValueUVE{entity})) {
            return MakeNodeFailureUVE(instructionIndex, "Get Entity rejected its callback or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "entity.add_component" || instruction.nodeTypeId == "entity.remove_component") {
        const ScriptEntityValueUVE* entity = FindEntityInputUVE(context, nodeId, "Entity");
        const ScriptComponentValueUVE* component = FindComponentInputUVE(context, nodeId, "Component");
        const ScriptEntityComponentMutationFunctionUVE mutation = instruction.nodeTypeId == "entity.add_component"
            ? bindings->addComponent
            : bindings->removeComponent;
        if (entity == nullptr || !entity->IsValidUVE() || component == nullptr || !component->IsValidUVE() ||
            mutation == nullptr || !mutation(bindings->userData, entity->entity, *component) ||
            !SetNodeOutputUVE(context, nodeId, "Result", true)) {
            return MakeNodeFailureUVE(instructionIndex, "Entity component mutation rejected its inputs, callback, or output capacity.");
        }
        return {};
    }
    return {};
}

[[nodiscard]] bool TryGetIntegralNumberInputUVE(const ScriptVmExecutionContextUVE& context,
                                                const std::uint32_t nodeId, const char* pinName,
                                                const float maximum, std::uint32_t* outValue) {
    const float* input = FindNumberInputUVE(context, nodeId, pinName);
    if (input == nullptr || outValue == nullptr || !std::isfinite(*input) || *input < 0.0F ||
        *input > maximum || std::floor(*input) != *input) {
        return false;
    }
    *outValue = static_cast<std::uint32_t>(*input);
    return true;
}

[[nodiscard]] bool IsFiniteVector2ValueUVE(const ScriptVector2ValueUVE& value) noexcept {
    return std::isfinite(value.value.x) && std::isfinite(value.value.y);
}

[[nodiscard]] bool IsFiniteVector3ValueUVE(const ScriptVector3ValueUVE& value) noexcept {
    return std::isfinite(value.value.x) && std::isfinite(value.value.y) && std::isfinite(value.value.z);
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteInputNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Input node requires caller-owned input bindings.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "input.key_pressed" || instruction.nodeTypeId == "input.key_released" ||
        instruction.nodeTypeId == "input.key_down") {
        std::uint32_t keyToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Key", 1024.0F, &keyToken)) {
            return MakeNodeFailureUVE(instructionIndex, "Keyboard input requires a bounded integral Key token.");
        }
        const ScriptInputKeyQueryFunctionUVE query = instruction.nodeTypeId == "input.key_pressed"
            ? bindings->inputKeyPressed
            : instruction.nodeTypeId == "input.key_released" ? bindings->inputKeyReleased : bindings->inputKeyDown;
        if (query == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Keyboard input requires its caller-owned query binding.");
        }
        bool result = false;
        if (!query(bindings->userData, static_cast<float>(keyToken), &result) ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Keyboard input callback rejected the query or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "input.mouse_position") {
        if (bindings->inputMousePosition == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Mouse position requires a caller-owned query binding.");
        }
        ScriptVector2ValueUVE position{};
        if (!bindings->inputMousePosition(bindings->userData, &position) || !IsFiniteVector2ValueUVE(position) ||
            !SetNodeOutputUVE(context, nodeId, "Position", position)) {
            return MakeNodeFailureUVE(instructionIndex, "Mouse position callback rejected its finite output or capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "input.mouse_button") {
        std::uint32_t buttonToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Button", 64.0F, &buttonToken) ||
            bindings->inputMouseButton == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Mouse button requires a bounded Button token and binding.");
        }
        bool result = false;
        if (!bindings->inputMouseButton(bindings->userData, static_cast<float>(buttonToken), &result) ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Mouse button callback rejected the query or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "input.gamepad_button") {
        std::uint32_t gamepadToken = 0U;
        std::uint32_t buttonToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Gamepad", 3.0F, &gamepadToken) ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Button", 15.0F, &buttonToken) ||
            bindings->inputGamepadButton == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Gamepad button requires bounded Gamepad/Button tokens and binding.");
        }
        bool result = false;
        if (!bindings->inputGamepadButton(bindings->userData, static_cast<float>(gamepadToken),
                                          static_cast<float>(buttonToken), &result) ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Gamepad button callback rejected the query or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "input.get_axis") {
        std::uint32_t gamepadToken = 0U;
        std::uint32_t axisToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Gamepad", 3.0F, &gamepadToken) ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Axis", 5.0F, &axisToken) ||
            bindings->inputAxis == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Gamepad axis requires bounded Gamepad/Axis tokens and binding.");
        }
        float result = 0.0F;
        if (!bindings->inputAxis(bindings->userData, static_cast<float>(gamepadToken), static_cast<float>(axisToken), &result) ||
            !std::isfinite(result) || result < -1.0F || result > 1.0F ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Gamepad axis callback must return a finite normalized value and capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "input.get_action") {
        std::uint32_t actionToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Action", 65535.0F, &actionToken) ||
            bindings->inputAction == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Input action requires a bounded integral Action token and binding.");
        }
        bool result = false;
        if (!bindings->inputAction(bindings->userData, static_cast<float>(actionToken), &result) ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Input action callback rejected the query or output capacity.");
        }
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown Input node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteCameraNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Camera node requires caller-owned camera bindings.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (instruction.nodeTypeId == "camera.get_camera") {
        if (bindings->cameraGet == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Get Camera requires a caller-owned camera binding.");
        }
        Scene::EntityUVE camera = Scene::kInvalidEntityUVE;
        if (!bindings->cameraGet(bindings->userData, &camera) || camera == Scene::kInvalidEntityUVE ||
            !SetNodeOutputUVE(context, nodeId, "Result", ScriptEntityValueUVE{camera})) {
            return MakeNodeFailureUVE(instructionIndex, "Get Camera rejected its callback or output capacity.");
        }
        return {};
    }
    const ScriptEntityValueUVE* camera = FindEntityInputUVE(context, nodeId, "Camera");
    if (camera == nullptr || !camera->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex, "Camera control requires a valid Camera entity input.");
    }
    bool accepted = false;
    if (instruction.nodeTypeId == "camera.set_position") {
        const ScriptVector3ValueUVE* position = FindVector3InputUVE(context, nodeId, "Position");
        accepted = position != nullptr && IsFiniteVector3ValueUVE(*position) && bindings->cameraSetPosition != nullptr &&
                   bindings->cameraSetPosition(bindings->userData, camera->entity, *position);
    } else if (instruction.nodeTypeId == "camera.set_rotation") {
        const ScriptRotationValueUVE* rotation = FindRotationInputUVE(context, nodeId, "Rotation");
        accepted = rotation != nullptr && Math::IsFiniteUVE(rotation->value) && bindings->cameraSetRotation != nullptr &&
                   bindings->cameraSetRotation(bindings->userData, camera->entity, *rotation);
    } else if (instruction.nodeTypeId == "camera.look_at") {
        const ScriptVector3ValueUVE* target = FindVector3InputUVE(context, nodeId, "Target");
        accepted = target != nullptr && IsFiniteVector3ValueUVE(*target) && bindings->cameraLookAt != nullptr &&
                   bindings->cameraLookAt(bindings->userData, camera->entity, *target);
    } else if (instruction.nodeTypeId == "camera.set_fov") {
        const float* fov = FindNumberInputUVE(context, nodeId, "FOV");
        accepted = fov != nullptr && std::isfinite(*fov) && *fov > 0.0F && *fov < 180.0F &&
                   bindings->cameraSetFov != nullptr && bindings->cameraSetFov(bindings->userData, camera->entity, *fov);
    } else if (instruction.nodeTypeId == "camera.shake") {
        const float* amplitude = FindNumberInputUVE(context, nodeId, "Amplitude");
        const float* duration = FindNumberInputUVE(context, nodeId, "Duration");
        accepted = amplitude != nullptr && duration != nullptr && std::isfinite(*amplitude) && std::isfinite(*duration) &&
                   *amplitude >= 0.0F && *duration >= 0.0F && bindings->cameraShake != nullptr &&
                   bindings->cameraShake(bindings->userData, camera->entity, *amplitude, *duration);
    } else if (instruction.nodeTypeId == "camera.set_active") {
        const bool* active = FindBooleanInputUVE(context, nodeId, "Active");
        accepted = active != nullptr && bindings->cameraSetActive != nullptr &&
                   bindings->cameraSetActive(bindings->userData, camera->entity, *active);
    } else {
        return MakeNodeFailureUVE(instructionIndex, "Unknown Camera node type.");
    }
    if (!accepted || !SetNodeOutputUVE(context, nodeId, "Result", true)) {
        return MakeNodeFailureUVE(instructionIndex, "Camera control rejected its copied inputs, callback, or output capacity.");
    }
    return {};
}

[[nodiscard]] bool IsFiniteTransformValueUVE(const ScriptTransformValueUVE& value) noexcept {
    return IsFiniteVector3ValueUVE(value.position) && Math::IsFiniteUVE(value.rotation.value) &&
           IsFiniteVector3ValueUVE(value.scale);
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteAnimationNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Animation node requires caller-owned animation bindings.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const ScriptEntityValueUVE* actor = FindEntityInputUVE(context, nodeId, "Actor");
    if (actor == nullptr || !actor->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex, "Animation node requires a valid Actor entity input.");
    }
    const auto setAcceptedResult = [&]() -> ScriptVmExecutionResultUVE {
        return SetNodeOutputUVE(context, nodeId, "Result", true)
            ? ScriptVmExecutionResultUVE{}
            : MakeNodeFailureUVE(instructionIndex, "Animation node could not store its Boolean output.");
    };
    if (instruction.nodeTypeId == "animation.play" || instruction.nodeTypeId == "animation.stop") {
        std::uint32_t clipToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Clip", 65535.0F, &clipToken)) {
            return MakeNodeFailureUVE(instructionIndex, "Animation clip input must be a bounded integral token.");
        }
        float blendDuration = 0.0F;
        if (instruction.nodeTypeId == "animation.play") {
            const float* input = FindNumberInputUVE(context, nodeId, "Blend Duration");
            if (input == nullptr || !std::isfinite(*input) || *input < 0.0F) {
                return MakeNodeFailureUVE(instructionIndex, "Play Animation requires a finite non-negative Blend Duration.");
            }
            blendDuration = *input;
        }
        const ScriptAnimationClipControlFunctionUVE callback =
            instruction.nodeTypeId == "animation.play" ? bindings->animationPlay : bindings->animationStop;
        bool accepted = false;
        if (callback == nullptr || !callback(bindings->userData, actor->entity, static_cast<float>(clipToken),
                                             blendDuration, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Animation clip callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.pause") {
        std::uint32_t clipToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Clip", 65535.0F, &clipToken) ||
            bindings->animationPause == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Pause Animation requires a bounded Clip token and binding.");
        }
        bool accepted = false;
        if (!bindings->animationPause(bindings->userData, actor->entity, static_cast<float>(clipToken), &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Pause Animation callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.blend") {
        std::uint32_t clipAToken = 0U;
        std::uint32_t clipBToken = 0U;
        const float* weight = FindNumberInputUVE(context, nodeId, "Weight");
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Clip A", 65535.0F, &clipAToken) ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Clip B", 65535.0F, &clipBToken) ||
            weight == nullptr || !std::isfinite(*weight) || *weight < 0.0F || *weight > 1.0F ||
            bindings->animationBlend == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Blend Animation requires bounded clips, finite Weight, and binding.");
        }
        bool accepted = false;
        if (!bindings->animationBlend(bindings->userData, actor->entity, static_cast<float>(clipAToken),
                                      static_cast<float>(clipBToken), *weight, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Blend Animation callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.blend_space") {
        std::uint32_t blendSpaceToken = 0U;
        const float* x = FindNumberInputUVE(context, nodeId, "X");
        const float* y = FindNumberInputUVE(context, nodeId, "Y");
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Blend Space", 65535.0F, &blendSpaceToken) ||
            x == nullptr || y == nullptr || !std::isfinite(*x) || !std::isfinite(*y) ||
            bindings->animationBlendSpace == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Blend Space requires a bounded token, finite coordinates, and binding.");
        }
        bool accepted = false;
        if (!bindings->animationBlendSpace(bindings->userData, actor->entity, static_cast<float>(blendSpaceToken),
                                           *x, *y, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Blend Space callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.set_speed" || instruction.nodeTypeId == "animation.set_weight") {
        const float* value = FindNumberInputUVE(context, nodeId,
                                                instruction.nodeTypeId == "animation.set_speed" ? "Speed" : "Weight");
        const bool validValue = value != nullptr && std::isfinite(*value) &&
            (instruction.nodeTypeId == "animation.set_speed" ? *value > 0.0F : *value >= 0.0F && *value <= 1.0F);
        const ScriptAnimationScalarControlFunctionUVE callback =
            instruction.nodeTypeId == "animation.set_speed" ? bindings->animationSetSpeed : bindings->animationSetWeight;
        if (!validValue || callback == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Animation scalar control requires a finite bounded value and binding.");
        }
        bool accepted = false;
        if (!callback(bindings->userData, actor->entity, *value, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Animation scalar callback rejected its copied input.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.montage") {
        std::uint32_t montageToken = 0U;
        const float* weight = FindNumberInputUVE(context, nodeId, "Weight");
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Montage", 65535.0F, &montageToken) ||
            weight == nullptr || !std::isfinite(*weight) || *weight < 0.0F || *weight > 1.0F ||
            bindings->animationMontage == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Animation Montage requires a bounded token, finite Weight, and binding.");
        }
        bool accepted = false;
        if (!bindings->animationMontage(bindings->userData, actor->entity, static_cast<float>(montageToken), *weight, &accepted) ||
            !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Animation Montage callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "animation.get_current_animation") {
        if (bindings->animationGetCurrent == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Get Current Animation requires a caller-owned binding.");
        }
        float clipToken = 0.0F;
        if (!bindings->animationGetCurrent(bindings->userData, actor->entity, &clipToken) || !std::isfinite(clipToken) ||
            clipToken < 0.0F || clipToken > 65535.0F || std::floor(clipToken) != clipToken ||
            !SetNodeOutputUVE(context, nodeId, "Result", clipToken)) {
            return MakeNodeFailureUVE(instructionIndex, "Get Current Animation callback returned an invalid token or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "animation.is_playing") {
        std::uint32_t clipToken = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Clip", 65535.0F, &clipToken) ||
            bindings->animationIsPlaying == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Is Playing requires a bounded Clip token and binding.");
        }
        bool result = false;
        if (!bindings->animationIsPlaying(bindings->userData, actor->entity, static_cast<float>(clipToken), &result) ||
            !SetNodeOutputUVE(context, nodeId, "Result", result)) {
            return MakeNodeFailureUVE(instructionIndex, "Is Playing callback rejected its copied input or output capacity.");
        }
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown Animation node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteMotionQueryNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Motion Query node requires caller-owned Motion Query bindings.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const ScriptEntityValueUVE* actor = FindEntityInputUVE(context, nodeId, "Actor");
    if (actor == nullptr || !actor->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex, "Motion Query node requires a valid Actor entity input.");
    }
    const auto setAcceptedResult = [&]() -> ScriptVmExecutionResultUVE {
        return SetNodeOutputUVE(context, nodeId, "Result", true)
            ? ScriptVmExecutionResultUVE{}
            : MakeNodeFailureUVE(instructionIndex, "Motion Query node could not store its Boolean output.");
    };
    if (instruction.nodeTypeId == "motion.query.build") {
        const ScriptVector3ValueUVE* velocity = FindVector3InputUVE(context, nodeId, "Velocity");
        const ScriptVector3ValueUVE* facing = FindVector3InputUVE(context, nodeId, "Facing");
        const float* delta = FindNumberInputUVE(context, nodeId, "Delta");
        if (velocity == nullptr || facing == nullptr || delta == nullptr || !IsFiniteVector3ValueUVE(*velocity) ||
            !IsFiniteVector3ValueUVE(*facing) || !std::isfinite(*delta) || *delta <= 0.0F ||
            bindings->motionQueryBuild == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Build Motion Query requires finite vectors, positive Delta, and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQueryBuild(bindings->userData, actor->entity, *velocity, *facing, *delta, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Build Motion Query callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.search") {
        std::uint32_t maximumResults = 0U;
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Max Results", 4096.0F, &maximumResults) || maximumResults == 0U ||
            bindings->motionQuerySearch == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Search Motion Query requires a bounded positive Max Results and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQuerySearch(bindings->userData, actor->entity, static_cast<float>(maximumResults), &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Search Motion Query callback rejected its copied input.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.get_best_match") {
        if (bindings->motionQueryBestMatch == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Get Best Motion Match requires a caller-owned binding.");
        }
        float candidateIndex = 0.0F;
        if (!bindings->motionQueryBestMatch(bindings->userData, actor->entity, &candidateIndex) ||
            !std::isfinite(candidateIndex) || candidateIndex < 0.0F || candidateIndex > 4095.0F ||
            std::floor(candidateIndex) != candidateIndex || !SetNodeOutputUVE(context, nodeId, "Result", candidateIndex)) {
            return MakeNodeFailureUVE(instructionIndex, "Get Best Motion Match callback returned an invalid candidate or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "motion.query.set_trajectory") {
        const ScriptVector3ValueUVE* sample = FindVector3InputUVE(context, nodeId, "Sample");
        const float* offset = FindNumberInputUVE(context, nodeId, "Offset");
        if (sample == nullptr || offset == nullptr || !IsFiniteVector3ValueUVE(*sample) || !std::isfinite(*offset) ||
            bindings->motionQuerySetTrajectory == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Trajectory requires a finite Sample/Offset and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQuerySetTrajectory(bindings->userData, actor->entity, *sample, *offset, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Trajectory callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.set_pose") {
        const ScriptTransformValueUVE* pose = FindTransformInputUVE(context, nodeId, "Pose");
        if (pose == nullptr || !IsFiniteTransformValueUVE(*pose) || bindings->motionQuerySetPose == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Pose requires a finite Pose and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQuerySetPose(bindings->userData, actor->entity, *pose, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Pose callback rejected its copied input.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.set_velocity" || instruction.nodeTypeId == "motion.query.set_facing") {
        const char* pinName = instruction.nodeTypeId == "motion.query.set_velocity" ? "Velocity" : "Facing";
        const ScriptVector3ValueUVE* value = FindVector3InputUVE(context, nodeId, pinName);
        if (value == nullptr || !IsFiniteVector3ValueUVE(*value)) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Query vector control requires a finite value and binding.");
        }
        bool accepted = false;
        const ScriptMotionQueryVectorFunctionUVE callback = instruction.nodeTypeId == "motion.query.set_velocity"
            ? bindings->motionQuerySetVelocity : bindings->motionQuerySetFacing;
        if (callback == nullptr || !callback(bindings->userData, actor->entity, *value, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Query vector callback rejected its copied input.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.set_yaw") {
        const float* yaw = FindNumberInputUVE(context, nodeId, "Yaw");
        if (yaw == nullptr || !std::isfinite(*yaw) || bindings->motionQuerySetYaw == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Yaw requires a finite Yaw and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQuerySetYaw(bindings->userData, actor->entity, *yaw, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Set Motion Yaw callback rejected its copied input.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.transition") {
        std::uint32_t targetToken = 0U;
        const float* duration = FindNumberInputUVE(context, nodeId, "Duration");
        if (!TryGetIntegralNumberInputUVE(context, nodeId, "Target", 65535.0F, &targetToken) || duration == nullptr ||
            !std::isfinite(*duration) || *duration < 0.0F || bindings->motionQueryTransition == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Query Transition requires bounded Target, finite Duration, and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQueryTransition(bindings->userData, actor->entity, static_cast<float>(targetToken), *duration, &accepted) ||
            !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Query Transition callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    if (instruction.nodeTypeId == "motion.query.motion_warp") {
        const ScriptVector3ValueUVE* target = FindVector3InputUVE(context, nodeId, "Target");
        const float* weight = FindNumberInputUVE(context, nodeId, "Weight");
        if (target == nullptr || weight == nullptr || !IsFiniteVector3ValueUVE(*target) || !std::isfinite(*weight) ||
            *weight < 0.0F || *weight > 1.0F || bindings->motionQueryMotionWarp == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Warp requires a finite Target, bounded Weight, and binding.");
        }
        bool accepted = false;
        if (!bindings->motionQueryMotionWarp(bindings->userData, actor->entity, *target, *weight, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Motion Warp callback rejected its copied inputs.");
        }
        return setAcceptedResult();
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown Motion Query node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecutePhysicsNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    if (bindings == nullptr) {
        return MakeNodeFailureUVE(instructionIndex, "Physics node requires caller-owned physics bindings.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const auto setBooleanResult = [&](const bool result) -> ScriptVmExecutionResultUVE {
        return SetNodeOutputUVE(context, nodeId, "Result", result)
            ? ScriptVmExecutionResultUVE{}
            : MakeNodeFailureUVE(instructionIndex, "Physics node rejected its Boolean output capacity.");
    };
    const auto setCastOutputs = [&](const bool hit, const Scene::EntityUVE entity,
                                    const ScriptVector3ValueUVE& point, const ScriptVector3ValueUVE& normal,
                                    const float distance, const bool includeNormal) -> ScriptVmExecutionResultUVE {
        if (!SetNodeOutputUVE(context, nodeId, "Hit", hit) ||
            !SetNodeOutputUVE(context, nodeId, "Entity", ScriptEntityValueUVE{hit ? entity : Scene::kInvalidEntityUVE}) ||
            !SetNodeOutputUVE(context, nodeId, "Point", point) ||
            (includeNormal && !SetNodeOutputUVE(context, nodeId, "Normal", normal)) ||
            !SetNodeOutputUVE(context, nodeId, "Distance", distance)) {
            return MakeNodeFailureUVE(instructionIndex, "Physics query rejected its copied outputs or output capacity.");
        }
        return {};
    };
    const auto validCastOutput = [](const bool hit, const Scene::EntityUVE entity,
                                    const ScriptVector3ValueUVE& point, const ScriptVector3ValueUVE& normal,
                                    const float distance, const float maximumDistance) {
        return (!hit || entity != Scene::kInvalidEntityUVE) && IsFiniteVector3ValueUVE(point) &&
               IsFiniteVector3ValueUVE(normal) && std::isfinite(distance) && distance >= 0.0F &&
               distance <= maximumDistance;
    };
    if (instruction.nodeTypeId == "physics.raycast") {
        const ScriptVector3ValueUVE* origin = FindVector3InputUVE(context, nodeId, "Origin");
        const ScriptVector3ValueUVE* direction = FindVector3InputUVE(context, nodeId, "Direction");
        const float* maximumDistance = FindNumberInputUVE(context, nodeId, "Max Distance");
        std::uint32_t layerMask = 0U;
        if (origin == nullptr || direction == nullptr || maximumDistance == nullptr ||
            !IsFiniteVector3ValueUVE(*origin) || !IsFiniteVector3ValueUVE(*direction) ||
            !std::isfinite(*maximumDistance) || *maximumDistance <= 0.0F ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Layer Mask", 65535.0F, &layerMask) ||
            bindings->physicsRaycast == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Raycast requires finite vectors, positive Max Distance, a bounded Layer Mask, and binding.");
        }
        const ScriptEntityValueUVE* ignore = FindEntityInputUVE(context, nodeId, "Ignore");
        bool hit = false;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        ScriptVector3ValueUVE point{};
        ScriptVector3ValueUVE normal{};
        float distance = 0.0F;
        if (ignore != nullptr && ignore->entity == Scene::kInvalidEntityUVE) {
            return MakeNodeFailureUVE(instructionIndex, "Raycast Ignore must be a valid or omitted Entity value.");
        }
        if (!bindings->physicsRaycast(bindings->userData, *origin, *direction, *maximumDistance, layerMask,
                                      ignore == nullptr ? Scene::kInvalidEntityUVE : ignore->entity, &hit, &entity,
                                      &point, &normal, &distance) ||
            !validCastOutput(hit, entity, point, normal, distance, *maximumDistance)) {
            return MakeNodeFailureUVE(instructionIndex, "Raycast callback rejected its copied inputs or returned invalid facts.");
        }
        return setCastOutputs(hit, entity, point, normal, distance, true);
    }
    if (instruction.nodeTypeId == "physics.sphere_cast") {
        const ScriptVector3ValueUVE* origin = FindVector3InputUVE(context, nodeId, "Origin");
        const ScriptVector3ValueUVE* direction = FindVector3InputUVE(context, nodeId, "Direction");
        const float* radius = FindNumberInputUVE(context, nodeId, "Radius");
        const float* maximumDistance = FindNumberInputUVE(context, nodeId, "Max Distance");
        std::uint32_t layerMask = 0U;
        if (origin == nullptr || direction == nullptr || radius == nullptr || maximumDistance == nullptr ||
            !IsFiniteVector3ValueUVE(*origin) || !IsFiniteVector3ValueUVE(*direction) ||
            !std::isfinite(*radius) || *radius <= 0.0F || !std::isfinite(*maximumDistance) || *maximumDistance <= 0.0F ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Layer Mask", 65535.0F, &layerMask) ||
            bindings->physicsSphereCast == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Sphere Cast requires finite values, positive radius/distance, a bounded Layer Mask, and binding.");
        }
        const ScriptEntityValueUVE* ignore = FindEntityInputUVE(context, nodeId, "Ignore");
        bool hit = false;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        ScriptVector3ValueUVE point{};
        float distance = 0.0F;
        if (ignore != nullptr && ignore->entity == Scene::kInvalidEntityUVE) {
            return MakeNodeFailureUVE(instructionIndex, "Sphere Cast Ignore must be a valid or omitted Entity value.");
        }
        if (!bindings->physicsSphereCast(bindings->userData, *origin, *direction, *radius, *maximumDistance, layerMask,
                                         ignore == nullptr ? Scene::kInvalidEntityUVE : ignore->entity, &hit, &entity,
                                         &point, &distance) ||
            !validCastOutput(hit, entity, point, ScriptVector3ValueUVE{}, distance, *maximumDistance)) {
            return MakeNodeFailureUVE(instructionIndex, "Sphere Cast callback rejected its copied inputs or returned invalid facts.");
        }
        return setCastOutputs(hit, entity, point, ScriptVector3ValueUVE{}, distance, false);
    }
    if (instruction.nodeTypeId == "physics.box_cast") {
        const ScriptVector3ValueUVE* origin = FindVector3InputUVE(context, nodeId, "Origin");
        const ScriptVector3ValueUVE* halfExtents = FindVector3InputUVE(context, nodeId, "Half Extents");
        const ScriptVector3ValueUVE* direction = FindVector3InputUVE(context, nodeId, "Direction");
        const float* maximumDistance = FindNumberInputUVE(context, nodeId, "Max Distance");
        std::uint32_t layerMask = 0U;
        if (origin == nullptr || halfExtents == nullptr || direction == nullptr || maximumDistance == nullptr ||
            !IsFiniteVector3ValueUVE(*origin) || !IsFiniteVector3ValueUVE(*halfExtents) ||
            !IsFiniteVector3ValueUVE(*direction) || halfExtents->value.x <= 0.0F || halfExtents->value.y <= 0.0F ||
            halfExtents->value.z <= 0.0F || !std::isfinite(*maximumDistance) || *maximumDistance <= 0.0F ||
            !TryGetIntegralNumberInputUVE(context, nodeId, "Layer Mask", 65535.0F, &layerMask) ||
            bindings->physicsBoxCast == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Box Cast requires finite positive extents/distance, a bounded Layer Mask, and binding.");
        }
        const ScriptEntityValueUVE* ignore = FindEntityInputUVE(context, nodeId, "Ignore");
        bool hit = false;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        ScriptVector3ValueUVE point{};
        float distance = 0.0F;
        if (ignore != nullptr && ignore->entity == Scene::kInvalidEntityUVE) {
            return MakeNodeFailureUVE(instructionIndex, "Box Cast Ignore must be a valid or omitted Entity value.");
        }
        if (!bindings->physicsBoxCast(bindings->userData, *origin, *halfExtents, *direction, *maximumDistance,
                                      layerMask, ignore == nullptr ? Scene::kInvalidEntityUVE : ignore->entity, &hit,
                                      &entity, &point, &distance) ||
            !validCastOutput(hit, entity, point, ScriptVector3ValueUVE{}, distance, *maximumDistance)) {
            return MakeNodeFailureUVE(instructionIndex, "Box Cast callback rejected its copied inputs or returned invalid facts.");
        }
        return setCastOutputs(hit, entity, point, ScriptVector3ValueUVE{}, distance, false);
    }
    if (instruction.nodeTypeId == "physics.capsule_cast") {
        const ScriptVector3ValueUVE* origin = FindVector3InputUVE(context, nodeId, "Origin");
        const ScriptVector3ValueUVE* direction = FindVector3InputUVE(context, nodeId, "Direction");
        const float* radius = FindNumberInputUVE(context, nodeId, "Radius");
        const float* halfHeight = FindNumberInputUVE(context, nodeId, "Half Height");
        const float* maximumDistance = FindNumberInputUVE(context, nodeId, "Max Distance");
        std::uint32_t layerMask = 0U;
        if (origin == nullptr || direction == nullptr || radius == nullptr || halfHeight == nullptr || maximumDistance == nullptr ||
            !IsFiniteVector3ValueUVE(*origin) || !IsFiniteVector3ValueUVE(*direction) || !std::isfinite(*radius) ||
            *radius <= 0.0F || !std::isfinite(*halfHeight) || *halfHeight <= 0.0F || !std::isfinite(*maximumDistance) ||
            *maximumDistance <= 0.0F || !TryGetIntegralNumberInputUVE(context, nodeId, "Layer Mask", 65535.0F, &layerMask) ||
            bindings->physicsCapsuleCast == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Capsule Cast requires finite positive shape/distance values, a bounded Layer Mask, and binding.");
        }
        const ScriptEntityValueUVE* ignore = FindEntityInputUVE(context, nodeId, "Ignore");
        bool hit = false;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        ScriptVector3ValueUVE point{};
        float distance = 0.0F;
        if (ignore != nullptr && ignore->entity == Scene::kInvalidEntityUVE) {
            return MakeNodeFailureUVE(instructionIndex, "Capsule Cast Ignore must be a valid or omitted Entity value.");
        }
        if (!bindings->physicsCapsuleCast(bindings->userData, *origin, *direction, *radius, *halfHeight,
                                          *maximumDistance, layerMask, ignore == nullptr ? Scene::kInvalidEntityUVE : ignore->entity,
                                          &hit, &entity, &point, &distance) ||
            !validCastOutput(hit, entity, point, ScriptVector3ValueUVE{}, distance, *maximumDistance)) {
            return MakeNodeFailureUVE(instructionIndex, "Capsule Cast callback rejected its copied inputs or returned invalid facts.");
        }
        return setCastOutputs(hit, entity, point, ScriptVector3ValueUVE{}, distance, false);
    }
    if (instruction.nodeTypeId == "physics.overlap") {
        const ScriptVector3ValueUVE* origin = FindVector3InputUVE(context, nodeId, "Origin");
        const ScriptVector3ValueUVE* halfExtents = FindVector3InputUVE(context, nodeId, "Half Extents");
        std::uint32_t layerMask = 0U;
        std::uint32_t count = 0U;
        if (origin == nullptr || halfExtents == nullptr || !IsFiniteVector3ValueUVE(*origin) ||
            !IsFiniteVector3ValueUVE(*halfExtents) || halfExtents->value.x <= 0.0F || halfExtents->value.y <= 0.0F ||
            halfExtents->value.z <= 0.0F || !TryGetIntegralNumberInputUVE(context, nodeId, "Layer Mask", 65535.0F, &layerMask) ||
            bindings->physicsOverlap == nullptr || !bindings->physicsOverlap(bindings->userData, *origin, *halfExtents, layerMask, &count) ||
            count > 4096U || !SetNodeOutputUVE(context, nodeId, "Count", static_cast<float>(count))) {
            return MakeNodeFailureUVE(instructionIndex, "Overlap requires finite positive extents, a bounded Layer Mask, and a bounded callback count.");
        }
        return {};
    }
    const ScriptEntityValueUVE* body = FindEntityInputUVE(context, nodeId, "Body");
    if (body == nullptr || !body->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex, "Physics body node requires a valid Body entity input.");
    }
    if (instruction.nodeTypeId == "physics.apply_force" || instruction.nodeTypeId == "physics.apply_impulse" ||
        instruction.nodeTypeId == "physics.set_velocity") {
        const char* pinName = instruction.nodeTypeId == "physics.apply_force" ? "Force" :
                              instruction.nodeTypeId == "physics.apply_impulse" ? "Impulse" : "Velocity";
        const ScriptVector3ValueUVE* value = FindVector3InputUVE(context, nodeId, pinName);
        const ScriptPhysicsBodyVectorMutationFunctionUVE callback =
            instruction.nodeTypeId == "physics.apply_force" ? bindings->physicsApplyForce :
            instruction.nodeTypeId == "physics.apply_impulse" ? bindings->physicsApplyImpulse : bindings->physicsSetVelocity;
        bool accepted = false;
        if (value == nullptr || !IsFiniteVector3ValueUVE(*value) || callback == nullptr ||
            !callback(bindings->userData, body->entity, *value, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Physics body vector callback rejected its copied input.");
        }
        return setBooleanResult(true);
    }
    if (instruction.nodeTypeId == "physics.get_velocity") {
        if (bindings->physicsGetVelocity == nullptr) {
            return MakeNodeFailureUVE(instructionIndex, "Get Velocity requires a caller-owned binding.");
        }
        ScriptVector3ValueUVE velocity{};
        if (!bindings->physicsGetVelocity(bindings->userData, body->entity, &velocity) ||
            !IsFiniteVector3ValueUVE(velocity) || !SetNodeOutputUVE(context, nodeId, "Velocity", velocity)) {
            return MakeNodeFailureUVE(instructionIndex, "Get Velocity callback returned invalid copied data or output capacity.");
        }
        return {};
    }
    if (instruction.nodeTypeId == "physics.enable_gravity") {
        const bool* enabled = FindBooleanInputUVE(context, nodeId, "Enabled");
        bool accepted = false;
        if (enabled == nullptr || bindings->physicsEnableGravity == nullptr ||
            !bindings->physicsEnableGravity(bindings->userData, body->entity, *enabled, &accepted) || !accepted) {
            return MakeNodeFailureUVE(instructionIndex, "Enable Gravity callback rejected its copied input.");
        }
        return setBooleanResult(true);
    }
    if (instruction.nodeTypeId == "physics.is_colliding") {
        bool colliding = false;
        if (bindings->physicsIsColliding == nullptr ||
            !bindings->physicsIsColliding(bindings->userData, body->entity, &colliding) ||
            !SetNodeOutputUVE(context, nodeId, "Result", colliding)) {
            return MakeNodeFailureUVE(instructionIndex, "Is Colliding callback rejected its copied query or output capacity.");
        }
        return {};
    }
    return MakeNodeFailureUVE(instructionIndex, "Unknown Physics node type.");
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteAudioNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    const bool isSetVolume = instruction.nodeTypeId == "audio.set_volume";
    const bool isSetPitch = instruction.nodeTypeId == "audio.set_pitch";
    const bool isSetPosition = instruction.nodeTypeId == "audio.set_3d_position";
    const bool isPlaySound = instruction.nodeTypeId == "audio.play_sound";
    const bool isStopSound = instruction.nodeTypeId == "audio.stop_sound";
    const bool isPlayingQuery = instruction.nodeTypeId == "audio.is_playing";
    const bool isSetAttenuation = instruction.nodeTypeId == "audio.set_attenuation";
    if (!isSetVolume && !isSetPitch && !isSetPosition && !isPlaySound && !isStopSound && !isPlayingQuery &&
        !isSetAttenuation) {
        return MakeNodeFailureUVE(instructionIndex, "Unknown Audio node type.");
    }
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const ScriptEntityValueUVE* source = FindEntityInputUVE(context, nodeId, "Source");
    if (source == nullptr || !source->IsValidUVE()) {
        return MakeNodeFailureUVE(instructionIndex, "Audio node requires a valid Source entity.");
    }
    bool accepted = false;
    if (isPlayingQuery) {
        const ScriptAudioStateQueryFunctionUVE callback =
            bindings == nullptr ? nullptr : bindings->audioIsPlaying;
        bool isPlaying = false;
        if (callback == nullptr || !callback(bindings->userData, source->entity, &isPlaying) ||
            !SetNodeOutputUVE(context, nodeId, "Result", isPlaying)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Is Playing requires an accepted caller-owned state query callback.");
        }
        return {};
    }
    if (isPlaySound || isStopSound) {
        const ScriptAudioTriggerFunctionUVE callback =
            isPlaySound ? (bindings == nullptr ? nullptr : bindings->audioPlaySound)
                        : (bindings == nullptr ? nullptr : bindings->audioStopSound);
        if (callback == nullptr || !callback(bindings->userData, source->entity, &accepted) || !accepted ||
            !SetNodeOutputUVE(context, nodeId, "Result", true)) {
            return MakeNodeFailureUVE(
                instructionIndex, isPlaySound
                    ? "Play Sound requires an accepted caller-owned trigger callback."
                    : "Stop Sound requires an accepted caller-owned trigger callback.");
        }
        return {};
    }
    if (isSetAttenuation) {
        const ScriptAudioAttenuationControlFunctionUVE callback =
            bindings == nullptr ? nullptr : bindings->audioSetAttenuation;
        const float* minDistance = FindNumberInputUVE(context, nodeId, "Min Distance");
        const float* maxDistance = FindNumberInputUVE(context, nodeId, "Max Distance");
        const float* model = FindNumberInputUVE(context, nodeId, "Model");
        const bool validModel = model != nullptr && (*model == 0.0F || *model == 1.0F);
        if (callback == nullptr || minDistance == nullptr || maxDistance == nullptr || model == nullptr ||
            !std::isfinite(*minDistance) || !std::isfinite(*maxDistance) || !std::isfinite(*model) ||
            *minDistance <= 0.0F || *maxDistance <= *minDistance || !validModel ||
            !callback(bindings->userData, source->entity, *minDistance, *maxDistance, *model, &accepted) || !accepted ||
            !SetNodeOutputUVE(context, nodeId, "Result", true)) {
            return MakeNodeFailureUVE(instructionIndex,
                                      "Set Attenuation requires finite ordered distances, a supported model, and an accepted caller-owned callback.");
        }
        return {};
    }
    if (isSetPosition) {
        const ScriptAudioPositionControlFunctionUVE callback =
            bindings == nullptr ? nullptr : bindings->audioSet3dPosition;
        const ScriptVector3ValueUVE* position = FindVector3InputUVE(context, nodeId, "Position");
        if (callback == nullptr || position == nullptr || !std::isfinite(position->value.x) ||
            !std::isfinite(position->value.y) || !std::isfinite(position->value.z) ||
            !callback(bindings->userData, source->entity, *position, &accepted) || !accepted ||
            !SetNodeOutputUVE(context, nodeId, "Result", true)) {
            return MakeNodeFailureUVE(
                instructionIndex, "Set 3D Position requires a finite Position and accepted callback output.");
        }
        return {};
    }
    const ScriptAudioScalarControlFunctionUVE callback =
        isSetVolume ? (bindings == nullptr ? nullptr : bindings->audioSetVolume)
                    : (bindings == nullptr ? nullptr : bindings->audioSetPitch);
    const char* valuePin = isSetVolume ? "Volume" : "Pitch";
    const float* value = FindNumberInputUVE(context, nodeId, valuePin);
    const bool validValue = value != nullptr && std::isfinite(*value) &&
        (isSetVolume ? *value >= 0.0F && *value <= 1.0F : *value > 0.0F);
    if (callback == nullptr || !validValue) {
        return MakeNodeFailureUVE(
            instructionIndex, isSetVolume
                ? "Set Volume requires a finite Volume in [0, 1] and accepted callback output."
                : "Set Pitch requires a finite positive Pitch and accepted callback output.");
    }
    if (!callback(bindings->userData, source->entity, *value, &accepted) || !accepted ||
        !SetNodeOutputUVE(context, nodeId, "Result", true)) {
        return MakeNodeFailureUVE(
            instructionIndex, isSetVolume
                ? "Set Volume callback rejected its copied input or Boolean output capacity."
                : "Set Pitch callback rejected its copied input or Boolean output capacity.");
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
    const bool isDebugPrint = instruction.nodeTypeId == "debug.print";
    if (value == nullptr || !std::isfinite(*value)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  isDebugPrint ? "debug.print requires a finite Number Value input."
                                               : "engine.log requires a finite Number Value input.");
    }
    if (bindings == nullptr || bindings->log == nullptr) {
        return MakeNodeFailureUVE(instructionIndex,
                                  isDebugPrint ? "debug.print requires a caller-owned log binding."
                                               : "engine.log requires a caller-owned engine call binding.");
    }
    if (!bindings->log(bindings->userData, *value)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  isDebugPrint ? "debug.print callback rejected the copied Number value."
                                               : "engine.log callback rejected the copied Number value.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteDebugWarningNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    const float* value = FindNumberInputUVE(context, instruction.sourceNodeId, "Value");
    bool accepted = false;
    if (value == nullptr || !std::isfinite(*value)) {
        return MakeNodeFailureUVE(instructionIndex, "debug.warning requires a finite Number Value input.");
    }
    if (bindings == nullptr || bindings->debugWarning == nullptr ||
        !bindings->debugWarning(bindings->userData, *value, &accepted) || !accepted ||
        !SetNodeOutputUVE(context, instruction.sourceNodeId, "Result", true)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "debug.warning requires an accepted caller-owned warning callback and Result output.");
    }
    return {};
}

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteDebugErrorNodeUVE(
    const ScriptIrInstructionUVE& instruction, const std::size_t instructionIndex,
    ScriptVmExecutionContextUVE& context, const ScriptEngineCallBindingsUVE* bindings) {
    const float* value = FindNumberInputUVE(context, instruction.sourceNodeId, "Value");
    bool accepted = false;
    if (value == nullptr || !std::isfinite(*value)) {
        return MakeNodeFailureUVE(instructionIndex, "debug.error requires a finite Number Value input.");
    }
    if (bindings == nullptr || bindings->debugError == nullptr ||
        !bindings->debugError(bindings->userData, *value, &accepted) || !accepted ||
        !SetNodeOutputUVE(context, instruction.sourceNodeId, "Result", true)) {
        return MakeNodeFailureUVE(instructionIndex,
                                  "debug.error requires an accepted caller-owned error callback and Result output.");
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
    if (instruction.nodeTypeId.ends_with("_vector3")) {
        return FindVector3InputUVE(context, instruction.sourceNodeId, "Value") != nullptr;
    }
    const ScriptVmValueBindingUVE* binding = FindBindingUVE(context.inputs, instruction.sourceNodeId, "Value");
    if (binding == nullptr) {
        return false;
    }
    if (instruction.nodeTypeId.ends_with("_array")) {
        return std::holds_alternative<ScriptArrayValueUVE>(binding->value);
    }
    if (instruction.nodeTypeId.ends_with("_map")) {
        return std::holds_alternative<ScriptMapValueUVE>(binding->value);
    }
    if (instruction.nodeTypeId.ends_with("_set")) {
        return std::holds_alternative<ScriptSetValueUVE>(binding->value);
    }
    if (instruction.nodeTypeId.ends_with("_struct")) {
        return std::holds_alternative<ScriptStructValueUVE>(binding->value);
    }
    return false;
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

[[nodiscard]] bool HasRequiredEntityNodeInputsUVE(
    const ScriptIrInstructionUVE& instruction, const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId.rfind("entity.", 0U) != 0U) {
        return true;
    }
    if (instruction.nodeTypeId == "entity.spawn") {
        return true;
    }
    if (instruction.nodeTypeId == "entity.destroy") {
        return FindEntityInputUVE(context, instruction.sourceNodeId, "Entity") != nullptr;
    }
    if (instruction.nodeTypeId == "entity.find") {
        return FindComponentInputUVE(context, instruction.sourceNodeId, "Component") != nullptr;
    }
    if (instruction.nodeTypeId == "entity.get_entity") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Handle") != nullptr;
    }
    if (instruction.nodeTypeId == "entity.add_component" || instruction.nodeTypeId == "entity.remove_component") {
        return FindEntityInputUVE(context, instruction.sourceNodeId, "Entity") != nullptr &&
               FindComponentInputUVE(context, instruction.sourceNodeId, "Component") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredInputNodeInputsUVE(
    const ScriptIrInstructionUVE& instruction, const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId == "input.mouse_position") {
        return true;
    }
    if (instruction.nodeTypeId == "input.key_pressed" || instruction.nodeTypeId == "input.key_released" ||
        instruction.nodeTypeId == "input.key_down") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Key") != nullptr;
    }
    if (instruction.nodeTypeId == "input.mouse_button") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Button") != nullptr;
    }
    if (instruction.nodeTypeId == "input.gamepad_button") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Gamepad") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "Button") != nullptr;
    }
    if (instruction.nodeTypeId == "input.get_axis") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Gamepad") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "Axis") != nullptr;
    }
    if (instruction.nodeTypeId == "input.get_action") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Action") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredCameraNodeInputsUVE(
    const ScriptIrInstructionUVE& instruction, const ScriptVmExecutionContextUVE& context) {
    if (instruction.nodeTypeId == "camera.get_camera") {
        return true;
    }
    if (instruction.nodeTypeId.rfind("camera.", 0U) != 0U) {
        return false;
    }
    if (FindEntityInputUVE(context, instruction.sourceNodeId, "Camera") == nullptr) {
        return false;
    }
    if (instruction.nodeTypeId == "camera.set_position") {
        return FindVector3InputUVE(context, instruction.sourceNodeId, "Position") != nullptr;
    }
    if (instruction.nodeTypeId == "camera.set_rotation") {
        return FindRotationInputUVE(context, instruction.sourceNodeId, "Rotation") != nullptr;
    }
    if (instruction.nodeTypeId == "camera.look_at") {
        return FindVector3InputUVE(context, instruction.sourceNodeId, "Target") != nullptr;
    }
    if (instruction.nodeTypeId == "camera.set_fov") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "FOV") != nullptr;
    }
    if (instruction.nodeTypeId == "camera.shake") {
        return FindNumberInputUVE(context, instruction.sourceNodeId, "Amplitude") != nullptr &&
               FindNumberInputUVE(context, instruction.sourceNodeId, "Duration") != nullptr;
    }
    if (instruction.nodeTypeId == "camera.set_active") {
        return FindBooleanInputUVE(context, instruction.sourceNodeId, "Active") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredPhysicsNodeInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                       const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    const std::string& type = instruction.nodeTypeId;
    if (type == "physics.raycast") {
        return FindVector3InputUVE(context, nodeId, "Origin") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Direction") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Layer Mask") != nullptr;
    }
    if (type == "physics.sphere_cast") {
        return FindVector3InputUVE(context, nodeId, "Origin") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Direction") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Radius") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Layer Mask") != nullptr;
    }
    if (type == "physics.box_cast") {
        return FindVector3InputUVE(context, nodeId, "Origin") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Half Extents") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Direction") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Layer Mask") != nullptr;
    }
    if (type == "physics.capsule_cast") {
        return FindVector3InputUVE(context, nodeId, "Origin") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Direction") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Radius") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Half Height") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Layer Mask") != nullptr;
    }
    if (type == "physics.overlap") {
        return FindVector3InputUVE(context, nodeId, "Origin") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Half Extents") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Layer Mask") != nullptr;
    }
    if (type == "physics.apply_force") return FindEntityInputUVE(context, nodeId, "Body") != nullptr &&
                                               FindVector3InputUVE(context, nodeId, "Force") != nullptr;
    if (type == "physics.apply_impulse") return FindEntityInputUVE(context, nodeId, "Body") != nullptr &&
                                                 FindVector3InputUVE(context, nodeId, "Impulse") != nullptr;
    if (type == "physics.set_velocity") return FindEntityInputUVE(context, nodeId, "Body") != nullptr &&
                                                 FindVector3InputUVE(context, nodeId, "Velocity") != nullptr;
    if (type == "physics.get_velocity" || type == "physics.is_colliding") {
        return FindEntityInputUVE(context, nodeId, "Body") != nullptr;
    }
    if (type == "physics.enable_gravity") {
        return FindEntityInputUVE(context, nodeId, "Body") != nullptr &&
               FindBooleanInputUVE(context, nodeId, "Enabled") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredAudioNodeInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                       const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (FindEntityInputUVE(context, nodeId, "Source") == nullptr) {
        return false;
    }
    if (instruction.nodeTypeId == "audio.play_sound" || instruction.nodeTypeId == "audio.stop_sound" ||
        instruction.nodeTypeId == "audio.is_playing") {
        return true;
    }
    if (instruction.nodeTypeId == "audio.set_3d_position") {
        return FindVector3InputUVE(context, nodeId, "Position") != nullptr;
    }
    if (instruction.nodeTypeId == "audio.set_volume") {
        return FindNumberInputUVE(context, nodeId, "Volume") != nullptr;
    }
    if (instruction.nodeTypeId == "audio.set_pitch") {
        return FindNumberInputUVE(context, nodeId, "Pitch") != nullptr;
    }
    if (instruction.nodeTypeId == "audio.set_attenuation") {
        return FindNumberInputUVE(context, nodeId, "Min Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Max Distance") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Model") != nullptr;
    }
    return false;
}

[[nodiscard]] bool HasRequiredAnimationNodeInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                          const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (FindEntityInputUVE(context, nodeId, "Actor") == nullptr) {
        return false;
    }
    const std::string& type = instruction.nodeTypeId;
    if (type == "animation.play") {
        return FindNumberInputUVE(context, nodeId, "Clip") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Blend Duration") != nullptr;
    }
    if (type == "animation.stop" || type == "animation.pause" || type == "animation.is_playing") {
        return FindNumberInputUVE(context, nodeId, "Clip") != nullptr;
    }
    if (type == "animation.blend") {
        return FindNumberInputUVE(context, nodeId, "Clip A") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Clip B") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Weight") != nullptr;
    }
    if (type == "animation.blend_space") {
        return FindNumberInputUVE(context, nodeId, "Blend Space") != nullptr &&
               FindNumberInputUVE(context, nodeId, "X") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Y") != nullptr;
    }
    if (type == "animation.set_speed") return FindNumberInputUVE(context, nodeId, "Speed") != nullptr;
    if (type == "animation.set_weight") return FindNumberInputUVE(context, nodeId, "Weight") != nullptr;
    if (type == "animation.montage") {
        return FindNumberInputUVE(context, nodeId, "Montage") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Weight") != nullptr;
    }
    return true;
}

[[nodiscard]] bool HasRequiredMotionQueryNodeInputsUVE(const ScriptIrInstructionUVE& instruction,
                                                         const ScriptVmExecutionContextUVE& context) {
    const std::uint32_t nodeId = instruction.sourceNodeId;
    if (FindEntityInputUVE(context, nodeId, "Actor") == nullptr) {
        return false;
    }
    const std::string& type = instruction.nodeTypeId;
    if (type == "motion.query.build") {
        return FindVector3InputUVE(context, nodeId, "Velocity") != nullptr &&
               FindVector3InputUVE(context, nodeId, "Facing") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Delta") != nullptr;
    }
    if (type == "motion.query.search") return FindNumberInputUVE(context, nodeId, "Max Results") != nullptr;
    if (type == "motion.query.set_trajectory") {
        return FindVector3InputUVE(context, nodeId, "Sample") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Offset") != nullptr;
    }
    if (type == "motion.query.set_pose") return FindTransformInputUVE(context, nodeId, "Pose") != nullptr;
    if (type == "motion.query.set_velocity") return FindVector3InputUVE(context, nodeId, "Velocity") != nullptr;
    if (type == "motion.query.set_facing") return FindVector3InputUVE(context, nodeId, "Facing") != nullptr;
    if (type == "motion.query.set_yaw") return FindNumberInputUVE(context, nodeId, "Yaw") != nullptr;
    if (type == "motion.query.transition") {
        return FindNumberInputUVE(context, nodeId, "Target") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Duration") != nullptr;
    }
    if (type == "motion.query.motion_warp") {
        return FindVector3InputUVE(context, nodeId, "Target") != nullptr &&
               FindNumberInputUVE(context, nodeId, "Weight") != nullptr;
    }
    return true;
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
        const bool isEntityNode = instruction.nodeTypeId.rfind("entity.", 0U) == 0U;
        const bool isInputNode = instruction.nodeTypeId.rfind("input.", 0U) == 0U;
        const bool isCameraNode = instruction.nodeTypeId.rfind("camera.", 0U) == 0U;
        const bool isAnimationNode = instruction.nodeTypeId.rfind("animation.", 0U) == 0U;
        const bool isMotionQueryNode = instruction.nodeTypeId.rfind("motion.query.", 0U) == 0U;
        const bool isPhysicsNode = instruction.nodeTypeId.rfind("physics.", 0U) == 0U;
        const bool isAudioNode = instruction.nodeTypeId.rfind("audio.", 0U) == 0U;
        const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
        const bool isDebugPrintNode = instruction.nodeTypeId == "debug.print";
        const bool isDebugWarningNode = instruction.nodeTypeId == "debug.warning";
        const bool isDebugErrorNode = instruction.nodeTypeId == "debug.error";
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
            (isEntityNode && !HasRequiredEntityNodeInputsUVE(instruction, context)) ||
            (isInputNode && !HasRequiredInputNodeInputsUVE(instruction, context)) ||
            (isCameraNode && !HasRequiredCameraNodeInputsUVE(instruction, context)) ||
            (isAnimationNode && !HasRequiredAnimationNodeInputsUVE(instruction, context)) ||
            (isMotionQueryNode && !HasRequiredMotionQueryNodeInputsUVE(instruction, context)) ||
            (isPhysicsNode && !HasRequiredPhysicsNodeInputsUVE(instruction, context)) ||
            (isAudioNode && !HasRequiredAudioNodeInputsUVE(instruction, context)) ||
            ((isEngineLogNode || isDebugPrintNode || isDebugWarningNode || isDebugErrorNode) &&
             FindNumberInputUVE(context, instruction.sourceNodeId, "Value") == nullptr)) {
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
        } else if (isEntityNode) {
            nodeResult = ExecuteEntityNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isInputNode) {
            nodeResult = ExecuteInputNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isCameraNode) {
            nodeResult = ExecuteCameraNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isAnimationNode) {
            nodeResult = ExecuteAnimationNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isMotionQueryNode) {
            nodeResult = ExecuteMotionQueryNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isPhysicsNode) {
            nodeResult = ExecutePhysicsNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
        } else if (isAudioNode) {
            nodeResult = ExecuteAudioNodeUVE(instruction, instructionIndex, context, options.engineCallBindings);
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
                (instruction.nodeTypeId == "engine.log" || instruction.nodeTypeId == "debug.print" ||
                 instruction.nodeTypeId == "debug.warning" || instruction.nodeTypeId == "debug.error" ||
                 instruction.nodeTypeId == "engine.get_time")) {
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
                const bool isEntityNode = instruction.nodeTypeId.rfind("entity.", 0U) == 0U;
                const bool isInputNode = instruction.nodeTypeId.rfind("input.", 0U) == 0U;
                const bool isCameraNode = instruction.nodeTypeId.rfind("camera.", 0U) == 0U;
                const bool isAnimationNode = instruction.nodeTypeId.rfind("animation.", 0U) == 0U;
                const bool isMotionQueryNode = instruction.nodeTypeId.rfind("motion.query.", 0U) == 0U;
                const bool isPhysicsNode = instruction.nodeTypeId.rfind("physics.", 0U) == 0U;
                const bool isAudioNode = instruction.nodeTypeId.rfind("audio.", 0U) == 0U;
                const bool isEngineLogNode = instruction.nodeTypeId == "engine.log";
                const bool isDebugPrintNode = instruction.nodeTypeId == "debug.print";
                const bool isDebugWarningNode = instruction.nodeTypeId == "debug.warning";
                const bool isDebugErrorNode = instruction.nodeTypeId == "debug.error";
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
                    (isEntityNode && !HasRequiredEntityNodeInputsUVE(instruction, *context)) ||
                    (isInputNode && !HasRequiredInputNodeInputsUVE(instruction, *context)) ||
                    (isCameraNode && !HasRequiredCameraNodeInputsUVE(instruction, *context)) ||
                    (isAnimationNode && !HasRequiredAnimationNodeInputsUVE(instruction, *context)) ||
                    (isMotionQueryNode && !HasRequiredMotionQueryNodeInputsUVE(instruction, *context)) ||
                    (isPhysicsNode && !HasRequiredPhysicsNodeInputsUVE(instruction, *context)) ||
                    (isAudioNode && !HasRequiredAudioNodeInputsUVE(instruction, *context)) ||
                    ((isEngineLogNode || isDebugPrintNode || isDebugWarningNode || isDebugErrorNode) &&
                     FindNumberInputUVE(*context, instruction.sourceNodeId, "Value") == nullptr)) {
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
                } else if (isEntityNode) {
                    nodeResult = ExecuteEntityNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isInputNode) {
                    nodeResult = ExecuteInputNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isCameraNode) {
                    nodeResult = ExecuteCameraNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isAnimationNode) {
                    nodeResult = ExecuteAnimationNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isMotionQueryNode) {
                    nodeResult = ExecuteMotionQueryNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isPhysicsNode) {
                    nodeResult = ExecutePhysicsNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isAudioNode) {
                    nodeResult = ExecuteAudioNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isEngineLogNode || isDebugPrintNode) {
                    nodeResult = ExecuteEngineLogNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isDebugWarningNode) {
                    nodeResult = ExecuteDebugWarningNodeUVE(instruction, index, *context, options.engineCallBindings);
                } else if (isDebugErrorNode) {
                    nodeResult = ExecuteDebugErrorNodeUVE(instruction, index, *context, options.engineCallBindings);
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
    if (nodeId == 0U || !IsValidScriptVmIdentifierUVE(pinName)) {
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
    if (nodeId == 0U || !IsValidScriptVmIdentifierUVE(pinName)) {
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
    if (entity == Scene::kInvalidEntityUVE || !IsValidScriptVmIdentifierUVE(componentTypeId)) {
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
