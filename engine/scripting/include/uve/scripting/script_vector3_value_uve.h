// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/math/vector3_uve.h"

#include <cstdint>

namespace UVE::Scripting {

enum class ScriptVector3EvaluationCodeUVE : std::uint8_t {
    Applied = 0,
    NonFiniteInput,
    ZeroLengthNormalize,
};

struct ScriptVector3ValueUVE final {
    Math::Vector3UVE value{};

    [[nodiscard]] constexpr bool operator==(const ScriptVector3ValueUVE&) const noexcept = default;
};

struct ScriptVector3ValueResultUVE final {
    ScriptVector3EvaluationCodeUVE code = ScriptVector3EvaluationCodeUVE::NonFiniteInput;
    ScriptVector3ValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptVector3EvaluationCodeUVE::Applied;
    }
};

struct ScriptVector3NumberResultUVE final {
    ScriptVector3EvaluationCodeUVE code = ScriptVector3EvaluationCodeUVE::NonFiniteInput;
    float value = 0.0F;

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptVector3EvaluationCodeUVE::Applied;
    }
};

[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3MakeUVE(
    float x, float y, float z) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3AddUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3SubtractUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3MultiplyUVE(
    const ScriptVector3ValueUVE& vector, float scalar) noexcept;
[[nodiscard]] ScriptVector3NumberResultUVE EvaluateScriptVector3DotUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3CrossUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector3NumberResultUVE EvaluateScriptVector3LengthUVE(
    const ScriptVector3ValueUVE& vector) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3NormalizeUVE(
    const ScriptVector3ValueUVE& vector) noexcept;
[[nodiscard]] ScriptVector3NumberResultUVE EvaluateScriptVector3DistanceUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3DirectionUVE(
    const ScriptVector3ValueUVE& from, const ScriptVector3ValueUVE& to) noexcept;
[[nodiscard]] ScriptVector3ValueResultUVE EvaluateScriptVector3LerpUVE(
    const ScriptVector3ValueUVE& lhs, const ScriptVector3ValueUVE& rhs, float alpha) noexcept;

} // namespace UVE::Scripting

