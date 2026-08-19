// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include <cmath>

#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics::Detail {

std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(Scene::IEntityManagerUVE& entityManager) {
    std::vector<ColliderWorldAabbUVE> cache;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::ColliderComponentUVE>(
        [&](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::ColliderComponentUVE& collider) {
            const bool isSphere = collider.shapeType == Scene::ColliderShapeTypeUVE::Sphere;
            const bool isCapsule = collider.shapeType == Scene::ColliderShapeTypeUVE::Capsule;
            const Math::Vector3UVE localHalfExtents = Scene::GetColliderLocalHalfExtentsUVE(collider);
            Math::QuaternionUVE shapeRotation{};
            if (!Math::TryNormalizeUVE(worldTransform.worldRotation, shapeRotation)) {
                shapeRotation = {};
            }
            const float capsuleHalfSegment = isCapsule ? collider.height * 0.5F - collider.radius : 0.0F;
            const Math::Vector3UVE localCapsuleOffset{0.0F, capsuleHalfSegment, 0.0F};
            const Math::Vector3UVE worldCapsuleOffset =
                isCapsule ? Math::RotateVectorUVE(shapeRotation, localCapsuleOffset) : Math::Vector3UVE{};
            const Math::Vector3UVE capsuleSegmentStart = worldTransform.worldPosition - worldCapsuleOffset;
            const Math::Vector3UVE capsuleSegmentEnd = worldTransform.worldPosition + worldCapsuleOffset;
            const Math::Vector3UVE worldHalfExtents = isCapsule
                                                          ? Math::Vector3UVE{
                                                                collider.radius + std::fabs(worldCapsuleOffset.x),
                                                                collider.radius + std::fabs(worldCapsuleOffset.y),
                                                                collider.radius + std::fabs(worldCapsuleOffset.z),
                                                            }
                                                          : isSphere
                                                                ? localHalfExtents
                                                                : Math::Vector3UVE{
                                                                      std::fabs(Math::RotateVectorUVE(
                                                                          shapeRotation, {localHalfExtents.x, 0.0F, 0.0F})
                                                                            .x) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, localHalfExtents.y, 0.0F})
                                                                                .x) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, 0.0F, localHalfExtents.z})
                                                                                .x),
                                                                      std::fabs(Math::RotateVectorUVE(
                                                                          shapeRotation, {localHalfExtents.x, 0.0F, 0.0F})
                                                                            .y) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, localHalfExtents.y, 0.0F})
                                                                                .y) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, 0.0F, localHalfExtents.z})
                                                                                .y),
                                                                      std::fabs(Math::RotateVectorUVE(
                                                                          shapeRotation, {localHalfExtents.x, 0.0F, 0.0F})
                                                                            .z) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, localHalfExtents.y, 0.0F})
                                                                                .z) +
                                                                          std::fabs(Math::RotateVectorUVE(
                                                                              shapeRotation, {0.0F, 0.0F, localHalfExtents.z})
                                                                                .z),
                                                                  };
            cache.push_back(ColliderWorldAabbUVE{
                entity,
                Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, worldHalfExtents),
                collider.collisionLayer,
                collider.collisionMask,
                collider.shapeType,
                (isSphere || isCapsule) ? collider.radius : 0.0F,
                localHalfExtents,
                shapeRotation,
                capsuleSegmentStart,
                capsuleSegmentEnd,
            });
        });
    return cache;
}

} // namespace UVE::Physics::Detail
