// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/character_controller_uve.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] float FiniteLengthUVE(const Math::Vector3UVE& value) noexcept {
    const float lengthSquared = Math::LengthSquaredUVE(value);
    return std::isfinite(lengthSquared) && lengthSquared >= 0.0F ? std::sqrt(lengthSquared) : 0.0F;
}

[[nodiscard]] Math::Vector3UVE RemoveIntoNormalComponentUVE(
    const Math::Vector3UVE& displacement, const Math::Vector3UVE& normal) noexcept {
    const float normalLengthSquared = Math::LengthSquaredUVE(normal);
    if (!std::isfinite(normalLengthSquared) || normalLengthSquared <= 0.0F) {
        return displacement;
    }
    const float intoSurface = Math::DotUVE(displacement, normal);
    if (!std::isfinite(intoSurface) || intoSurface <= 0.0F) {
        return displacement;
    }
    return displacement - normal * (intoSurface / normalLengthSquared);
}

bool ValidateControllerInputUVE(Scene::IEntityManagerUVE& entityManager,
                                  const CharacterControllerInputUVE& input,
                                  CharacterControllerMoveResultUVE& result) {
    if (!entityManager.IsAliveUVE(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::InvalidEntity;
        return false;
    }
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(input.entity) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::MissingTransform;
        return false;
    }
    if (!entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::MissingCollider;
        return false;
    }
    if (!IsFiniteVectorUVE(input.desiredDisplacement) ||
        !Scene::IsColliderComponentValidUVE(
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(input.entity))) {
        result.code = CharacterControllerMoveCodeUVE::InvalidInput;
        return false;
    }
    if (entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(input.entity)) {
        const Scene::RigidBodyComponentUVE& rigidBody =
            entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(input.entity);
        if (!Scene::IsRigidBodyComponentValidUVE(rigidBody)) {
            result.code = CharacterControllerMoveCodeUVE::InvalidInput;
            return false;
        }
        if (!rigidBody.isKinematic) {
            result.code = CharacterControllerMoveCodeUVE::NonKinematicBody;
            return false;
        }
    }
    return true;
}

void ApplyLocalDeltaUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                        Scene::EntityUVE entity, const Math::Vector3UVE& delta) {
    Scene::TransformComponentUVE transform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    transform.localPosition += delta;
    sceneGraph.SetLocalTransformUVE(entityManager, entity, transform);
}

[[nodiscard]] float ResolveGroundSlopeCosineUVE(
    const CharacterControllerInputUVE& input, CharacterControllerMoveResultUVE& result) noexcept {
    float degrees = input.maximumGroundSlopeDegrees;
    if (!std::isfinite(degrees) || degrees < 0.0F) {
        degrees = CharacterControllerUVE::kDefaultMaximumGroundSlopeDegreesUVE;
        result.inputClamped = true;
    }
    if (degrees > CharacterControllerUVE::kMaximumGroundSlopeDegreesUVE) {
        degrees = CharacterControllerUVE::kMaximumGroundSlopeDegreesUVE;
        result.inputClamped = true;
    }
    constexpr float kPiUVE = 3.14159265358979323846F;
    return std::cos(degrees * (kPiUVE / 180.0F));
}

void RegisterGroundContactUVE(CharacterControllerMoveResultUVE& result,
                              const Math::Vector3UVE& controllerToTargetNormal,
                              const float minimumGroundNormalY) noexcept {
    const float lengthSquared = Math::LengthSquaredUVE(controllerToTargetNormal);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0F) {
        return;
    }
    const Math::Vector3UVE supportNormal =
        -controllerToTargetNormal * (1.0F / std::sqrt(lengthSquared));
    if (!std::isfinite(supportNormal.y) || supportNormal.y < minimumGroundNormalY) {
        return;
    }
    if (!result.grounded || supportNormal.y > result.groundNormal.y) {
        result.grounded = true;
        result.groundNormal = supportNormal;
    }
}

