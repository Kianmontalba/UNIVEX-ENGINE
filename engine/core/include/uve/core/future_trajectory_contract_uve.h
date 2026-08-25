// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/math/vector3_uve.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace UVE::Core {

enum class AnimationMotionContextUVE : std::uint8_t {
    Locomotion = 0,
    Turn,
    Hop,
    Slide,
    Jump,
    Fall,
    LightLanding,
    HeavyLanding,
    Takedown,
    Ragdoll,
    Combat,
    Interaction,
    Custom,
};

struct TimeSampledTrajectorySampleUVE final {
    double offsetSeconds = 0.0;
    Math::Vector3UVE relativePosition;
    Math::Vector3UVE velocity;
    Math::Vector3UVE facingDirection{0.0F, 0.0F, 1.0F};
    /// Zero radius and zero half-height mean that the caller did not provide collision shape data.
    float capsuleRadius = 0.0F;
    float capsuleHalfHeight = 0.0F;

    TimeSampledTrajectorySampleUVE() = default;
    TimeSampledTrajectorySampleUVE(double inOffsetSeconds, Math::Vector3UVE inRelativePosition) noexcept
        : offsetSeconds(inOffsetSeconds), relativePosition(inRelativePosition) {}
    TimeSampledTrajectorySampleUVE(double inOffsetSeconds, Math::Vector3UVE inRelativePosition,
                                   Math::Vector3UVE inVelocity, Math::Vector3UVE inFacingDirection,
                                   float inCapsuleRadius, float inCapsuleHalfHeight) noexcept
        : offsetSeconds(inOffsetSeconds), relativePosition(inRelativePosition), velocity(inVelocity),
          facingDirection(inFacingDirection), capsuleRadius(inCapsuleRadius),
          capsuleHalfHeight(inCapsuleHalfHeight) {}

    [[nodiscard]] bool operator==(const TimeSampledTrajectorySampleUVE&) const noexcept = default;
};

struct TimeSampledTrajectoryUVE final {
    static constexpr std::uint32_t kCurrentSchemaVersionUVE = 1U;
    static constexpr std::size_t kMaximumSamplesUVE = 32U;

    std::uint32_t schemaVersion = kCurrentSchemaVersionUVE;
    AnimationMotionContextUVE context = AnimationMotionContextUVE::Locomotion;
    std::vector<TimeSampledTrajectorySampleUVE> samples;

    TimeSampledTrajectoryUVE() = default;
    TimeSampledTrajectoryUVE(const std::vector<TimeSampledTrajectorySampleUVE>& values) : samples(values) {}

    [[nodiscard]] bool empty() const noexcept { return samples.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return samples.size(); }
    [[nodiscard]] TimeSampledTrajectorySampleUVE& operator[](const std::size_t index) noexcept {
        return samples[index];
    }
    [[nodiscard]] const TimeSampledTrajectorySampleUVE& operator[](const std::size_t index) const noexcept {
        return samples[index];
    }
    [[nodiscard]] TimeSampledTrajectorySampleUVE& front() noexcept { return samples.front(); }
    [[nodiscard]] const TimeSampledTrajectorySampleUVE& front() const noexcept { return samples.front(); }
    [[nodiscard]] TimeSampledTrajectorySampleUVE& back() noexcept { return samples.back(); }
    [[nodiscard]] const TimeSampledTrajectorySampleUVE& back() const noexcept { return samples.back(); }
    [[nodiscard]] auto begin() noexcept { return samples.begin(); }
    [[nodiscard]] auto end() noexcept { return samples.end(); }
    [[nodiscard]] auto begin() const noexcept { return samples.begin(); }
    [[nodiscard]] auto end() const noexcept { return samples.end(); }
    void clear() noexcept { samples.clear(); }
    void push_back(const TimeSampledTrajectorySampleUVE& sample) { samples.push_back(sample); }
    void pop_back() noexcept { samples.pop_back(); }
    void reserve(const std::size_t capacity) { samples.reserve(capacity); }
    void operator=(std::initializer_list<TimeSampledTrajectorySampleUVE> values) { samples = values; }
    void operator=(const std::vector<TimeSampledTrajectorySampleUVE>& values) { samples = values; }

    [[nodiscard]] bool operator==(const TimeSampledTrajectoryUVE&) const noexcept = default;
    [[nodiscard]] bool operator==(const std::vector<TimeSampledTrajectorySampleUVE>& values) const noexcept {
        return samples == values;
    }
};

using MotionTrajectorySampleUVE = TimeSampledTrajectorySampleUVE;

[[nodiscard]] inline bool operator==(const std::vector<TimeSampledTrajectorySampleUVE>& values,
                                     const TimeSampledTrajectoryUVE& trajectory) noexcept {
    return trajectory == values;
}

