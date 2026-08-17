// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>

#include "uve/math/vector3_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_scene_graph_uve.h"

namespace UVE::Physics {

/// Caller-owned kinematic movement request. The entity must have a valid transform, collider,
/// and either no rigid body or a valid kinematic rigid body; CharacterControllerUVE never adds,
/// removes, or serializes components.
struct CharacterControllerInputUVE final {
    Scene::EntityUVE entity;
    Math::Vector3UVE desiredDisplacement{};
    std::size_t maximumSubsteps = 8U;
    float maximumSubstepDistance = 0.25F;
};

enum class CharacterControllerMoveCodeUVE : std::uint8_t {
    Moved = 0,
    InvalidEntity,
    MissingTransform,
    MissingCollider,
    NonKinematicBody,
    InvalidInput,
};

struct CharacterControllerMoveResultUVE final {
    CharacterControllerMoveCodeUVE code = CharacterControllerMoveCodeUVE::InvalidInput;
    Math::Vector3UVE requestedDisplacement{};
    Math::Vector3UVE appliedDisplacement{};
    Math::Vector3UVE remainingDisplacement{};
    std::size_t substeps = 0U;
    std::size_t contactCount = 0U;
    bool blocked = false;
    bool inputClamped = false;
    bool contactsTruncated = false;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == CharacterControllerMoveCodeUVE::Moved;
    }
};

/// Bounded main-thread kinematic movement over the existing AABB collision/layer contracts.
/// Movement is advanced in capped discrete substeps, then existing minimum-translation-vector
/// contacts are applied to the controller and the remaining displacement is projected away from
/// contacted normals. This v1 seam is deliberately not continuous collision detection, a shape
/// sweep, a dynamic-body push policy, a ground/step solver, or a character locomotion state owner.
class CharacterControllerUVE final {
public:
    static constexpr std::size_t kMaximumSubstepsUVE = 32U;
    static constexpr std::size_t kMaximumContactsUVE = 64U;
    static constexpr float kMinimumMovementDistanceUVE = 1.0e-5F;
    static constexpr float kDefaultMaximumSubstepDistanceUVE = 0.25F;

    [[nodiscard]] static CharacterControllerMoveResultUVE MoveUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input);
};

} // namespace UVE::Physics