struct DynamicBodyPushPolicyUVE final {
    bool enabled = false;
    float strength = 0.0F;
    float maximumSpeed = 0.0F;
    float deltaTimeSeconds = 0.0F;
};

[[nodiscard]] DynamicBodyPushPolicyUVE ResolveDynamicBodyPushPolicyUVE(
    const CharacterControllerInputUVE& input, CharacterControllerMoveResultUVE& result) noexcept {
    DynamicBodyPushPolicyUVE policy;
    if (!input.pushDynamicBodies) {
        return policy;
    }
    policy.enabled = true;
    policy.strength = input.dynamicBodyPushStrength;
    if (!std::isfinite(policy.strength) || policy.strength < 0.0F) {
        policy.strength = 0.0F;
        result.dynamicBodyPushClamped = true;
        result.inputClamped = true;
    }
    policy.maximumSpeed = input.maximumDynamicBodyPushSpeed;
    if (!std::isfinite(policy.maximumSpeed) || policy.maximumSpeed <= 0.0F) {
        policy.maximumSpeed = 5.0F;
        result.dynamicBodyPushClamped = true;
        result.inputClamped = true;
    }
    policy.deltaTimeSeconds = input.dynamicBodyPushDeltaTimeSeconds;
    if (!std::isfinite(policy.deltaTimeSeconds) || policy.deltaTimeSeconds <= 0.0F) {
        policy.deltaTimeSeconds = 1.0F / 60.0F;
        result.dynamicBodyPushClamped = true;
        result.inputClamped = true;
    }
    return policy;
}

[[nodiscard]] bool ApplyDynamicBodyPushUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE targetEntity,
    const Math::Vector3UVE& contactNormal, const Math::Vector3UVE& controllerDisplacement,
    const DynamicBodyPushPolicyUVE& policy) {
    if (!policy.enabled || policy.strength <= 0.0F || !std::isfinite(policy.maximumSpeed) ||
        policy.maximumSpeed <= 0.0F || !std::isfinite(policy.deltaTimeSeconds) ||
        policy.deltaTimeSeconds <= 0.0F || !entityManager.IsAliveUVE(targetEntity) ||
        !entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(targetEntity)) {
        return false;
    }
    const float normalLengthSquared = Math::LengthSquaredUVE(contactNormal);
    if (!std::isfinite(normalLengthSquared) || normalLengthSquared <= 0.0F ||
        !IsFiniteVectorUVE(controllerDisplacement)) {
        return false;
    }
    const Math::Vector3UVE normal = contactNormal * (1.0F / std::sqrt(normalLengthSquared));
    const float intoTarget = Math::DotUVE(controllerDisplacement, normal);
    if (!std::isfinite(intoTarget) || intoTarget <= 0.0F) {
        return false;
    }

    Scene::RigidBodyComponentUVE& rigidBody =
        entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(targetEntity);
    if (!Scene::IsRigidBodyComponentValidUVE(rigidBody) || rigidBody.isKinematic ||
        rigidBody.mass <= 0.0F) {
        return false;
    }
    const float desiredNormalSpeed = std::min(
        policy.maximumSpeed, intoTarget * policy.strength / policy.deltaTimeSeconds);
    if (!std::isfinite(desiredNormalSpeed) || desiredNormalSpeed <= 0.0F) {
        return false;
    }
    const float currentNormalSpeed = Math::DotUVE(rigidBody.velocity, normal);
    if (!std::isfinite(currentNormalSpeed)) {
        return false;
    }
    const float speedDelta = desiredNormalSpeed - currentNormalSpeed;
    if (!std::isfinite(speedDelta) || speedDelta <= 0.0F) {
        return false;
    }
    const Math::Vector3UVE proposedVelocity = rigidBody.velocity + normal * speedDelta;
    if (!IsFiniteVectorUVE(proposedVelocity)) {
        return false;
    }
    rigidBody.velocity = proposedVelocity;
    return true;
}