enum class TimeSampledTrajectoryValidationCodeUVE : std::uint8_t {
    Valid = 0,
    CapacityExceeded,
    InvalidSchema,
    InvalidContext,
    InvalidTime,
    UnsortedSamples,
    InvalidVector,
    InvalidShape,
};

struct TimeSampledTrajectoryValidationResultUVE final {
    TimeSampledTrajectoryValidationCodeUVE code = TimeSampledTrajectoryValidationCodeUVE::InvalidSchema;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == TimeSampledTrajectoryValidationCodeUVE::Valid;
    }
};

[[nodiscard]] inline TimeSampledTrajectoryValidationResultUVE ValidateTimeSampledTrajectoryUVE(
    const TimeSampledTrajectoryUVE& trajectory) noexcept {
    const auto makeError = [](const TimeSampledTrajectoryValidationCodeUVE code,
                              const std::size_t index, const char* message) noexcept {
        return TimeSampledTrajectoryValidationResultUVE{code, index, message};
    };
    const auto finiteVector = [](const Math::Vector3UVE& value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (trajectory.samples.size() > TimeSampledTrajectoryUVE::kMaximumSamplesUVE) {
        return makeError(TimeSampledTrajectoryValidationCodeUVE::CapacityExceeded, 0U,
                         "time-sampled trajectory exceeds its bounded capacity");
    }
    if (trajectory.schemaVersion != TimeSampledTrajectoryUVE::kCurrentSchemaVersionUVE) {
        return makeError(TimeSampledTrajectoryValidationCodeUVE::InvalidSchema, 0U,
                         "time-sampled trajectory schema version is unsupported");
    }
    if (static_cast<std::uint8_t>(trajectory.context) >
        static_cast<std::uint8_t>(AnimationMotionContextUVE::Custom)) {
        return makeError(TimeSampledTrajectoryValidationCodeUVE::InvalidContext, 0U,
                         "time-sampled trajectory context is unsupported");
    }
    constexpr float minimumDimension = 1.0e-4F;
    constexpr float maximumDimension = 1.0e4F;
    double previousOffsetSeconds = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < trajectory.samples.size(); ++index) {
        const TimeSampledTrajectorySampleUVE& sample = trajectory.samples[index];
        if (!std::isfinite(sample.offsetSeconds) || sample.offsetSeconds < 0.0) {
            return makeError(TimeSampledTrajectoryValidationCodeUVE::InvalidTime, index,
                             "time-sampled trajectory offset must be finite and non-negative");
        }
        if (sample.offsetSeconds < previousOffsetSeconds) {
            return makeError(TimeSampledTrajectoryValidationCodeUVE::UnsortedSamples, index,
                             "time-sampled trajectory offsets must be sorted");
        }
        if (!finiteVector(sample.relativePosition) || !finiteVector(sample.velocity) ||
            !finiteVector(sample.facingDirection)) {
            return makeError(TimeSampledTrajectoryValidationCodeUVE::InvalidVector, index,
                             "time-sampled trajectory contains a non-finite vector");
        }
        const bool hasRadius = sample.capsuleRadius > 0.0F;
        const bool hasHalfHeight = sample.capsuleHalfHeight > 0.0F;
        if (!std::isfinite(sample.capsuleRadius) || !std::isfinite(sample.capsuleHalfHeight) ||
            hasRadius != hasHalfHeight ||
            (hasRadius && (sample.capsuleRadius < minimumDimension ||
                           sample.capsuleHalfHeight < minimumDimension ||
                           sample.capsuleRadius > maximumDimension ||
                           sample.capsuleHalfHeight > maximumDimension))) {
            return makeError(TimeSampledTrajectoryValidationCodeUVE::InvalidShape, index,
                             "time-sampled trajectory capsule dimensions are invalid");
        }
        previousOffsetSeconds = sample.offsetSeconds;
    }
    return TimeSampledTrajectoryValidationResultUVE{
        TimeSampledTrajectoryValidationCodeUVE::Valid, 0U, "valid"};
}

[[nodiscard]] inline bool TryBuildTimeSampledTrajectoryUVE(
    const AnimationMotionContextUVE context, const std::vector<TimeSampledTrajectorySampleUVE>& samples,
    TimeSampledTrajectoryUVE& outTrajectory) noexcept {
    TimeSampledTrajectoryUVE candidate;
    candidate.context = context;
    candidate.samples = samples;
    if (!ValidateTimeSampledTrajectoryUVE(candidate).IsValidUVE()) {
        return false;
    }
    outTrajectory = std::move(candidate);
    return true;
}

} // namespace UVE::Core
