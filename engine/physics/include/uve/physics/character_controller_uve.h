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
    float maximumGroundSlopeDegrees = 45.0F;
    /// Opt-in maximum vertical step height for MoveWithToIUVE; zero preserves the no-step behavior.
    float maximumStepHeight = 0.0F;
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
    bool toiUsed = false;
    bool stepUpUsed = false;
    bool grounded = false;
    Math::Vector3UVE groundNormal{};
    float earliestImpactTime = 1.0F;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == CharacterControllerMoveCodeUVE::Moved;
    }
};

/// Bounded main-thread kinematic movement over the existing AABB collision/layer contracts.
/// The default MoveUVE path advances in capped discrete substeps, then existing minimum-translation-
/// vector contacts are applied and remaining displacement is projected away from contacted normals.
/// The opt-in MoveWithToIUVE path adds capped conservative swept-AABB TOI and, when
/// `maximumStepHeight` is positive, one failure-atomic vertical lift/clearance/drop attempt for a
/// wall-like impact. Neither path owns exact non-box narrow phases, dynamic-body push policy,
/// locomotion state, or persistent controller history.
class CharacterControllerUVE final {
public:
    static constexpr std::size_t kMaximumSubstepsUVE = 32U;
    static constexpr std::size_t kMaximumContactsUVE = 64U;
    static constexpr float kMinimumMovementDistanceUVE = 1.0e-5F;
    static constexpr float kDefaultMaximumSubstepDistanceUVE = 0.25F;
    static constexpr float kDefaultMaximumGroundSlopeDegreesUVE = 45.0F;
    static constexpr float kMaximumGroundSlopeDegreesUVE = 89.0F;
    static constexpr std::size_t kMaximumToIIterationsUVE = 8U;
    static constexpr std::size_t kMaximumToITargetsUVE = 256U;
    static constexpr std::size_t kMaximumStepSearchIterationsUVE = 8U;
    static constexpr float kToIEpsilonUVE = 1.0e-4F;

    [[nodiscard]] static CharacterControllerMoveResultUVE MoveUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input);

    /// Opt-in bounded TOI-assisted movement. It scans the shared conservative world-AABB cache,
    /// advances to the earliest swept-AABB impact, removes the contacted normal from remaining
    /// displacement, and repeats for a capped number of iterations. It does not replace the
    /// overlap resolver or claim backend-wide CCD, exact non-box narrow phases, or dynamic-body
    /// continuous response.
    [[nodiscard]] static CharacterControllerMoveResultUVE MoveWithToIUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        ICollisionSystemUVE& collisionSystem, const CharacterControllerInputUVE& input);
};

} // namespace UVE::Physics
