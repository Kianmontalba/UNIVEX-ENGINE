// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/future_trajectory_contract_uve.h"
#include "uve/physics/shape_cast_system_uve.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Physics {

enum class TrajectoryCollisionPredictionCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidRequest,
    InvalidTrajectory,
    InvalidShape,
    SweepFailed,
};

struct TrajectoryCollisionPredictionRequestUVE final {
    Math::Vector3UVE origin;
    Core::TimeSampledTrajectoryUVE trajectory;
    float defaultCapsuleRadius = 0.35F;
    float defaultCapsuleHalfHeight = 0.9F;
    std::uint32_t layerMask = 0xFFFFFFFFU;
    Scene::EntityUVE ignoreEntity{};
};

struct TrajectoryCollisionPredictionSampleUVE final {
    double offsetSeconds = 0.0;
    Math::Vector3UVE center;
    float capsuleRadius = 0.0F;
    float capsuleHalfHeight = 0.0F;
    bool swept = false;
    std::optional<CapsuleCastHitUVE> hit;
};

struct TrajectoryCollisionPredictionResultUVE final {
    TrajectoryCollisionPredictionCodeUVE code =
        TrajectoryCollisionPredictionCodeUVE::InvalidRequest;
    Core::AnimationMotionContextUVE context = Core::AnimationMotionContextUVE::Locomotion;
    std::vector<TrajectoryCollisionPredictionSampleUVE> samples;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return code == TrajectoryCollisionPredictionCodeUVE::Valid;
    }
};

[[nodiscard]] bool PredictTrajectoryCollisionsUVE(
    Scene::IEntityManagerUVE& entityManager, const TrajectoryCollisionPredictionRequestUVE& request,
    TrajectoryCollisionPredictionResultUVE& outResult);

[[nodiscard]] TrajectoryCollisionPredictionResultUVE PredictTrajectoryCollisionsUVE(
    Scene::IEntityManagerUVE& entityManager, const TrajectoryCollisionPredictionRequestUVE& request);

} // namespace UVE::Physics