struct ToICandidateUVE final {
    Scene::EntityUVE entity;
    Math::SweptAabbHitUVE hit;
};

[[nodiscard]] bool CollisionFilterAcceptsUVE(const Scene::ColliderComponentUVE& moving,
                                              std::uint32_t targetLayer,
                                              std::uint32_t targetMask) noexcept {
    return (moving.collisionMask & targetLayer) != 0U &&
           (targetMask & moving.collisionLayer) != 0U;
}

[[nodiscard]] std::optional<ToICandidateUVE> FindEarliestToIUVE(
    const Math::AabbUVE& movingAabb, const Math::Vector3UVE& remaining,
    const Scene::ColliderComponentUVE& movingCollider,
    const std::vector<Detail::ColliderWorldAabbUVE>& cache,
    Scene::EntityUVE movingEntity, std::size_t maximumTargets, bool& truncated) {
    truncated = cache.size() > maximumTargets;
    const std::size_t targetCount = std::min(cache.size(), maximumTargets);
    std::optional<ToICandidateUVE> best;
    for (std::size_t index = 0U; index < targetCount; ++index) {
        const Detail::ColliderWorldAabbUVE& candidate = cache[index];
        if (candidate.entity == movingEntity ||
            !CollisionFilterAcceptsUVE(movingCollider, candidate.collisionLayer, candidate.collisionMask)) {
            continue;
        }
        const std::optional<Math::SweptAabbHitUVE> hit =
            Math::SweepAabbUVE(movingAabb, remaining, candidate.worldAabb);
        if (!hit.has_value()) {
            continue;
        }
        const bool earlier = !best.has_value() || hit->time < best->hit.time - CharacterControllerUVE::kToIEpsilonUVE;
        const bool tied = best.has_value() &&
                          std::fabs(hit->time - best->hit.time) <= CharacterControllerUVE::kToIEpsilonUVE;
        if (earlier || (tied && candidate.entity.index < best->entity.index)) {
            best = ToICandidateUVE{candidate.entity, *hit};
        }
    }
    return best;
}

[[nodiscard]] bool HasControllerCollisionUVE(Scene::IEntityManagerUVE& entityManager,
                                              ICollisionSystemUVE& collisionSystem,
                                              Scene::EntityUVE controller) {
    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    return std::any_of(pairs.begin(), pairs.end(), [&](const CollisionPairUVE& pair) {
        return pair.first == controller || pair.second == controller;
    });
}

[[nodiscard]] bool IsCollisionFreeAfterDeltaUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    ICollisionSystemUVE& collisionSystem, Scene::EntityUVE controller,
    const Math::Vector3UVE& delta) {
    ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, delta);
    sceneGraph.UpdateUVE(entityManager);
    const bool collisionFree = !HasControllerCollisionUVE(entityManager, collisionSystem, controller);
    ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, -delta);
    sceneGraph.UpdateUVE(entityManager);
    return collisionFree;
}

