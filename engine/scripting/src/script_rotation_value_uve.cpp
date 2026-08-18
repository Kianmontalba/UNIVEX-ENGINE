// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_rotation_value_uve.h"

#include <cmath>
#include <numbers>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const ScriptVector3ValueUVE& value) noexcept {
    return std::isfinite(value.value.x) && std::isfinite(value.value.y) && std::isfinite(value.value.z);
}

[[nodiscard]] ScriptRotationValueResultUVE MakeRotationResultUVE(
    const Math::QuaternionUVE& value) noexcept {
    Math::QuaternionUVE normalized{};
    if (!Math::TryNormalizeUVE(value, normalized)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    return {ScriptRotationEvaluationCodeUVE::Applied, {normalized}};
}

} // namespace

ScriptRotationValueResultUVE EvaluateScriptRotationMakeUVE(
    const ScriptVector3ValueUVE& axis, const float radians) noexcept {
    if (!IsFiniteVectorUVE(axis) || !std::isfinite(radians)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    Math::QuaternionUVE value{};
    if (!Math::TryMakeAxisAngleUVE(axis.value, radians, value)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    return MakeRotationResultUVE(value);
}

ScriptRotationBreakResultUVE EvaluateScriptRotationBreakUVE(
    const ScriptRotationValueUVE& rotation) noexcept {
    Math::Vector3UVE axis{};
    float radians = 0.0F;
    if (!Math::IsFiniteUVE(rotation.value)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}, 0.0F};
    }
    if (!Math::TryToAxisAngleUVE(rotation.value, axis, radians)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}, 0.0F};
    }
    return {ScriptRotationEvaluationCodeUVE::Applied, {axis}, radians};
}

ScriptRotationNumberResultUVE EvaluateScriptRotationDegreesUVE(const float radians) noexcept {
    if (!std::isfinite(radians)) return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, 0.0F};
    const float value = radians * (180.0F / std::numbers::pi_v<float>);
    return std::isfinite(value) ? ScriptRotationNumberResultUVE{ScriptRotationEvaluationCodeUVE::Applied, value}
                                : ScriptRotationNumberResultUVE{ScriptRotationEvaluationCodeUVE::NonFiniteInput, 0.0F};
}

ScriptRotationNumberResultUVE EvaluateScriptRotationRadiansUVE(const float degrees) noexcept {
    if (!std::isfinite(degrees)) return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, 0.0F};
    const float value = degrees * (std::numbers::pi_v<float> / 180.0F);
    return std::isfinite(value) ? ScriptRotationNumberResultUVE{ScriptRotationEvaluationCodeUVE::Applied, value}
                                : ScriptRotationNumberResultUVE{ScriptRotationEvaluationCodeUVE::NonFiniteInput, 0.0F};
}

ScriptRotationValueResultUVE EvaluateScriptRotationEulerUVE(
    const ScriptVector3ValueUVE& radians) noexcept {
    if (!IsFiniteVectorUVE(radians)) return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    Math::QuaternionUVE value{};
    if (!Math::TryMakeEulerUVE(radians.value, value)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    return MakeRotationResultUVE(value);
}

ScriptRotationValueResultUVE EvaluateScriptRotationQuaternionUVE(
    const float x, const float y, const float z, const float w) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    return MakeRotationResultUVE(Math::QuaternionUVE{x, y, z, w});
}

ScriptRotationValueResultUVE EvaluateScriptRotationLookAtUVE(
    const ScriptVector3ValueUVE& direction, const ScriptVector3ValueUVE& up) noexcept {
    if (!IsFiniteVectorUVE(direction) || !IsFiniteVectorUVE(up)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    Math::QuaternionUVE value{};
    if (!Math::TryMakeLookAtUVE(direction.value, up.value, value)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    return MakeRotationResultUVE(value);
}

ScriptRotationValueResultUVE EvaluateScriptRotationSlerpUVE(
    const ScriptRotationValueUVE& lhs, const ScriptRotationValueUVE& rhs, const float alpha) noexcept {
    if (!Math::IsFiniteUVE(lhs.value) || !Math::IsFiniteUVE(rhs.value) || !std::isfinite(alpha)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    Math::QuaternionUVE value{};
    if (!Math::TrySlerpUVE(lhs.value, rhs.value, alpha, value)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    return MakeRotationResultUVE(value);
}

ScriptRotationVectorResultUVE EvaluateScriptRotationRotateUVE(
    const ScriptRotationValueUVE& rotation, const ScriptVector3ValueUVE& vector) noexcept {
    if (!Math::IsFiniteUVE(rotation.value) || !IsFiniteVectorUVE(vector)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    Math::QuaternionUVE normalized{};
    if (!Math::TryNormalizeUVE(rotation.value, normalized)) {
        return {ScriptRotationEvaluationCodeUVE::DegenerateInput, {}};
    }
    const Math::Vector3UVE rotated = Math::RotateVectorUVE(normalized, vector.value);
    if (!std::isfinite(rotated.x) || !std::isfinite(rotated.y) || !std::isfinite(rotated.z)) {
        return {ScriptRotationEvaluationCodeUVE::NonFiniteInput, {}};
    }
    return {ScriptRotationEvaluationCodeUVE::Applied, {rotated}};
}

} // namespace UVE::Scripting
