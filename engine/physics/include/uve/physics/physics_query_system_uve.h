// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_query_system_uve.h"

namespace UVE::Physics {

/// Engine-composed implementation of IPhysicsQuerySystemUVE. It is stateless apart from a
/// non-owning collision-system dependency used by CharacterControllerUVE movement commands.
class PhysicsQuerySystemUVE final : public IPhysicsQuerySystemUVE {
public:
    explicit PhysicsQuerySystemUVE(ICollisionSystemUVE& collisionSystem) noexcept
        : m_collisionSystem(&collisionSystem) {}

    [[nodiscard]] std::optional<SphereCastHitUVE> SphereCastUVE(
        Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query) const override;
    [[nodiscard]] std::optional<BoxCastHitUVE> BoxCastUVE(
        Scene::IEntityManagerUVE& entityManager, const BoxCastQueryUVE& query) const override;
    [[nodiscard]] std::optional<CapsuleCastHitUVE> CapsuleCastUVE(
        Scene::IEntityManagerUVE& entityManager, const CapsuleCastQueryUVE& query) const override;
    [[nodiscard]] AreaOverlapQueryResultUVE QueryAreaOverlapsUVE(
        Scene::IEntityManagerUVE& entityManager,
        std::size_t maximumResults = kMaximumAreaOverlapResultsUVE) const override;
    [[nodiscard]] CharacterControllerMoveResultUVE MoveCharacterUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        const CharacterControllerInputUVE& input) const override;
    [[nodiscard]] CharacterControllerMoveResultUVE MoveCharacterWithToIUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        const CharacterControllerInputUVE& input) const override;

private:
    ICollisionSystemUVE* m_collisionSystem;
};

} // namespace UVE::Physics
