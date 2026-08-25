// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/trajectory_collision_prediction_uve.h"

#include <cmath>
#include <utility>

namespace UVE::Physics {
namespace {

constexpr float kMinimumCapsuleDimensionUVE = 1.0e-4F;
constexpr float kMaximumCapsuleDimensionUVE = 1.0e4F;

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsValidCapsuleDimensionUVE(const float value) noexcept {
    return std::isfinite(value) && value >= kMinimumCapsuleDimensionUVE &&
           value <= kMaximumCapsuleDimensionUVE;
}

[[nodiscard]] TrajectoryCollisionPredictionResultUVE MakeErrorUVE(
    const TrajectoryCollisionPredictionCodeUVE code, Core::AnimationMotionContextUVE context,
    const char* message) noexcept {
    TrajectoryCollisionPredictionResultUVE result;
    result.code = code;
    result.context = context;
    result.message = message;
    return result;
}

} // namespace

bool PredictTrajectoryCollisionsUVE(
    Scene::IEntityManagerUVE& entityManager, const TrajectoryCollisionPredictionRequestUVE& request,
    TrajectoryCollisionPredictionResultUVE& outResult) {
    if (!IsFiniteVectorUVE(request.origin) || request.layerMask == 0U ||
        !IsValidCapsuleDimensionUVE(request.defaultCapsuleRadius) ||
        !IsValidCapsuleDimensionUVE(request.defaultCapsuleHalfHeight)) {
        return false;
    }
    const Core::TimeSampledTrajectoryValidationResultUVE trajectoryValidation =
        Core::ValidateTimeSampledTrajectoryUVE(request.trajectory);
    if (!trajectoryValidation.IsValidUVE()) {
        return false;
    }

    TrajectoryCollisionPredictionResultUVE candidate;
    candidate.code = TrajectoryCollisionPredictionCodeUVE::Valid;
    candidate.context = request.trajectory.context;
    candidate.message = "Trajectory collision prediction completed.";
    candidate.samples.reserve(request.trajectory.samples.size());

    Math::Vector3UVE previousCenter = request.origin;
    for (const Core::TimeSampledTrajectorySampleUVE& trajectorySample : request.trajectory.samples) {
        const Math::Vector3UVE center = request.origin + trajectorySample.relativePosition;
        const float radius = trajectorySample.capsuleRadius > 0.0F
            ? trajectorySample.capsuleRadius : request.defaultCapsuleRadius;
        const float halfHeight = trajectorySample.capsuleHalfHeight > 0.0F
            ? trajectorySample.capsuleHalfHeight : request.defaultCapsuleHalfHeight;
        if (!IsFiniteVectorUVE(center) || !IsValidCapsuleDimensionUVE(radius) ||
            !IsValidCapsuleDimensionUVE(halfHeight)) {
            return false;
        }

        TrajectoryCollisionPredictionSampleUVE preview;
        preview.offsetSeconds = trajectorySample.offsetSeconds;
        preview.center = center;
        preview.capsuleRadius = radius;
        preview.capsuleHalfHeight = halfHeight;
        const Math::Vector3UVE delta = center - previousCenter;
        const float distance = Math::LengthUVE(delta);
        if (!std::isfinite(distance) || distance < 0.0F) {
            return false;
        }
        if (distance > 0.0F) {
            CapsuleCastQueryUVE query;
            query.ray = Math::RayUVE{previousCenter, Math::NormalizeUVE(delta)};
            query.radius = radius;
            query.height = halfHeight * 2.0F;
            query.maxDistance = distance;
            query.layerMask = request.layerMask;
            query.ignoreEntity = request.ignoreEntity;
            preview.swept = true;
            preview.hit = ShapeCastSystemUVE::CapsuleCastUVE(entityManager, query);
        }
        candidate.samples.push_back(std::move(preview));
        previousCenter = center;
    }

    outResult = std::move(candidate);
    return true;
}

TrajectoryCollisionPredictionResultUVE PredictTrajectoryCollisionsUVE(
    Scene::IEntityManagerUVE& entityManager, const TrajectoryCollisionPredictionRequestUVE& request) {
    TrajectoryCollisionPredictionResultUVE result;
    if (!PredictTrajectoryCollisionsUVE(entityManager, request, result)) {
        return MakeErrorUVE(TrajectoryCollisionPredictionCodeUVE::InvalidRequest,
                            request.trajectory.context,
                            "Trajectory collision prediction request is invalid.");
    }
    return result;
}

} // namespace UVE::Physics
