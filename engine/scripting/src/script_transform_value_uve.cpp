// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_transform_value_uve.h"

#include "uve/math/matrix4x4_uve.h"

#include <cmath>

namespace UVE::Scripting {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const ScriptVector3ValueUVE& vector) noexcept {
    return std::isfinite(vector.value.x) && std::isfinite(vector.value.y) && std::isfinite(vector.value.z);
}

[[nodiscard]] bool IsFiniteTransformUVE(const ScriptTransformValueUVE& transform) noexcept {
    return IsFiniteVectorUVE(transform.position) && IsFiniteVectorUVE(transform.scale) &&
           Math::IsFiniteUVE(transform.rotation.value);
}

[[nodiscard]] ScriptTransformEvaluationCodeUVE NormalizeTransformUVE(
    const ScriptTransformValueUVE& input, ScriptTransformValueUVE& output) noexcept {
    if (!IsFiniteTransformUVE(input)) {
        return ScriptTransformEvaluationCodeUVE::NonFiniteInput;
    }
    Math::QuaternionUVE normalizedRotation{};
    if (!Math::TryNormalizeUVE(input.rotation.value, normalizedRotation)) {
        return ScriptTransformEvaluationCodeUVE::DegenerateRotation;
    }
    output = input;
    output.rotation.value = normalizedRotation;
    return ScriptTransformEvaluationCodeUVE::Applied;
}

[[nodiscard]] ScriptTransformValueResultUVE MakeValueResultUVE(
    const ScriptTransformEvaluationCodeUVE code, const ScriptTransformValueUVE& value = {}) noexcept {
    return {code, value};
}

[[nodiscard]] ScriptTransformVectorResultUVE MakeVectorResultUVE(
    const ScriptTransformEvaluationCodeUVE code, const ScriptVector3ValueUVE& value = {}) noexcept {
    return {code, value};
}

[[nodiscard]] ScriptTransformRotationResultUVE MakeRotationResultUVE(
    const ScriptTransformEvaluationCodeUVE code, const ScriptRotationValueUVE& value = {}) noexcept {
    return {code, value};
}

} // namespace

ScriptTransformValueResultUVE EvaluateScriptTransformMakeUVE(
    const ScriptVector3ValueUVE& position, const ScriptRotationValueUVE& rotation,
    const ScriptVector3ValueUVE& scale) noexcept {
    ScriptTransformValueUVE candidate{position, rotation, scale};
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(candidate, normalized);
    return MakeValueResultUVE(code, normalized);
}

ScriptTransformValueResultUVE EvaluateScriptTransformBreakUVE(
    const ScriptTransformValueUVE& transform) noexcept {
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    return MakeValueResultUVE(code, normalized);
}

ScriptTransformVectorResultUVE EvaluateScriptTransformGetPositionUVE(
    const ScriptTransformValueUVE& transform) noexcept {
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    return MakeVectorResultUVE(code, normalized.position);
}

ScriptTransformValueResultUVE EvaluateScriptTransformSetPositionUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& position) noexcept {
    if (!IsFiniteVectorUVE(position)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeValueResultUVE(code);
    }
    normalized.position = position;
    return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::Applied, normalized);
}

ScriptTransformRotationResultUVE EvaluateScriptTransformGetRotationUVE(
    const ScriptTransformValueUVE& transform) noexcept {
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    return MakeRotationResultUVE(code, normalized.rotation);
}

ScriptTransformValueResultUVE EvaluateScriptTransformSetRotationUVE(
    const ScriptTransformValueUVE& transform, const ScriptRotationValueUVE& rotation) noexcept {
    if (!Math::IsFiniteUVE(rotation.value)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    Math::QuaternionUVE normalizedRotation{};
    if (!Math::TryNormalizeUVE(rotation.value, normalizedRotation)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::DegenerateRotation);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeValueResultUVE(code);
    }
    normalized.rotation.value = normalizedRotation;
    return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::Applied, normalized);
}

ScriptTransformVectorResultUVE EvaluateScriptTransformGetScaleUVE(
    const ScriptTransformValueUVE& transform) noexcept {
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    return MakeVectorResultUVE(code, normalized.scale);
}

ScriptTransformValueResultUVE EvaluateScriptTransformSetScaleUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& scale) noexcept {
    if (!IsFiniteVectorUVE(scale)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeValueResultUVE(code);
    }
    normalized.scale = scale;
    return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::Applied, normalized);
}

ScriptTransformValueResultUVE EvaluateScriptTransformTranslateUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& translation) noexcept {
    if (!IsFiniteVectorUVE(translation)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeValueResultUVE(code);
    }
    normalized.position.value = {
        normalized.position.value.x + translation.value.x,
        normalized.position.value.y + translation.value.y,
        normalized.position.value.z + translation.value.z,
    };
    if (!IsFiniteVectorUVE(normalized.position)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::Applied, normalized);
}

ScriptTransformValueResultUVE EvaluateScriptTransformRotateUVE(
    const ScriptTransformValueUVE& transform, const ScriptRotationValueUVE& rotation) noexcept {
    if (!Math::IsFiniteUVE(rotation.value)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    Math::QuaternionUVE normalizedDelta{};
    if (!Math::TryNormalizeUVE(rotation.value, normalizedDelta)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::DegenerateRotation);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeValueResultUVE(code);
    }
    const Math::QuaternionUVE composed = Math::MultiplyUVE(normalized.rotation.value, normalizedDelta);
    if (!Math::TryNormalizeUVE(composed, normalized.rotation.value)) {
        return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::DegenerateRotation);
    }
    return MakeValueResultUVE(ScriptTransformEvaluationCodeUVE::Applied, normalized);
}

ScriptTransformVectorResultUVE EvaluateScriptTransformPointUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& point) noexcept {
    if (!IsFiniteVectorUVE(point)) {
        return MakeVectorResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    ScriptTransformValueUVE normalized{};
    const ScriptTransformEvaluationCodeUVE code = NormalizeTransformUVE(transform, normalized);
    if (code != ScriptTransformEvaluationCodeUVE::Applied) {
        return MakeVectorResultUVE(code);
    }
    const Math::Matrix4x4UVE matrix = Math::Matrix4x4UVE::ComposeTrsUVE(
        normalized.position.value, normalized.rotation.value, normalized.scale.value);
    const Math::Vector3UVE transformed = Math::TransformPointUVE(matrix, point.value);
    if (!std::isfinite(transformed.x) || !std::isfinite(transformed.y) || !std::isfinite(transformed.z)) {
        return MakeVectorResultUVE(ScriptTransformEvaluationCodeUVE::NonFiniteInput);
    }
    return MakeVectorResultUVE(ScriptTransformEvaluationCodeUVE::Applied, ScriptVector3ValueUVE{transformed});
}

} // namespace UVE::Scripting
