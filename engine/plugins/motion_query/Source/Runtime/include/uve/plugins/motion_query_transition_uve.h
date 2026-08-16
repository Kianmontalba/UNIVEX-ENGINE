// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace UVE::Plugins {

struct MotionQueryTransitionSettingsUVE final {
    static constexpr float kMaximumCostImprovementUVE = 1.0e6F;
    static constexpr double kMaximumHoldWindowSecondsUVE = 10.0;

    float minimumCostImprovement = 0.05F;
    double minimumHoldSeconds = 0.1;
    double maximumHoldWindowSeconds = 0.5;
};

struct MotionQueryTransitionRequestUVE final {
    bool hasPreviousSelection = false;
    std::size_t previousCandidateIndex = 0U;
    std::size_t currentCandidateIndex = 0U;
    float previousCost = 0.0F;
    float currentCost = 0.0F;
    double elapsedSeconds = 0.0;
};

enum class MotionQueryTransitionCodeUVE : std::uint8_t {
    NoPreviousSelection = 0,
    SameCandidate,
    SwitchedCandidate,
    HeldPreviousCandidate,
    HoldWindowExpired,
    InvalidRequest,
};

struct MotionQueryTransitionResultUVE final {
    MotionQueryTransitionCodeUVE code = MotionQueryTransitionCodeUVE::InvalidRequest;
    float costImprovement = 0.0F;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code != MotionQueryTransitionCodeUVE::InvalidRequest;
    }

    [[nodiscard]] bool ShouldHoldPreviousUVE() const noexcept {
        return code == MotionQueryTransitionCodeUVE::HeldPreviousCandidate;
    }
};

[[nodiscard]] MotionQueryTransitionResultUVE ArbitrateMotionQueryTransitionUVE(
    MotionQueryTransitionRequestUVE request,
    MotionQueryTransitionSettingsUVE settings) noexcept;

} // namespace UVE::Plugins
