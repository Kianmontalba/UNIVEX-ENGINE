// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vector2_value_uve.h"

#include <cmath>
#include <limits>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsFiniteUVE(const float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool IsFiniteUVE(const Math::Vector2UVE& value) noexcept {
    return IsFiniteUVE(value.x) && IsFiniteUVE(value.y);
}

[[nodiscard]] bool IsFiniteInputUVE(const ScriptVector2ValueUVE& value) noexcept {
    return IsFiniteUVE(value.value);
}

[[nodiscard]] ScriptVector2ValueResultUVE MakeValueResultUVE(
    const ScriptVector2EvaluationCodeUVE code, const Math::Vector2UVE value = {}) noexcept {
    return {code, ScriptVector2ValueUVE{value}};
}

[[nodiscard]] ScriptVector2NumberResultUVE MakeNumberResultUVE(
    const ScriptVector2EvaluationCodeUVE code, const float value = 0.0F) noexcept {
    return {code, value};
}

} // namespace

ScriptVector2ValueResultUVE EvaluateScriptVector2MakeUVE(const float x, const float y) noexcept {
    const Math::Vector2UVE value{x, y};
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector2ValueResultUVE EvaluateScriptVector2AddUVE(
    const ScriptVector2ValueUVE& lhs, const ScriptVector2ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector2UVE value = lhs.value + rhs.value;
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector2ValueResultUVE EvaluateScriptVector2SubtractUVE(
    const ScriptVector2ValueUVE& lhs, const ScriptVector2ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector2UVE value = lhs.value - rhs.value;
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector2ValueResultUVE EvaluateScriptVector2MultiplyUVE(
    const ScriptVector2ValueUVE& vector, const float scalar) noexcept {
    if (!IsFiniteInputUVE(vector) || !IsFiniteUVE(scalar)) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector2UVE value{vector.value.x * scalar, vector.value.y * scalar};
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector2NumberResultUVE EvaluateScriptVector2LengthUVE(
    const ScriptVector2ValueUVE& vector) noexcept {
    if (!IsFiniteInputUVE(vector)) {
        return MakeNumberResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    const float value = std::hypot(vector.value.x, vector.value.y);
    return IsFiniteUVE(value)
        ? MakeNumberResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeNumberResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector2ValueResultUVE EvaluateScriptVector2NormalizeUVE(
    const ScriptVector2ValueUVE& vector) noexcept {
    if (!IsFiniteInputUVE(vector)) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    const float length = std::hypot(vector.value.x, vector.value.y);
    if (!IsFiniteUVE(length)) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
    }
    if (length <= std::numeric_limits<float>::epsilon()) {
        return MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::ZeroLengthNormalize);
    }
    const Math::Vector2UVE value{vector.value.x / length, vector.value.y / length};
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

} // namespace UVE::Scripting
