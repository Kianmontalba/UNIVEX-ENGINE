// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_rotation_value_uve.h"
#include "uve/scripting/script_vector3_value_uve.h"

#include <cstdint>

namespace UVE::Scripting {

enum class ScriptTransformEvaluationCodeUVE : std::uint8_t {
    Applied = 0,
    NonFiniteInput,
    DegenerateRotation,
};

struct ScriptTransformValueUVE final {
    ScriptVector3ValueUVE position{};
    ScriptRotationValueUVE rotation{};
    ScriptVector3ValueUVE scale{{1.0F, 1.0F, 1.0F}};

    [[nodiscard]] constexpr bool operator==(const ScriptTransformValueUVE&) const noexcept = default;
};

struct ScriptTransformValueResultUVE final {
    ScriptTransformEvaluationCodeUVE code = ScriptTransformEvaluationCodeUVE::NonFiniteInput;
    ScriptTransformValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptTransformEvaluationCodeUVE::Applied;
    }
};

struct ScriptTransformVectorResultUVE final {
    ScriptTransformEvaluationCodeUVE code = ScriptTransformEvaluationCodeUVE::NonFiniteInput;
    ScriptVector3ValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptTransformEvaluationCodeUVE::Applied;
    }
};

struct ScriptTransformRotationResultUVE final {
    ScriptTransformEvaluationCodeUVE code = ScriptTransformEvaluationCodeUVE::NonFiniteInput;
    ScriptRotationValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptTransformEvaluationCodeUVE::Applied;
    }
};

[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformMakeUVE(
    const ScriptVector3ValueUVE& position, const ScriptRotationValueUVE& rotation,
    const ScriptVector3ValueUVE& scale) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformBreakUVE(
    const ScriptTransformValueUVE& transform) noexcept;
[[nodiscard]] ScriptTransformVectorResultUVE EvaluateScriptTransformGetPositionUVE(
    const ScriptTransformValueUVE& transform) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformSetPositionUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& position) noexcept;
[[nodiscard]] ScriptTransformRotationResultUVE EvaluateScriptTransformGetRotationUVE(
    const ScriptTransformValueUVE& transform) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformSetRotationUVE(
    const ScriptTransformValueUVE& transform, const ScriptRotationValueUVE& rotation) noexcept;
[[nodiscard]] ScriptTransformVectorResultUVE EvaluateScriptTransformGetScaleUVE(
    const ScriptTransformValueUVE& transform) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformSetScaleUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& scale) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformTranslateUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& translation) noexcept;
[[nodiscard]] ScriptTransformValueResultUVE EvaluateScriptTransformRotateUVE(
    const ScriptTransformValueUVE& transform, const ScriptRotationValueUVE& rotation) noexcept;
[[nodiscard]] ScriptTransformVectorResultUVE EvaluateScriptTransformPointUVE(
    const ScriptTransformValueUVE& transform, const ScriptVector3ValueUVE& point) noexcept;

} // namespace UVE::Scripting