[[nodiscard]] bool TryStepUpUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    ICollisionSystemUVE& collisionSystem, Scene::EntityUVE controller,
    const Math::Vector3UVE& horizontalDisplacement, float maximumStepHeight,
    bool& outContactsTruncated, Math::Vector3UVE& outNetDisplacement) {
    const Math::Vector3UVE lift{0.0F, maximumStepHeight, 0.0F};
    ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, lift);
    sceneGraph.UpdateUVE(entityManager);
    if (HasControllerCollisionUVE(entityManager, collisionSystem, controller)) {
        ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, -lift);
        sceneGraph.UpdateUVE(entityManager);
        return false;
    }

    const Scene::ColliderComponentUVE& movingCollider =
        entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(controller);
    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(controller);
    const Math::AabbUVE movingAabb = Math::AabbUVE::FromCenterExtentsUVE(
        worldTransform.worldPosition, Scene::GetColliderLocalHalfExtentsUVE(movingCollider));
    const std::vector<Detail::ColliderWorldAabbUVE> cache =
        Detail::BuildColliderWorldAabbCacheUVE(entityManager);
    bool targetCacheTruncated = false;
    const std::optional<ToICandidateUVE> horizontalImpact = FindEarliestToIUVE(
        movingAabb, horizontalDisplacement, movingCollider, cache, controller,
        CharacterControllerUVE::kMaximumToITargetsUVE, targetCacheTruncated);
    outContactsTruncated = outContactsTruncated || targetCacheTruncated;
    if (horizontalImpact.has_value()) {
        ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, -lift);
        sceneGraph.UpdateUVE(entityManager);
        return false;
    }

    ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, horizontalDisplacement);
    sceneGraph.UpdateUVE(entityManager);

    const Math::Vector3UVE fullDrop{0.0F, -maximumStepHeight, 0.0F};
    float dropDistance = maximumStepHeight;
    if (!IsCollisionFreeAfterDeltaUVE(entityManager, sceneGraph, collisionSystem, controller, fullDrop)) {
        float freeDistance = 0.0F;
        float blockedDistance = maximumStepHeight;
        for (std::size_t iteration = 0U; iteration < CharacterControllerUVE::kMaximumStepSearchIterationsUVE;
             ++iteration) {
            const float candidateDistance = (freeDistance + blockedDistance) * 0.5F;
            const Math::Vector3UVE candidateDrop{0.0F, -candidateDistance, 0.0F};
            if (IsCollisionFreeAfterDeltaUVE(entityManager, sceneGraph, collisionSystem, controller, candidateDrop)) {
                freeDistance = candidateDistance;
            } else {
                blockedDistance = candidateDistance;
            }
        }
        if (freeDistance <= CharacterControllerUVE::kMinimumMovementDistanceUVE) {
            ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, -horizontalDisplacement);
            ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, -lift);
            sceneGraph.UpdateUVE(entityManager);
            return false;
        }
        dropDistance = freeDistance;
    }

    const Math::Vector3UVE actualDrop{0.0F, -dropDistance, 0.0F};
    ApplyLocalDeltaUVE(entityManager, sceneGraph, controller, actualDrop);
    sceneGraph.UpdateUVE(entityManager);
    outNetDisplacement = lift + horizontalDisplacement + actualDrop;
    return true;
}

} // namespace

