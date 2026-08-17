// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/physics_system_uve.h"

#include <algorithm>
#include <vector>

#include "uve/debug/assert_uve.h"
#include "uve/physics/collision_pair_uve.h"
#include "uve/physics/physics_material_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"

namespace UVE::Physics {

namespace {

/// 0 for a kinematic or non-positive-mass body — the "infinite mass" case (immovable). Computed
/// on demand rather than stored on RigidBodyComponentUVE, so mass/inverseMass can never desync.
[[nodiscard]] float EffectiveInverseMassUVE(bool isKinematic, float mass) noexcept {
    return (isKinematic || mass <= 0.0F) ? 0.0F : 1.0F / mass;
}

/// Moves `entity` by `positionDelta` (via SetLocalTransformUVE, so dirty-flag propagation stays
/// correct) and, if it has a non-kinematic RigidBodyComponentUVE, applies `material`'s combined
/// friction/restitution to the velocity component pointing toward `towardOtherBody`: the
/// tangential (sliding) component is damped by `1 - friction`, and the into-surface component is
/// reflected and scaled by `restitution` rather than simply zeroed. With `friction = 0,
/// restitution = 0` (every collider's default), this reduces to exactly `velocity -=
/// towardOtherBody * intoSurface` — bit-identical to Increment 15's pure vector-rejection formula
/// — never touches velocity that's already separating.
void MoveAndDeflectUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                       Scene::EntityUVE entity, Math::Vector3UVE positionDelta, Math::Vector3UVE towardOtherBody,
                       const PhysicsMaterialUVE& material) {
    Scene::TransformComponentUVE transform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    transform.localPosition += positionDelta;
    sceneGraph.SetLocalTransformUVE(entityManager, entity, transform);

    if (!entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(entity)) {
        return;
    }
    Scene::RigidBodyComponentUVE& rigidBody = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(entity);
    if (rigidBody.isKinematic) {
        return;
    }
    const float intoSurface = Math::DotUVE(rigidBody.velocity, towardOtherBody);
    if (intoSurface > 0.0F) {
        const Math::Vector3UVE normalVelocity = towardOtherBody * intoSurface;
        const Math::Vector3UVE tangentialVelocity = rigidBody.velocity - normalVelocity;
        const float frictionFactor = std::clamp(1.0F - material.friction, 0.0F, 1.0F);
        rigidBody.velocity = tangentialVelocity * frictionFactor - normalVelocity * material.restitution;
    }
}

/// Resolves one overlapping pair: mass-weighted positional correction (a kinematic or
/// collider-only-static side gets 0% of the correction; two dynamic bodies split proportionally
/// to inverse mass) plus per-body velocity deflection using both sides' combined material.
void ResolvePairUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                    const CollisionPairUVE& pair) {
    const auto InverseMassOfUVE = [&entityManager](Scene::EntityUVE entity) {
        if (!entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(entity)) {
            return 0.0F;
        }
        const Scene::RigidBodyComponentUVE& rigidBody =
            entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(entity);
        return EffectiveInverseMassUVE(rigidBody.isKinematic, rigidBody.mass);
    };

    const float firstInverseMass = InverseMassOfUVE(pair.first);
    const float secondInverseMass = InverseMassOfUVE(pair.second);
    const float totalInverseMass = firstInverseMass + secondInverseMass;
    if (totalInverseMass <= 0.0F) {
        return; // Both sides immovable (static/kinematic) — nothing to resolve.
    }

    const float firstShare = firstInverseMass / totalInverseMass;
    const float secondShare = secondInverseMass / totalInverseMass;

    // Both entities in `pair` are guaranteed to have ColliderComponentUVE — DetectCollisionsUVE
    // only ever returns pairs where both sides have one.
    const PhysicsMaterialUVE combinedMaterial = CombineMaterialsUVE(
        MaterialOfUVE(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(pair.first)),
        MaterialOfUVE(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(pair.second)));

    if (firstShare > 0.0F) {
        MoveAndDeflectUVE(entityManager, sceneGraph, pair.first,
                          -pair.separationAxis * (pair.penetrationDepth * firstShare), pair.separationAxis,
                          combinedMaterial);
    }
    if (secondShare > 0.0F) {
        MoveAndDeflectUVE(entityManager, sceneGraph, pair.second,
                          pair.separationAxis * (pair.penetrationDepth * secondShare), -pair.separationAxis,
                          combinedMaterial);
    }
}

} // namespace

PhysicsSystemUVE::PhysicsSystemUVE(ICollisionSystemUVE& collisionSystem, Math::Vector3UVE gravity)
    : m_collisionSystem(&collisionSystem), m_gravity(gravity) {}

void PhysicsSystemUVE::StepUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                                float fixedDeltaTimeSeconds) {
    entityManager.ForEachUVE<Scene::TransformComponentUVE, Scene::RigidBodyComponentUVE>(
        [&entityManager, &sceneGraph, this, fixedDeltaTimeSeconds](
            Scene::EntityUVE entity, const Scene::TransformComponentUVE& transform,
            Scene::RigidBodyComponentUVE& rigidBody) {
            UVE_ASSERT(Scene::IsRigidBodyComponentValidUVE(rigidBody));
            if (rigidBody.isKinematic) {
                return;
            }

            rigidBody.velocity += m_gravity * (rigidBody.gravityScale * fixedDeltaTimeSeconds);
            if (rigidBody.drag > 0.0F) {
                rigidBody.velocity *= std::max(0.0F, 1.0F - rigidBody.drag * fixedDeltaTimeSeconds);
            }

            Scene::TransformComponentUVE newTransform = transform;
            newTransform.localPosition += rigidBody.velocity * fixedDeltaTimeSeconds;
            sceneGraph.SetLocalTransformUVE(entityManager, entity, newTransform);
        });

    // SetLocalTransformUVE only marks WorldTransformComponentUVE dirty; propagate now so
    // DetectCollisionsUVE (which reads WorldTransformComponentUVE) sees this step's
    // post-integration positions, not last step's stale ones.
    sceneGraph.UpdateUVE(entityManager);

    const std::vector<CollisionPairUVE> pairs = m_collisionSystem->DetectCollisionsUVE(entityManager);
    for (const CollisionPairUVE& pair : pairs) {
        ResolvePairUVE(entityManager, sceneGraph, pair);
    }

    // Propagate resolution positions too, so a caller inspecting WorldTransformComponentUVE
    // immediately after StepUVE() (tests, or a second StepUVE() this same frame) sees fully
    // resolved state without needing an external UpdateUVE() call first.
    sceneGraph.UpdateUVE(entityManager);
}

} // namespace UVE::Physics
