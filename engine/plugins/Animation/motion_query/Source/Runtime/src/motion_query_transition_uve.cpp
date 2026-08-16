// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_transition_uve.h"

#include <cmath>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryTransitionResultUVE MakeTransitionResultUVE(
    const MotionQueryTransitionCodeUVE code, const float improvement,
    const char* message) noexcept {
    return MotionQueryTransitionResultUVE{code, improvement, message};
}
}

MotionQueryTransitionResultUVE ArbitrateMotionQueryTransitionUVE(
    const MotionQueryTransitionRequestUVE request,
    const MotionQueryTransitionSettingsUVE settings) noexcept {
    if (!std::isfinite(settings.minimumCostImprovement) || settings.minimumCostImprovement < 0.0F ||
        settings.minimumCostImprovement > MotionQueryTransitionSettingsUVE::kMaximumCostImprovementUVE ||
        !std::isfinite(settings.minimumHoldSeconds) || settings.minimumHoldSeconds < 0.0 ||
        !std::isfinite(settings.maximumHoldWindowSeconds) || settings.maximumHoldWindowSeconds < 0.0 ||
        settings.maximumHoldWindowSeconds > MotionQueryTransitionSettingsUVE::kMaximumHoldWindowSecondsUVE) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::InvalidRequest, 0.0F,
                                       "motion query transition settings are invalid");
    }
    if (!request.hasPreviousSelection) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::NoPreviousSelection, 0.0F,
                                       "motion query transition has no previous selection");
    }
    if (!std::isfinite(request.previousCost) || request.previousCost < 0.0F ||
        !std::isfinite(request.currentCost) || request.currentCost < 0.0F ||
        !std::isfinite(request.elapsedSeconds) || request.elapsedSeconds < 0.0) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::InvalidRequest, 0.0F,
                                       "motion query transition request is invalid");
    }
    if (request.previousCandidateIndex == request.currentCandidateIndex) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::SameCandidate, 0.0F,
                                       "motion query transition retained the same candidate");
    }
    const float improvement = request.previousCost - request.currentCost;
    if (request.elapsedSeconds > settings.maximumHoldWindowSeconds) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::HoldWindowExpired, improvement,
                                       "motion query transition hold window expired");
    }
    if (request.elapsedSeconds < settings.minimumHoldSeconds &&
        improvement < settings.minimumCostImprovement) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::HeldPreviousCandidate,
                                       improvement,
                                       "motion query transition held the previous candidate");
    }
    if (improvement >= settings.minimumCostImprovement) {
        return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::SwitchedCandidate, improvement,
                                       "motion query transition switched to the improved candidate");
    }
    return MakeTransitionResultUVE(MotionQueryTransitionCodeUVE::HeldPreviousCandidate, improvement,
                                   "motion query transition retained the previous candidate");
}

} // namespace UVE::Plugins