CharacterControllerMoveResultUVE CharacterControllerUVE::MoveUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input) {
    CharacterControllerMoveResultUVE result;
    result.requestedDisplacement = input.desiredDisplacement;
    result.remainingDisplacement = input.desiredDisplacement;

    if (!ValidateControllerInputUVE(entityManager, input, result)) {
        return result;
    }

    std::size_t maximumSubsteps = std::min(input.maximumSubsteps, kMaximumSubstepsUVE);
    if (input.maximumSubsteps > kMaximumSubstepsUVE) {
        result.inputClamped = true;
    }
    if (maximumSubsteps == 0U) {
        maximumSubsteps = 1U;
        result.inputClamped = true;
    }

    float maximumSubstepDistance = input.maximumSubstepDistance;
    if (!std::isfinite(maximumSubstepDistance) || maximumSubstepDistance <= 0.0F) {
        maximumSubstepDistance = kDefaultMaximumSubstepDistanceUVE;
        result.inputClamped = true;
    }
    const float minimumGroundNormalY = ResolveGroundSlopeCosineUVE(input, result);
    const DynamicBodyPushPolicyUVE pushPolicy = ResolveDynamicBodyPushPolicyUVE(input, result);

    sceneGraph.UpdateUVE(entityManager);
    while (result.substeps < maximumSubsteps &&
           FiniteLengthUVE(result.remainingDisplacement) > kMinimumMovementDistanceUVE) {
        const float remainingLength = FiniteLengthUVE(result.remainingDisplacement);
        const float stepLength = std::min(remainingLength, maximumSubstepDistance);
        const Math::Vector3UVE step = result.remainingDisplacement * (stepLength / remainingLength);
        ApplyLocalDeltaUVE(entityManager, sceneGraph, input.entity, step);
        result.remainingDisplacement -= step;
        result.appliedDisplacement += step;
        ++result.substeps;
        sceneGraph.UpdateUVE(entityManager);

        const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
        const std::size_t processCount = std::min(pairs.size(), kMaximumContactsUVE);
        result.contactsTruncated = result.contactsTruncated || pairs.size() > kMaximumContactsUVE;
        for (std::size_t index = 0U; index < processCount; ++index) {
            const CollisionPairUVE& pair = pairs[index];
            if (pair.first != input.entity && pair.second != input.entity) {
                continue;
            }
            if (!std::isfinite(pair.penetrationDepth) || pair.penetrationDepth <= 0.0F ||
                !IsFiniteVectorUVE(pair.separationAxis)) {
                continue;
            }
            const bool controllerIsFirst = pair.first == input.entity;
            const Scene::EntityUVE targetEntity = controllerIsFirst ? pair.second : pair.first;
            const Math::Vector3UVE contactNormal = controllerIsFirst
                ? pair.separationAxis : -pair.separationAxis;
            RegisterGroundContactUVE(result, contactNormal, minimumGroundNormalY);
            if (ApplyDynamicBodyPushUVE(entityManager, targetEntity, contactNormal, step, pushPolicy)) {
                ++result.pushedBodyCount;
            }
            const Math::Vector3UVE correction = controllerIsFirst
                ? -pair.separationAxis * pair.penetrationDepth
                : pair.separationAxis * pair.penetrationDepth;
            ApplyLocalDeltaUVE(entityManager, sceneGraph, input.entity, correction);
            result.appliedDisplacement += correction;
            result.remainingDisplacement = RemoveIntoNormalComponentUVE(
                result.remainingDisplacement, contactNormal);
            result.blocked = true;
            ++result.contactCount;
        }
        if (processCount > 0U) {
            sceneGraph.UpdateUVE(entityManager);
        }
    }

    result.code = CharacterControllerMoveCodeUVE::Moved;
    return result;
}

