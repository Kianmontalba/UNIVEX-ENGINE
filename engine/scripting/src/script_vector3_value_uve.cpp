// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vector3_value_uve.h"

#include <cmath>
#include <limits>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsFiniteUVE(const float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool IsFiniteUVE(const Math::Vector3UVE& value) noexcept {
    return IsFiniteUVE(value.x) && IsFiniteUVE(value.y) && IsFiniteUVE(value.z);
}

[[nodiscard]] ScriptVector3ValueResultUVE MakeValueResultUVE(
    const ScriptVector3EvaluationCodeUVE code, const Math::Vector3UVE value = {}) noexcept {
    return ScriptVector3ValueResultUVE{code, ScriptVector3ValueUVE{value}};
}

[[nodiscard]] ScriptVector3NumberResultUVE MakeNumberResultUVE(
    const ScriptVector3EvaluationCodeUVE code, const float value = 0.0F) noexcept {
    return ScriptVector3NumberResultUVE{code, value};
}

[[nodiscard]] bool IsFiniteInputUVE(const ScriptVector3ValueUVE& value) noexcept {
    return IsFiniteUVE(value.value);
}

} // namespace

ScriptVector3ValueResultUVE EvaluateScriptVector3MakeUVE(const float x, const float y, const float z) noexcept {
    const Math::Vector3UVE value{x, y, z};
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3AddUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector3UVE value = lhs.value + rhs.value;
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3SubtractUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector3UVE value = lhs.value - rhs.value;
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3MultiplyUVE(
    const ScriptVector3ValueUVE& vector, const float scalar) noexcept {
    if (!IsFiniteInputUVE(vector) || !IsFiniteUVE(scalar)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector3UVE value = vector.value * scalar;
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3NumberResultUVE EvaluateScriptVector3DotUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const float value = Math::DotUVE(lhs.value, rhs.value);
    return IsFiniteUVE(value)
        ? MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3CrossUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector3UVE value = Math::CrossUVE(lhs.value, rhs.value);
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3NumberResultUVE EvaluateScriptVector3LengthUVE(
    const ScriptVector3ValueUVE& vector) noexcept {
    if (!IsFiniteInputUVE(vector)) {
        return MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const float value = Math::LengthUVE(vector.value);
    return IsFiniteUVE(value)
        ? MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3NormalizeUVE(
    const ScriptVector3ValueUVE& vector) noexcept {
    if (!IsFiniteInputUVE(vector)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    if (Math::LengthSquaredUVE(vector.value) <= std::numeric_limits<float>::epsilon()) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::ZeroLengthNormalize);
    }
    const Math::Vector3UVE value = Math::NormalizeUVE(vector.value);
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3NumberResultUVE EvaluateScriptVector3DistanceUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs)) {
        return MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const float value = std::hypot(lhs.value.x - rhs.value.x,
                                   lhs.value.y - rhs.value.y,
                                   lhs.value.z - rhs.value.z);
    return IsFiniteUVE(value)
        ? MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeNumberResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

ScriptVector3ValueResultUVE EvaluateScriptVector3DirectionUVE(
    const ScriptVector3ValueUVE& from, const ScriptVector3ValueUVE& to) noexcept {
    if (!IsFiniteInputUVE(from) || !IsFiniteInputUVE(to)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    return EvaluateScriptVector3NormalizeUVE(
        ScriptVector3ValueUVE{Math::Vector3UVE{to.value.x - from.value.x,
                                              to.value.y - from.value.y,
                                              to.value.z - from.value.z}});
}

ScriptVector3ValueResultUVE EvaluateScriptVector3LerpUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs, const float alpha) noexcept {
    if (!IsFiniteInputUVE(lhs) || !IsFiniteInputUVE(rhs) || !IsFiniteUVE(alpha)) {
        return MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    }
    const Math::Vector3UVE value{lhs.value.x + ((rhs.value.x - lhs.value.x) * alpha),
                                 lhs.value.y + ((rhs.value.y - lhs.value.y) * alpha),
                                 lhs.value.z + ((rhs.value.z - lhs.value.z) * alpha)};
    return IsFiniteUVE(value)
        ? MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::Applied, value)
        : MakeValueResultUVE(ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

} // namespace UVE::Scripting

