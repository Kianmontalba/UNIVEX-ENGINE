// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/character_controller_uve.h"

#include <algorithm>
#include <cmath>

#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"

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

void ApplyLocalDeltaUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                        Scene::EntityUVE entity, const Math::Vector3UVE& delta) {
    Scene::TransformComponentUVE transform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    transform.localPosition += delta;
    sceneGraph.SetLocalTransformUVE(entityManager, entity, transform);
}

} // namespace

CharacterControllerMoveResultUVE CharacterControllerUVE::MoveUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input) {
    CharacterControllerMoveResultUVE result;
    result.requestedDisplacement = input.desiredDisplacement;
    result.remainingDisplacement = input.desiredDisplacement;

    if (!entityManager.IsAliveUVE(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::InvalidEntity;
        return result;
    }
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::MissingTransform;
        return result;
    }
    if (!entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(input.entity)) {
        result.code = CharacterControllerMoveCodeUVE::MissingCollider;
        return result;
    }
    if (!IsFiniteVectorUVE(input.desiredDisplacement) ||
        !Scene::IsColliderComponentValidUVE(
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(input.entity))) {
        result.code = CharacterControllerMoveCodeUVE::InvalidInput;
        return result;
    }
    if (entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(input.entity)) {
        const Scene::RigidBodyComponentUVE& rigidBody =
            entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(input.entity);
        if (!Scene::IsRigidBodyComponentValidUVE(rigidBody)) {
            result.code = CharacterControllerMoveCodeUVE::InvalidInput;
            return result;
        }
        if (!rigidBody.isKinematic) {
            result.code = CharacterControllerMoveCodeUVE::NonKinematicBody;
            return result;
        }
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
            const Math::Vector3UVE contactNormal = controllerIsFirst
                ? pair.separationAxis : -pair.separationAxis;
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

} // namespace UVE::Physics
