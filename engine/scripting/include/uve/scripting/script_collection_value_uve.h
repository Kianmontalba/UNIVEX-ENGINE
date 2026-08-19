#pragma once

#include "uve/scripting/script_vector2_value_uve.h"
#include "uve/scripting/script_vector3_value_uve.h"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace UVE::Scripting {

enum class ScriptCollectionElementTypeUVE : std::uint8_t {
    Number = 0,
    Boolean,
    Vector2,
    Vector3,
};

using ScriptCollectionElementUVE =
    std::variant<float, bool, ScriptVector2ValueUVE, ScriptVector3ValueUVE>;

struct ScriptArrayValueUVE final {
    ScriptCollectionElementTypeUVE elementType = ScriptCollectionElementTypeUVE::Number;
    std::vector<ScriptCollectionElementUVE> elements;

    [[nodiscard]] bool operator==(const ScriptArrayValueUVE&) const = default;
};

struct ScriptMapEntryUVE final {
    float key = 0.0F;
    ScriptCollectionElementUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptMapEntryUVE&) const = default;
};

struct ScriptMapValueUVE final {
    ScriptCollectionElementTypeUVE valueType = ScriptCollectionElementTypeUVE::Number;
    std::vector<ScriptMapEntryUVE> entries;

    [[nodiscard]] bool operator==(const ScriptMapValueUVE&) const = default;
};

struct ScriptSetValueUVE final {
    ScriptCollectionElementTypeUVE elementType = ScriptCollectionElementTypeUVE::Number;
    std::vector<ScriptCollectionElementUVE> elements;

    [[nodiscard]] bool operator==(const ScriptSetValueUVE&) const = default;
};

struct ScriptStructFieldUVE final {
    std::uint32_t fieldId = 0U;
    ScriptCollectionElementUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptStructFieldUVE&) const = default;
};

struct ScriptStructValueUVE final {
    std::vector<ScriptStructFieldUVE> fields;

    [[nodiscard]] bool operator==(const ScriptStructValueUVE&) const = default;
};

inline constexpr std::size_t kMaximumScriptCollectionElementsUVE = 64U;

[[nodiscard]] bool IsFiniteScriptCollectionElementUVE(
    const ScriptCollectionElementUVE& value) noexcept;
[[nodiscard]] bool IsValidScriptArrayValueUVE(const ScriptArrayValueUVE& value) noexcept;
[[nodiscard]] bool IsValidScriptMapValueUVE(const ScriptMapValueUVE& value) noexcept;
[[nodiscard]] bool IsValidScriptSetValueUVE(const ScriptSetValueUVE& value) noexcept;
[[nodiscard]] bool IsValidScriptStructValueUVE(const ScriptStructValueUVE& value) noexcept;

} // namespace UVE::Scripting
