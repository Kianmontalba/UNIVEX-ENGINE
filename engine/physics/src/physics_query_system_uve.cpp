// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/physics_query_system_uve.h"

namespace UVE::Physics {

std::optional<SphereCastHitUVE> PhysicsQuerySystemUVE::SphereCastUVE(
    Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query) const {
    return ShapeCastSystemUVE::SphereCastUVE(entityManager, query);
}

std::optional<BoxCastHitUVE> PhysicsQuerySystemUVE::BoxCastUVE(
    Scene::IEntityManagerUVE& entityManager, const BoxCastQueryUVE& query) const {
    return ShapeCastSystemUVE::BoxCastUVE(entityManager, query);
}

std::optional<CapsuleCastHitUVE> PhysicsQuerySystemUVE::CapsuleCastUVE(
    Scene::IEntityManagerUVE& entityManager, const CapsuleCastQueryUVE& query) const {
    return ShapeCastSystemUVE::CapsuleCastUVE(entityManager, query);
}

AreaOverlapQueryResultUVE PhysicsQuerySystemUVE::QueryAreaOverlapsUVE(
    Scene::IEntityManagerUVE& entityManager, const std::size_t maximumResults) const {
    return AreaOverlapSystemUVE::QueryUVE(entityManager, maximumResults);
}

CharacterControllerMoveResultUVE PhysicsQuerySystemUVE::MoveCharacterUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    const CharacterControllerInputUVE& input) const {
    return CharacterControllerUVE::MoveUVE(entityManager, sceneGraph, *m_collisionSystem, input);
}

CharacterControllerMoveResultUVE PhysicsQuerySystemUVE::MoveCharacterWithToIUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    const CharacterControllerInputUVE& input) const {
    return CharacterControllerUVE::MoveWithToIUVE(entityManager, sceneGraph, *m_collisionSystem, input);
}

} // namespace UVE::Physics
