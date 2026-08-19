// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/collision_system_uve.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/physics/detail/shape_narrow_phase_uve.h"

namespace UVE::Physics {
namespace {

[[nodiscard]] bool LayersAcceptPairUVE(const Detail::ColliderWorldAabbUVE& first,
                                       const Detail::ColliderWorldAabbUVE& second) noexcept {
    return (first.collisionMask & second.collisionLayer) != 0U &&
           (second.collisionMask & first.collisionLayer) != 0U;
}

[[nodiscard]] std::optional<Math::PenetrationUVE> ComputePairPenetrationUVE(
    const Detail::ColliderWorldAabbUVE& first, const Detail::ColliderWorldAabbUVE& second) {
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Sphere &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Sphere) {
        return Detail::ComputeSphereSpherePenetrationUVE(
            first.worldAabb.GetCenterUVE(), first.shapeRadius, second.worldAabb.GetCenterUVE(),
            second.shapeRadius);
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Sphere &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Box) {
        return Detail::ComputeSphereOrientedBoxPenetrationUVE(
            second.worldAabb.GetCenterUVE(), second.shapeHalfExtents, second.shapeRotation,
            first.worldAabb.GetCenterUVE(), first.shapeRadius);
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Box &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Sphere) {
        const std::optional<Math::PenetrationUVE> sphereToBox =
            Detail::ComputeSphereOrientedBoxPenetrationUVE(
                first.worldAabb.GetCenterUVE(), first.shapeHalfExtents, first.shapeRotation,
                second.worldAabb.GetCenterUVE(), second.shapeRadius);
        if (!sphereToBox.has_value()) {
            return std::nullopt;
        }
        return Math::PenetrationUVE{-sphereToBox->axis, sphereToBox->depth};
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Capsule &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Capsule) {
        return Detail::ComputeCapsuleCapsulePenetrationUVE(
            first.shapeSegmentStart, first.shapeSegmentEnd, first.shapeRadius,
            second.shapeSegmentStart, second.shapeSegmentEnd, second.shapeRadius);
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Capsule &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Sphere) {
        return Detail::ComputeCapsuleSpherePenetrationUVE(
            first.shapeSegmentStart, first.shapeSegmentEnd, first.shapeRadius,
            second.worldAabb.GetCenterUVE(), second.shapeRadius);
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Sphere &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Capsule) {
        const std::optional<Math::PenetrationUVE> capsuleToSphere =
            Detail::ComputeCapsuleSpherePenetrationUVE(
                second.shapeSegmentStart, second.shapeSegmentEnd, second.shapeRadius,
                first.worldAabb.GetCenterUVE(), first.shapeRadius);
        if (!capsuleToSphere.has_value()) {
            return std::nullopt;
        }
        return Math::PenetrationUVE{-capsuleToSphere->axis, capsuleToSphere->depth};
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Capsule &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Box) {
        return Detail::ComputeCapsuleOrientedBoxPenetrationUVE(
            second.worldAabb.GetCenterUVE(), second.shapeHalfExtents, second.shapeRotation,
            first.shapeSegmentStart, first.shapeSegmentEnd, first.shapeRadius);
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Box &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Capsule) {
        const std::optional<Math::PenetrationUVE> capsuleToBox =
            Detail::ComputeCapsuleOrientedBoxPenetrationUVE(
                first.worldAabb.GetCenterUVE(), first.shapeHalfExtents, first.shapeRotation,
                second.shapeSegmentStart, second.shapeSegmentEnd, second.shapeRadius);
        if (!capsuleToBox.has_value()) {
            return std::nullopt;
        }
        return Math::PenetrationUVE{-capsuleToBox->axis, capsuleToBox->depth};
    }
    if (first.shapeType == Scene::ColliderShapeTypeUVE::Box &&
        second.shapeType == Scene::ColliderShapeTypeUVE::Box) {
        return Detail::ComputeOrientedBoxOrientedBoxPenetrationUVE(
            first.worldAabb.GetCenterUVE(), first.shapeHalfExtents, first.shapeRotation,
            second.worldAabb.GetCenterUVE(), second.shapeHalfExtents, second.shapeRotation);
    }
    return Math::ComputePenetrationUVE(first.worldAabb, second.worldAabb);
}

void AppendPairIfOverlappingUVE(const std::vector<Detail::ColliderWorldAabbUVE>& colliders,
                                const std::size_t firstIndex, const std::size_t secondIndex,
                                std::vector<CollisionPairUVE>& pairs) {
    const Detail::ColliderWorldAabbUVE& first = colliders[firstIndex];
    const Detail::ColliderWorldAabbUVE& second = colliders[secondIndex];
    if (!LayersAcceptPairUVE(first, second)) {
        return;
    }

    const std::optional<Math::PenetrationUVE> penetration = ComputePairPenetrationUVE(first, second);
    if (penetration.has_value()) {
        pairs.push_back(CollisionPairUVE{first.entity, second.entity, penetration->axis, penetration->depth});
    }
}

} // namespace

std::vector<CollisionPairUVE> CollisionSystemUVE::DetectCollisionsUVE(Scene::IEntityManagerUVE& entityManager) const {
    Detail::DynamicAabbBvhUVE bvh(Detail::BuildColliderWorldAabbCacheUVE(entityManager));
    const std::vector<Detail::ColliderWorldAabbUVE>& colliders = bvh.GetCollidersUVE();

    std::vector<CollisionPairUVE> pairs;
    std::vector<std::size_t> candidates;
    for (std::size_t firstIndex = 0U; firstIndex < colliders.size(); ++firstIndex) {
        if (!bvh.QueryUVE(colliders[firstIndex].worldAabb, candidates)) {
            for (std::size_t secondIndex = firstIndex + 1U; secondIndex < colliders.size(); ++secondIndex) {
                AppendPairIfOverlappingUVE(colliders, firstIndex, secondIndex, pairs);
            }
            continue;
        }

        std::sort(candidates.begin(), candidates.end());
        for (const std::size_t secondIndex : candidates) {
            if (secondIndex > firstIndex) {
                AppendPairIfOverlappingUVE(colliders, firstIndex, secondIndex, pairs);
            }
        }
    }
    return pairs;
}

} // namespace UVE::Physics

// EOF
