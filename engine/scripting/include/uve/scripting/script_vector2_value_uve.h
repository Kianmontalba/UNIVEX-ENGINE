// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/math/vector2_uve.h"

#include <cstdint>

namespace UVE::Scripting {

enum class ScriptVector2EvaluationCodeUVE : std::uint8_t {
    Applied = 0,
    NonFiniteInput,
    ZeroLengthNormalize,
};

struct ScriptVector2ValueUVE final {
    Math::Vector2UVE value{};

    [[nodiscard]] constexpr bool operator==(const ScriptVector2ValueUVE&) const noexcept = default;
};

struct ScriptVector2ValueResultUVE final {
    ScriptVector2EvaluationCodeUVE code = ScriptVector2EvaluationCodeUVE::NonFiniteInput;
    ScriptVector2ValueUVE value{};

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptVector2EvaluationCodeUVE::Applied;
    }
};

struct ScriptVector2NumberResultUVE final {
    ScriptVector2EvaluationCodeUVE code = ScriptVector2EvaluationCodeUVE::NonFiniteInput;
    float value = 0.0F;

    [[nodiscard]] constexpr bool IsAppliedUVE() const noexcept {
        return code == ScriptVector2EvaluationCodeUVE::Applied;
    }
};

[[nodiscard]] ScriptVector2ValueResultUVE EvaluateScriptVector2MakeUVE(float x, float y) noexcept;
[[nodiscard]] ScriptVector2ValueResultUVE EvaluateScriptVector2AddUVE(
    const ScriptVector2ValueUVE& lhs, const ScriptVector2ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector2ValueResultUVE EvaluateScriptVector2SubtractUVE(
    const ScriptVector2ValueUVE& lhs, const ScriptVector2ValueUVE& rhs) noexcept;
[[nodiscard]] ScriptVector2ValueResultUVE EvaluateScriptVector2MultiplyUVE(
    const ScriptVector2ValueUVE& vector, float scalar) noexcept;
[[nodiscard]] ScriptVector2NumberResultUVE EvaluateScriptVector2LengthUVE(
    const ScriptVector2ValueUVE& vector) noexcept;
[[nodiscard]] ScriptVector2ValueResultUVE EvaluateScriptVector2NormalizeUVE(
    const ScriptVector2ValueUVE& vector) noexcept;

} // namespace UVE::Scripting
