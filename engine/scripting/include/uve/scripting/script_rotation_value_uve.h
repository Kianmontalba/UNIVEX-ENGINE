// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/scripting/script_vector3_value_uve.h"

#include <cstdint>

namespace UVE::Scripting {

enum class ScriptRotationEvaluationCodeUVE : std::uint8_t {
    Applied = 0,
    NonFiniteInput,
    DegenerateInput,
    InvalidRange,
};

struct ScriptRotationValueUVE final {
    Math::QuaternionUVE value{};

    [[nodiscard]] constexpr bool operator==(const ScriptRotationValueUVE&) const noexcept = default;
};

struct ScriptRotationValueResultUVE final {
    ScriptRotationEvaluationCodeUVE code = ScriptRotationEvaluationCodeUVE::NonFiniteInput;
    ScriptRotationValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptRotationEvaluationCodeUVE::Applied;
    }
};

struct ScriptRotationBreakResultUVE final {
    ScriptRotationEvaluationCodeUVE code = ScriptRotationEvaluationCodeUVE::NonFiniteInput;
    ScriptVector3ValueUVE axis{};
    float radians = 0.0F;

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptRotationEvaluationCodeUVE::Applied;
    }
};

struct ScriptRotationNumberResultUVE final {
    ScriptRotationEvaluationCodeUVE code = ScriptRotationEvaluationCodeUVE::NonFiniteInput;
    float value = 0.0F;

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptRotationEvaluationCodeUVE::Applied;
    }
};

struct ScriptRotationVectorResultUVE final {
    ScriptRotationEvaluationCodeUVE code = ScriptRotationEvaluationCodeUVE::NonFiniteInput;
    ScriptVector3ValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptRotationEvaluationCodeUVE::Applied;
    }
};

[[nodiscard]] ScriptRotationValueResultUVE EvaluateScriptRotationMakeUVE(
    const ScriptVector3ValueUVE& axis, float radians) noexcept;
[[nodiscard]] ScriptRotationBreakResultUVE EvaluateScriptRotationBreakUVE(
    const ScriptRotationValueUVE& rotation) noexcept;
[[nodiscard]] ScriptRotationNumberResultUVE EvaluateScriptRotationDegreesUVE(float radians) noexcept;
[[nodiscard]] ScriptRotationNumberResultUVE EvaluateScriptRotationRadiansUVE(float degrees) noexcept;
[[nodiscard]] ScriptRotationValueResultUVE EvaluateScriptRotationEulerUVE(
    const ScriptVector3ValueUVE& radians) noexcept;
[[nodiscard]] ScriptRotationValueResultUVE EvaluateScriptRotationQuaternionUVE(
    float x, float y, float z, float w) noexcept;
[[nodiscard]] ScriptRotationValueResultUVE EvaluateScriptRotationLookAtUVE(
    const ScriptVector3ValueUVE& direction, const ScriptVector3ValueUVE& up) noexcept;
[[nodiscard]] ScriptRotationValueResultUVE EvaluateScriptRotationSlerpUVE(
    const ScriptRotationValueUVE& lhs, const ScriptRotationValueUVE& rhs, float alpha) noexcept;
[[nodiscard]] ScriptRotationVectorResultUVE EvaluateScriptRotationRotateUVE(
    const ScriptRotationValueUVE& rotation, const ScriptVector3ValueUVE& vector) noexcept;

} // namespace UVE::Scripting
