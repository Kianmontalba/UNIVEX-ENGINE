// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_clip_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

[[nodiscard]] bool IsFiniteNonNegativeUVE(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] double NormalizeSampleTimeUVE(const double timeSeconds, const double durationSeconds,
                                            const bool looping) noexcept {
    if (!looping || durationSeconds <= 0.0) {
        return std::clamp(timeSeconds, 0.0, durationSeconds);
    }
    double normalized = std::fmod(timeSeconds, durationSeconds);
    if (normalized < 0.0) {
        normalized += durationSeconds;
    }
    return normalized;
}

[[nodiscard]] TransformPoseUVE InterpolatePoseUVE(const TransformPoseUVE& left,
                                                  const TransformPoseUVE& right,
                                                  const double alpha) noexcept {
    TransformPoseUVE result;
    const float factor = static_cast<float>(std::clamp(alpha, 0.0, 1.0));
    result.position = left.position * (1.0F - factor) + right.position * factor;
    result.scale = left.scale * (1.0F - factor) + right.scale * factor;
    result.rotation = Math::QuaternionUVE{
        left.rotation.x * (1.0F - factor) + right.rotation.x * factor,
        left.rotation.y * (1.0F - factor) + right.rotation.y * factor,
        left.rotation.z * (1.0F - factor) + right.rotation.z * factor,
        left.rotation.w * (1.0F - factor) + right.rotation.w * factor,
    };
    TransformPoseUVE normalized;
    return TryNormalizeTransformPoseUVE(result, normalized) ? normalized : left;
}

} // namespace

AnimationClipValidationResultUVE ValidateAnimationClipUVE(const AnimationClipUVE& clip) noexcept {
    if (clip.clipId.empty() || clip.clipId.size() > AnimationClipUVE::kMaximumIdentifierBytesUVE) {
        return {AnimationClipValidationCodeUVE::InvalidIdentifier, 0U,
                "Animation clip identifier is empty or exceeds the bounded length."};
    }
    if (!IsFiniteNonNegativeUVE(clip.durationSeconds) || clip.durationSeconds <= 0.0) {
        return {AnimationClipValidationCodeUVE::InvalidDuration, 0U,
                "Animation clip duration must be finite and greater than zero."};
    }
    if (clip.samples.empty() || clip.samples.size() > AnimationClipUVE::kMaximumSamplesUVE ||
        clip.events.size() > AnimationClipUVE::kMaximumEventsUVE) {
        return {AnimationClipValidationCodeUVE::CapacityExceeded, 0U,
                "Animation clip samples/events exceed the bounded contract."};
    }
    double previousTime = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < clip.samples.size(); ++index) {
        const PoseSampleUVE& sample = clip.samples[index];
        if (!IsFiniteNonNegativeUVE(sample.timeSeconds) || sample.timeSeconds > clip.durationSeconds) {
            return {AnimationClipValidationCodeUVE::InvalidSampleTime, index,
                    "Animation sample time is outside the clip duration."};
        }
        if (sample.timeSeconds < previousTime) {
            return {AnimationClipValidationCodeUVE::UnsortedSamples, index,
                    "Animation samples must be sorted by non-decreasing time."};
        }
        if (!IsFiniteTransformPoseUVE(sample.pose)) {
            return {AnimationClipValidationCodeUVE::InvalidPose, index,
                    "Animation sample pose contains non-finite values."};
        }
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(sample.pose, normalized)) {
            return {AnimationClipValidationCodeUVE::InvalidPose, index,
                    "Animation sample rotation must be finite and non-zero."};
        }
        previousTime = sample.timeSeconds;
    }
    previousTime = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < clip.events.size(); ++index) {
        const AnimationEventUVE& event = clip.events[index];
        if (!IsFiniteNonNegativeUVE(event.timeSeconds) || event.timeSeconds > clip.durationSeconds) {
            return {AnimationClipValidationCodeUVE::InvalidEventTime, index,
                    "Animation event time is outside the clip duration."};
        }
        if (event.timeSeconds < previousTime) {
            return {AnimationClipValidationCodeUVE::UnsortedEvents, index,
                    "Animation events must be sorted by non-decreasing time."};
        }
        if (event.eventId.empty() || event.eventId.size() > AnimationClipUVE::kMaximumIdentifierBytesUVE) {
            return {AnimationClipValidationCodeUVE::InvalidEventIdentifier, index,
                    "Animation event identifier is empty or exceeds the bounded length."};
        }
        previousTime = event.timeSeconds;
    }
    return {AnimationClipValidationCodeUVE::Valid, 0U, "Animation clip is valid."};
}

