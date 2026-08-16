// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstdint>
#include <string>

namespace UVE::Plugins {

enum class MotionQueryContinuityPolicyUVE : std::uint8_t {
    Disabled = 0,
    BlendPreviousWithinWindow,
};

struct MotionQueryContinuitySettingsUVE final {
    MotionQueryContinuityPolicyUVE policy = MotionQueryContinuityPolicyUVE::Disabled;
    double maximumPreviousAgeSeconds = 0.25;
};

enum class MotionQueryContinuityCodeUVE : std::uint8_t {
    Disabled = 0,
    Applied,
    NoPreviousSample,
    PreviousSampleTooOld,
    InvalidSettings,
    InvalidPose,
};

struct MotionQueryContinuityResultUVE final {
    MotionQueryContinuityCodeUVE code = MotionQueryContinuityCodeUVE::Disabled;
    double previousAgeSeconds = 0.0;
    UVE::Core::TransformPoseUVE pose;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code != MotionQueryContinuityCodeUVE::InvalidSettings &&
               code != MotionQueryContinuityCodeUVE::InvalidPose;
    }

    [[nodiscard]] bool WasAppliedUVE() const noexcept {
        return code == MotionQueryContinuityCodeUVE::Applied;
    }
};

[[nodiscard]] MotionQueryContinuityResultUVE ApplyMotionQueryContinuityUVE(
    const UVE::Core::TransformPoseUVE& currentPose,
    const UVE::Core::PoseSampleUVE* previousSample,
    double currentTimeSeconds,
    MotionQueryContinuitySettingsUVE settings) noexcept;

} // namespace UVE::Plugins
