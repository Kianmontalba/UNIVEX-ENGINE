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

/// Conservative sphere-cast query. The moving sphere is swept along a unit-direction ray; each
/// target collider is expanded by the radius in world axes and tested as its cached world AABB.
/// This is a bounded read-only query, not an exact oriented sphere cast or backend replacement.
struct SphereCastQueryUVE final {
    Math::RayUVE ray{};
    float radius = 0.0F;
    float maxDistance = 0.0F;
    std::uint32_t layerMask = 0xFFFFFFFFU;
    Scene::EntityUVE ignoreEntity{};
};

struct SphereCastHitUVE final {
    Scene::EntityUVE entity;
    /// Center of the moving sphere at first entry into the conservative expanded AABB.
    Math::Vector3UVE center;
    /// Axis-aligned face normal of the expanded AABB; zero when the ray starts inside it.
    Math::Vector3UVE normal;
    float distance = 0.0F;
    PhysicsMaterialUVE material;
};

class ShapeCastSystemUVE final {
public:
    [[nodiscard]] static std::optional<SphereCastHitUVE> SphereCastUVE(
        Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query);
};

} // namespace UVE::Physics