bool TrySampleAnimationClipUVE(const AnimationClipUVE& clip, const double timeSeconds,
                              const bool looping, TransformPoseUVE& outPose) noexcept {
    if (!ValidateAnimationClipUVE(clip).IsValidUVE() || !std::isfinite(timeSeconds)) {
        return false;
    }
    const double sampleTime = NormalizeSampleTimeUVE(timeSeconds, clip.durationSeconds, looping);
    const auto upper = std::upper_bound(clip.samples.begin(), clip.samples.end(), sampleTime,
                                        [](const double value, const PoseSampleUVE& sample) {
                                            return value < sample.timeSeconds;
                                        });
    if (upper == clip.samples.begin()) {
        return TryNormalizeTransformPoseUVE(clip.samples.front().pose, outPose);
    }
    if (upper == clip.samples.end()) {
        return TryNormalizeTransformPoseUVE(clip.samples.back().pose, outPose);
    }
    const PoseSampleUVE& right = *upper;
    const PoseSampleUVE& left = *(upper - 1);
    const double span = right.timeSeconds - left.timeSeconds;
    const double alpha = span <= 0.0 ? 0.0 : (sampleTime - left.timeSeconds) / span;
    outPose = InterpolatePoseUVE(left.pose, right.pose, alpha);
    return true;
}

std::vector<AnimationEventUVE> CollectAnimationEventsUVE(const AnimationClipUVE& clip,
                                                         const double startSeconds,
                                                         const double endSeconds,
                                                         const bool looping) {
    std::vector<AnimationEventUVE> result;
    if (!ValidateAnimationClipUVE(clip).IsValidUVE() || !std::isfinite(startSeconds) ||
        !std::isfinite(endSeconds) || endSeconds < startSeconds) {
        return result;
    }
    if (!looping || endSeconds <= clip.durationSeconds) {
        for (const AnimationEventUVE& event : clip.events) {
            if (event.timeSeconds > startSeconds && event.timeSeconds <= endSeconds) {
                result.push_back(event);
            }
        }
        return result;
    }
    const double firstLoop = std::floor(startSeconds / clip.durationSeconds);
    const double lastLoop = std::floor(endSeconds / clip.durationSeconds);
    if (!std::isfinite(firstLoop) || !std::isfinite(lastLoop) ||
        firstLoop < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
        firstLoop >= static_cast<double>(std::numeric_limits<std::int64_t>::max()) ||
        lastLoop < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
        lastLoop >= static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
        return result;
    }
    const std::int64_t firstLoopIndex = static_cast<std::int64_t>(firstLoop);
    const std::int64_t lastLoopIndex = static_cast<std::int64_t>(lastLoop);
    const std::uint64_t loopSpan = static_cast<std::uint64_t>(lastLoopIndex) -
                                   static_cast<std::uint64_t>(firstLoopIndex);
    const std::uint64_t maximumLoops =
        (AnimationClipUVE::kMaximumCollectedEventsUVE / std::max<std::size_t>(clip.events.size(), 1U)) + 1U;
    if (loopSpan >= maximumLoops) {
        return result;
    }
    result.reserve(std::min(AnimationClipUVE::kMaximumCollectedEventsUVE,
                            clip.events.size() * static_cast<std::size_t>(loopSpan + 1U)));
    for (std::int64_t loop = firstLoopIndex;; ++loop) {
        const double offset = static_cast<double>(loop) * clip.durationSeconds;
        for (const AnimationEventUVE& event : clip.events) {
            const double occurrence = offset + event.timeSeconds;
            if (occurrence > startSeconds && occurrence <= endSeconds) {
                if (result.size() >= AnimationClipUVE::kMaximumCollectedEventsUVE) {
                    return result;
                }
                result.push_back(event);
            }
        }
        if (loop == lastLoopIndex) {
            break;
        }
    }
    return result;
}

} // namespace UVE::Core
