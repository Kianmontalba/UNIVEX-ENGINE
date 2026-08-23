// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_continuity_uve.h"

#include <cmath>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryContinuityResultUVE MakeContinuityResultUVE(
    const MotionQueryContinuityCodeUVE code, const double age,
    const UVE::Core::TransformPoseUVE& pose, const char* message) noexcept {
    MotionQueryContinuityResultUVE result;
    result.code = code;
    result.previousAgeSeconds = age;
    result.pose = pose;
    result.message = message;
    return result;
}

[[nodiscard]] UVE::Math::Vector3UVE BlendVectorUVE(const UVE::Math::Vector3UVE& previous,
                                                    const UVE::Math::Vector3UVE& current) noexcept {
    return UVE::Math::Vector3UVE{
        static_cast<float>((static_cast<double>(previous.x) + static_cast<double>(current.x)) * 0.5),
        static_cast<float>((static_cast<double>(previous.y) + static_cast<double>(current.y)) * 0.5),
        static_cast<float>((static_cast<double>(previous.z) + static_cast<double>(current.z)) * 0.5)};
}
} // namespace

MotionQueryContinuityResultUVE ApplyMotionQueryContinuityUVE(
    const UVE::Core::TransformPoseUVE& currentPose,
    const UVE::Core::PoseSampleUVE* previousSample,
    const double currentTimeSeconds,
    const MotionQueryContinuitySettingsUVE settings) noexcept {
    if (settings.policy != MotionQueryContinuityPolicyUVE::Disabled &&
        settings.policy != MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::InvalidSettings, 0.0,
                                       currentPose,
                                       "motion query continuity policy is unsupported");
    }
    if (!std::isfinite(currentTimeSeconds) || currentTimeSeconds < 0.0 ||
        !std::isfinite(settings.maximumPreviousAgeSeconds) ||
        settings.maximumPreviousAgeSeconds < 0.0) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::InvalidSettings, 0.0,
                                       currentPose,
                                       "motion query continuity settings are invalid");
    }
    if (!UVE::Core::IsFiniteTransformPoseUVE(currentPose)) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::InvalidPose, 0.0, currentPose,
                                       "motion query continuity current pose is invalid");
    }
    if (settings.policy == MotionQueryContinuityPolicyUVE::Disabled) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::Disabled, 0.0, currentPose,
                                       "motion query continuity is disabled");
    }
    if (previousSample == nullptr) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::NoPreviousSample, 0.0,
                                       currentPose,
                                       "motion query continuity has no previous pose sample");
    }
    if (!std::isfinite(previousSample->timeSeconds) || previousSample->timeSeconds < 0.0 ||
        previousSample->timeSeconds > currentTimeSeconds ||
        !UVE::Core::IsFiniteTransformPoseUVE(previousSample->pose)) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::InvalidPose, 0.0, currentPose,
                                       "motion query continuity previous pose is invalid");
    }
    const double age = currentTimeSeconds - previousSample->timeSeconds;
    if (age > settings.maximumPreviousAgeSeconds) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::PreviousSampleTooOld, age,
                                       currentPose,
                                       "motion query continuity previous pose is outside the age window");
    }
    UVE::Core::TransformPoseUVE blended = currentPose;
    blended.position = BlendVectorUVE(previousSample->pose.position, currentPose.position);
    blended.scale = BlendVectorUVE(previousSample->pose.scale, currentPose.scale);
    if (!UVE::Core::IsFiniteTransformPoseUVE(blended)) {
        return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::InvalidPose, age, currentPose,
                                       "motion query continuity blended pose is invalid");
    }
    // Quaternion interpolation is intentionally not performed until an authoritative SLERP
    // contract exists in the math layer. Current sampled rotation remains authoritative.
    return MakeContinuityResultUVE(MotionQueryContinuityCodeUVE::Applied, age, blended,
                                   "motion query continuity blended previous translation and scale");
}

} // namespace UVE::Plugins
