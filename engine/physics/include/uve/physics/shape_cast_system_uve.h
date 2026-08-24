// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>

#include "uve/math/ray_uve.h"
#include "uve/physics/physics_material_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene {
class IEntityManagerUVE;
}

namespace UVE::Physics {

/// Bounded sphere-cast query. Sphere targets use exact moving-sphere/sphere time-of-impact and
/// capsule targets use exact point-ray/capsule time-of-impact with the moving radius expanded into
/// the capsule; box and unknown targets remain conservatively expanded cached world AABBs. This is
/// read-only and does not claim exact oriented-box casts or backend replacement.
struct SphereCastQueryUVE final {
    Math::RayUVE ray{};
    float radius = 0.0F;
    float maxDistance = 0.0F;
    std::uint32_t layerMask = 0xFFFFFFFFU;
    Scene::EntityUVE ignoreEntity{};
};

struct BoxCastQueryUVE final {
    Math::RayUVE ray{};
    Math::Vector3UVE halfExtents{};
    float maxDistance = 0.0F;
    std::uint32_t layerMask = 0xFFFFFFFFU;
    Scene::EntityUVE ignoreEntity{};
};

struct CapsuleCastQueryUVE final {
    Math::RayUVE ray{};
    float radius = 0.0F;
    float height = 0.0F;
    float maxDistance = 0.0F;
    std::uint32_t layerMask = 0xFFFFFFFFU;
    Scene::EntityUVE ignoreEntity{};
};

struct CapsuleCastHitUVE final {
    Scene::EntityUVE entity;
    /// Center of the moving local-Y capsule at first entry into the conservative expanded AABB.
    Math::Vector3UVE center;
    /// Axis-aligned face normal of the expanded AABB; zero when the ray starts inside it.
    Math::Vector3UVE normal;
    float distance = 0.0F;
    PhysicsMaterialUVE material;
};

struct BoxCastHitUVE final {
    Scene::EntityUVE entity;
    /// Center of the moving axis-aligned box at first entry into the conservative expanded AABB.
    Math::Vector3UVE center;
    /// Axis-aligned face normal of the expanded AABB; zero when the ray starts inside it.
    Math::Vector3UVE normal;
    float distance = 0.0F;
    PhysicsMaterialUVE material;
};

struct SphereCastHitUVE final {
    Scene::EntityUVE entity;
    /// Center of the moving sphere at first contact with the exact sphere/capsule target or the
    /// conservative expanded AABB fallback.
    Math::Vector3UVE center;
    /// Exact target-surface normal for sphere/capsule targets; axis-aligned fallback normal for an
    /// expanded AABB, and zero when the ray starts inside the target.
    Math::Vector3UVE normal;
    float distance = 0.0F;
    PhysicsMaterialUVE material;
};

class ShapeCastSystemUVE final {
public:
    [[nodiscard]] static std::optional<SphereCastHitUVE> SphereCastUVE(
        Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query);

    /// Conservative axis-aligned box cast over cached world AABBs. The mover half-extents expand
    /// each target AABB; this is read-only and deliberately does not claim an oriented/exact box cast.
    [[nodiscard]] static std::optional<BoxCastHitUVE> BoxCastUVE(
        Scene::IEntityManagerUVE& entityManager, const BoxCastQueryUVE& query);

    /// Conservative axis-aligned local-Y capsule cast. The mover capsule's broad-phase half-extents
    /// expand each target AABB; this does not claim exact or rotated capsule geometry.
    [[nodiscard]] static std::optional<CapsuleCastHitUVE> CapsuleCastUVE(
        Scene::IEntityManagerUVE& entityManager, const CapsuleCastQueryUVE& query);
};

} // namespace UVE::Physics