CharacterControllerMoveResultUVE CharacterControllerUVE::MoveWithToIUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input) {
    CharacterControllerMoveResultUVE result;
    result.requestedDisplacement = input.desiredDisplacement;
    result.remainingDisplacement = input.desiredDisplacement;
    if (!ValidateControllerInputUVE(entityManager, input, result)) {
        return result;
    }

    std::size_t maximumIterations = std::min(input.maximumSubsteps, kMaximumToIIterationsUVE);
    if (input.maximumSubsteps > kMaximumToIIterationsUVE) {
        result.inputClamped = true;
    }
    if (maximumIterations == 0U) {
        maximumIterations = 1U;
        result.inputClamped = true;
    }
    float maximumStepDistance = input.maximumSubstepDistance;
    if (!std::isfinite(maximumStepDistance) || maximumStepDistance <= 0.0F) {
        maximumStepDistance = kDefaultMaximumSubstepDistanceUVE;
        result.inputClamped = true;
    }
    const float minimumGroundNormalY = ResolveGroundSlopeCosineUVE(input, result);
    const DynamicBodyPushPolicyUVE pushPolicy = ResolveDynamicBodyPushPolicyUVE(input, result);
    float maximumStepHeight = input.maximumStepHeight;
    if (!std::isfinite(maximumStepHeight) || maximumStepHeight < 0.0F) {
        maximumStepHeight = 0.0F;
        result.inputClamped = true;
    }

    sceneGraph.UpdateUVE(entityManager);
    const std::vector<CollisionPairUVE> initialPairs = collisionSystem.DetectCollisionsUVE(entityManager);
    if (std::any_of(initialPairs.begin(), initialPairs.end(), [&](const CollisionPairUVE& pair) {
            return pair.first == input.entity || pair.second == input.entity;
        })) {
        return MoveUVE(entityManager, sceneGraph, collisionSystem, input);
    }

    while (result.substeps < maximumIterations &&
           FiniteLengthUVE(result.remainingDisplacement) > kMinimumMovementDistanceUVE) {
        const float remainingLength = FiniteLengthUVE(result.remainingDisplacement);
        const float consideredLength = std::min(remainingLength, maximumStepDistance);
        const Math::Vector3UVE consideredDisplacement =
            result.remainingDisplacement * (consideredLength / remainingLength);
        const Scene::ColliderComponentUVE& movingCollider =
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(input.entity);
        const Scene::WorldTransformComponentUVE& worldTransform =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(input.entity);
        const Math::AabbUVE movingAabb = Math::AabbUVE::FromCenterExtentsUVE(
            worldTransform.worldPosition, Scene::GetColliderLocalHalfExtentsUVE(movingCollider));
        const std::vector<Detail::ColliderWorldAabbUVE> cache =
            Detail::BuildColliderWorldAabbCacheUVE(entityManager);
        bool targetCacheTruncated = false;
        const std::optional<ToICandidateUVE> candidate = FindEarliestToIUVE(
            movingAabb, consideredDisplacement, movingCollider, cache, input.entity,
            kMaximumToITargetsUVE, targetCacheTruncated);
        result.contactsTruncated = result.contactsTruncated || targetCacheTruncated;

        if (!candidate.has_value()) {
            ApplyLocalDeltaUVE(entityManager, sceneGraph, input.entity, consideredDisplacement);
            result.remainingDisplacement -= consideredDisplacement;
            result.appliedDisplacement += consideredDisplacement;
            ++result.substeps;
            sceneGraph.UpdateUVE(entityManager);
            continue;
        }

        const float impactTime = std::clamp(candidate->hit.time, 0.0F, 1.0F);
        const Math::Vector3UVE horizontalDisplacement{
            consideredDisplacement.x, 0.0F, consideredDisplacement.z};
        if (maximumStepHeight > kMinimumMovementDistanceUVE &&
            std::fabs(candidate->hit.normal.y) < minimumGroundNormalY &&
            FiniteLengthUVE(horizontalDisplacement) > kMinimumMovementDistanceUVE) {
            Math::Vector3UVE stepDisplacement{};
            if (TryStepUpUVE(entityManager, sceneGraph, collisionSystem, input.entity,
                             horizontalDisplacement, maximumStepHeight, result.contactsTruncated,
                             stepDisplacement)) {
                result.remainingDisplacement -= horizontalDisplacement;
                result.appliedDisplacement += stepDisplacement;
                result.blocked = true;
                result.toiUsed = true;
                result.stepUpUsed = true;
                result.earliestImpactTime = std::min(result.earliestImpactTime, impactTime);
                ++result.contactCount;
                ++result.substeps;
                continue;
            }
        }

        const float advanceTime = std::max(0.0F, impactTime - kToIEpsilonUVE);
        const Math::Vector3UVE preImpactDisplacement = consideredDisplacement * advanceTime;
        ApplyLocalDeltaUVE(entityManager, sceneGraph, input.entity, preImpactDisplacement);
        result.remainingDisplacement -= preImpactDisplacement;
        result.appliedDisplacement += preImpactDisplacement;
        ++result.substeps;
        sceneGraph.UpdateUVE(entityManager);

        result.blocked = true;
        result.toiUsed = true;
        RegisterGroundContactUVE(result, candidate->hit.normal, minimumGroundNormalY);
        if (ApplyDynamicBodyPushUVE(entityManager, candidate->entity, candidate->hit.normal,
                                    consideredDisplacement, pushPolicy)) {
            ++result.pushedBodyCount;
        }
        result.earliestImpactTime = std::min(result.earliestImpactTime, impactTime);
        ++result.contactCount;
        result.remainingDisplacement = RemoveIntoNormalComponentUVE(
            result.remainingDisplacement, candidate->hit.normal);
    }

    result.code = CharacterControllerMoveCodeUVE::Moved;
    return result;
}

} // namespace UVE::Physics
