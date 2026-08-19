#include "uve/scripting/script_collection_value_uve.h"

#include <cmath>
#include <type_traits>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsElementTypeUVE(const ScriptCollectionElementUVE& value,
                                    const ScriptCollectionElementTypeUVE type) noexcept {
    switch (type) {
    case ScriptCollectionElementTypeUVE::Number:
        return std::holds_alternative<float>(value);
    case ScriptCollectionElementTypeUVE::Boolean:
        return std::holds_alternative<bool>(value);
    case ScriptCollectionElementTypeUVE::Vector2:
        return std::holds_alternative<ScriptVector2ValueUVE>(value);
    case ScriptCollectionElementTypeUVE::Vector3:
        return std::holds_alternative<ScriptVector3ValueUVE>(value);
    }
    return false;
}

[[nodiscard]] bool IsCollectionElementLessUVE(const ScriptCollectionElementUVE& lhs,
                                              const ScriptCollectionElementUVE& rhs) noexcept {
    return std::visit(
        [](const auto& left, const auto& right) noexcept {
            using LeftType = std::decay_t<decltype(left)>;
            using RightType = std::decay_t<decltype(right)>;
            if constexpr (!std::is_same_v<LeftType, RightType>) {
                return false;
            } else if constexpr (std::is_same_v<LeftType, float> ||
                                 std::is_same_v<LeftType, bool>) {
                return left < right;
            } else if constexpr (std::is_same_v<LeftType, ScriptVector2ValueUVE>) {
                return left.value.x < right.value.x ||
                       (left.value.x == right.value.x && left.value.y < right.value.y);
            } else {
                return left.value.x < right.value.x ||
                       (left.value.x == right.value.x &&
                        (left.value.y < right.value.y ||
                         (left.value.y == right.value.y && left.value.z < right.value.z)));
            }
        },
        lhs, rhs);
}

[[nodiscard]] bool IsStrictlyOrderedAndUniqueUVE(
    const std::vector<ScriptCollectionElementUVE>& elements) noexcept {
    for (std::size_t index = 1U; index < elements.size(); ++index) {
        if (!IsCollectionElementLessUVE(elements[index - 1U], elements[index])) {
            return false;
        }
    }
    return true;
}

} // namespace

bool IsFiniteScriptCollectionElementUVE(const ScriptCollectionElementUVE& value) noexcept {
    return std::visit(
        [](const auto& typedValue) noexcept {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, float>) {
                return std::isfinite(typedValue);
            } else if constexpr (std::is_same_v<ValueType, bool>) {
                return true;
            } else if constexpr (std::is_same_v<ValueType, ScriptVector2ValueUVE>) {
                return std::isfinite(typedValue.value.x) && std::isfinite(typedValue.value.y);
            } else {
                return std::isfinite(typedValue.value.x) && std::isfinite(typedValue.value.y) &&
                       std::isfinite(typedValue.value.z);
            }
        },
        value);
}

bool IsValidScriptArrayValueUVE(const ScriptArrayValueUVE& value) noexcept {
    if (value.elements.size() > kMaximumScriptCollectionElementsUVE) {
        return false;
    }
    for (const ScriptCollectionElementUVE& element : value.elements) {
        if (!IsElementTypeUVE(element, value.elementType) || !IsFiniteScriptCollectionElementUVE(element)) {
            return false;
        }
    }
    return true;
}

bool IsValidScriptMapValueUVE(const ScriptMapValueUVE& value) noexcept {
    if (value.entries.size() > kMaximumScriptCollectionElementsUVE) {
        return false;
    }
    for (std::size_t index = 0U; index < value.entries.size(); ++index) {
        const ScriptMapEntryUVE& entry = value.entries[index];
        if (!std::isfinite(entry.key) || !IsElementTypeUVE(entry.value, value.valueType) ||
            !IsFiniteScriptCollectionElementUVE(entry.value) ||
            (index > 0U && !(value.entries[index - 1U].key < entry.key))) {
            return false;
        }
    }
    return true;
}

bool IsValidScriptSetValueUVE(const ScriptSetValueUVE& value) noexcept {
    if (value.elements.size() > kMaximumScriptCollectionElementsUVE) {
        return false;
    }
    for (const ScriptCollectionElementUVE& element : value.elements) {
        if (!IsElementTypeUVE(element, value.elementType) || !IsFiniteScriptCollectionElementUVE(element)) {
            return false;
        }
    }
    return IsStrictlyOrderedAndUniqueUVE(value.elements);
}

bool IsValidScriptStructValueUVE(const ScriptStructValueUVE& value) noexcept {
    if (value.fields.size() > kMaximumScriptCollectionElementsUVE) {
        return false;
    }
    for (std::size_t index = 0U; index < value.fields.size(); ++index) {
        const ScriptStructFieldUVE& field = value.fields[index];
        if (field.fieldId == 0U || !IsFiniteScriptCollectionElementUVE(field.value) ||
            (index > 0U && value.fields[index - 1U].fieldId >= field.fieldId)) {
            return false;
        }
    }
    return true;
}

} // namespace UVE::Scripting
