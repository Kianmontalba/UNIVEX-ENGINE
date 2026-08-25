// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <optional>

#include "uve/physics/area_overlap_system_uve.h"
#include "uve/physics/character_controller_uve.h"
#include "uve/physics/shape_cast_system_uve.h"

namespace UVE::Physics {

/// Read-only and caller-command physics query boundary above the existing bounded algorithms.
/// This interface owns no ECS, collider, controller, or backend state; implementations only route
/// calls to the existing shape-cast, overlap, and character-controller authorities.
/// Thread-safety follows IEntityManagerUVE and ISceneGraphUVE: calls are main-thread commands unless
/// the caller separately provides concurrent-read/mutation guarantees for those scene services.
class IPhysicsQuerySystemUVE {
public:
    virtual ~IPhysicsQuerySystemUVE() = default;

    [[nodiscard]] virtual std::optional<SphereCastHitUVE> SphereCastUVE(
        Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query) const = 0;
    [[nodiscard]] virtual std::optional<BoxCastHitUVE> BoxCastUVE(
        Scene::IEntityManagerUVE& entityManager, const BoxCastQueryUVE& query) const = 0;
    [[nodiscard]] virtual std::optional<CapsuleCastHitUVE> CapsuleCastUVE(
        Scene::IEntityManagerUVE& entityManager, const CapsuleCastQueryUVE& query) const = 0;
    [[nodiscard]] virtual AreaOverlapQueryResultUVE QueryAreaOverlapsUVE(
        Scene::IEntityManagerUVE& entityManager,
        std::size_t maximumResults = kMaximumAreaOverlapResultsUVE) const = 0;
    [[nodiscard]] virtual CharacterControllerMoveResultUVE MoveCharacterUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        const CharacterControllerInputUVE& input) const = 0;
    [[nodiscard]] virtual CharacterControllerMoveResultUVE MoveCharacterWithToIUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        const CharacterControllerInputUVE& input) const = 0;
};

} // namespace UVE::Physics
