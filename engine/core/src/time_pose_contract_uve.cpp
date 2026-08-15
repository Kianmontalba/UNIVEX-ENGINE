// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/time_pose_contract_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

[[nodiscard]] double NonNegativeScaleUVE(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0 ? value : 0.0;
}

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

UnifiedTimeStateUVE UnifiedTimeContractUVE::AdvanceUVE(
    const UnifiedTimeStateUVE& previous, UnifiedTimeAdvanceInputUVE input,
    std::size_t& outFixedSteps, bool& outInputClamped) noexcept {
    UnifiedTimeStateUVE next = previous;
    outFixedSteps = 0U;
    outInputClamped = false;

    double realDelta = input.realDeltaSeconds;
    if (!std::isfinite(realDelta) || realDelta < 0.0) {
        realDelta = 0.0;
        outInputClamped = true;
    }
    if (realDelta > kMaximumFrameDeltaSecondsUVE) {
        realDelta = kMaximumFrameDeltaSecondsUVE;
        outInputClamped = true;
    }
    double fixedStep = input.fixedStepSeconds;
    if (!std::isfinite(fixedStep) || fixedStep <= 0.0) {
        fixedStep = 1.0 / 60.0;
        outInputClamped = true;
    }
    const std::size_t maximumFixedSteps = std::min(
        input.maximumFixedSteps, kMaximumFixedStepsPerAdvanceUVE);
    if (input.maximumFixedSteps > kMaximumFixedStepsPerAdvanceUVE) {
        outInputClamped = true;
    }
    const double gameScale = NonNegativeScaleUVE(input.gameTimeScale);
    const double animationScale = NonNegativeScaleUVE(input.animationTimeScale);
    if (!std::isfinite(input.gameTimeScale) || input.gameTimeScale < 0.0 ||
        !std::isfinite(input.animationTimeScale) || input.animationTimeScale < 0.0) {
        outInputClamped = true;
    }

    next.frameNumber = previous.frameNumber == std::numeric_limits<std::uint64_t>::max()
        ? previous.frameNumber : previous.frameNumber + 1U;
    next.realDeltaSeconds = realDelta;
    next.gameDeltaSeconds = input.paused ? 0.0 : realDelta * gameScale;
    next.animationDeltaSeconds = input.paused ? 0.0 : realDelta * animationScale;
    next.realTimeSeconds += realDelta;
    if (!input.paused) {
        next.gameTimeSeconds += next.gameDeltaSeconds;
        next.animationTimeSeconds += next.animationDeltaSeconds;
        next.fixedAccumulatorSeconds += realDelta;
        while (outFixedSteps < maximumFixedSteps && next.fixedAccumulatorSeconds >= fixedStep) {
            next.fixedAccumulatorSeconds -= fixedStep;
            next.fixedTimeSeconds += fixedStep;
            ++outFixedSteps;
        }
    }
    next.paused = input.paused;
    return next;
}

bool IsFiniteTransformPoseUVE(const TransformPoseUVE& pose) noexcept {
    return IsFiniteVectorUVE(pose.position) && IsFiniteVectorUVE(pose.scale) &&
           Math::IsFiniteUVE(pose.rotation);
}

bool TryNormalizeTransformPoseUVE(const TransformPoseUVE& pose,
                                  TransformPoseUVE& outNormalized) noexcept {
    if (!IsFiniteTransformPoseUVE(pose)) {
        return false;
    }
    Math::QuaternionUVE normalizedRotation;
    if (!Math::TryNormalizeUVE(pose.rotation, normalizedRotation)) {
        return false;
    }
    outNormalized = pose;
    outNormalized.rotation = normalizedRotation;
    return true;
}

} // namespace UVE::Core
