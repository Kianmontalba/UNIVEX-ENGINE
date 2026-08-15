// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

#include <cstddef>
#include <cstdint>

namespace UVE::Core {

struct UnifiedTimeStateUVE final {
    std::uint64_t frameNumber = 0U;
    double realTimeSeconds = 0.0;
    double gameTimeSeconds = 0.0;
    double fixedTimeSeconds = 0.0;
    double animationTimeSeconds = 0.0;
    double fixedAccumulatorSeconds = 0.0;
    double realDeltaSeconds = 0.0;
    double gameDeltaSeconds = 0.0;
    double animationDeltaSeconds = 0.0;
    bool paused = false;

    [[nodiscard]] bool operator==(const UnifiedTimeStateUVE&) const = default;
};

struct UnifiedTimeAdvanceInputUVE final {
    double realDeltaSeconds = 0.0;
    double gameTimeScale = 1.0;
    double animationTimeScale = 1.0;
    double fixedStepSeconds = 1.0 / 60.0;
    std::size_t maximumFixedSteps = 8U;
    bool paused = false;
};

struct UnifiedTimeAdvanceResultUVE final {
    UnifiedTimeStateUVE state;
    std::size_t fixedSteps = 0U;
    bool inputClamped = false;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return state.realDeltaSeconds >= 0.0 && state.gameDeltaSeconds >= 0.0 &&
               state.animationDeltaSeconds >= 0.0;
    }
};

// Shared time-domain state is a pure transition contract. The engine timer remains responsible
// for wall-clock sampling; this contract only advances copied values for game/fixed/animation users.
class UnifiedTimeContractUVE final {
public:
    static constexpr double kMaximumFrameDeltaSecondsUVE = 0.25;
    static constexpr std::size_t kMaximumFixedStepsPerAdvanceUVE = 8U;

    [[nodiscard]] static UnifiedTimeStateUVE AdvanceUVE(
        const UnifiedTimeStateUVE& previous, UnifiedTimeAdvanceInputUVE input,
        std::size_t& outFixedSteps, bool& outInputClamped) noexcept;
};

struct TransformPoseUVE final {
    Math::Vector3UVE position;
    Math::QuaternionUVE rotation;
    Math::Vector3UVE scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool operator==(const TransformPoseUVE&) const noexcept = default;
};

struct PoseSampleUVE final {
    double timeSeconds = 0.0;
    TransformPoseUVE pose;

    [[nodiscard]] bool operator==(const PoseSampleUVE&) const noexcept = default;
};

[[nodiscard]] bool IsFiniteTransformPoseUVE(const TransformPoseUVE& pose) noexcept;
[[nodiscard]] bool TryNormalizeTransformPoseUVE(const TransformPoseUVE& pose,
                                                 TransformPoseUVE& outNormalized) noexcept;

} // namespace UVE::Core
