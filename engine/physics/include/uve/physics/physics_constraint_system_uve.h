// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "uve/math/vector3_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_scene_graph_uve.h"

namespace UVE::Physics {

struct PhysicsConstraintHandleUVE final {
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0U;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return index != UINT32_MAX && generation != 0U;
    }

    [[nodiscard]] constexpr bool operator==(const PhysicsConstraintHandleUVE&) const noexcept = default;
};

enum class PhysicsConstraintCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidConstraint,
    CapacityExceeded,
    NotFound,
    StaleGeneration,
};

struct DistanceConstraintUVE final {
    Scene::EntityUVE firstEntity;
    Scene::EntityUVE secondEntity;
    Math::Vector3UVE firstAnchor{};
    Math::Vector3UVE secondAnchor{};
    float restLength = 0.0F;
};

struct HingeConstraintUVE final {
    Scene::EntityUVE firstEntity;
    Scene::EntityUVE secondEntity;
    Math::Vector3UVE firstAnchor{};
    Math::Vector3UVE secondAnchor{};
    Math::Vector3UVE worldAxis{0.0F, 1.0F, 0.0F};
};

struct PhysicsConstraintMutationResultUVE final {
    PhysicsConstraintCodeUVE code = PhysicsConstraintCodeUVE::InvalidConstraint;
    PhysicsConstraintHandleUVE handle;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == PhysicsConstraintCodeUVE::Accepted;
    }
};

struct PhysicsConstraintSolveResultUVE final {
    std::size_t iterations = 0U;
    std::size_t solvedConstraintCount = 0U;
    std::size_t skippedConstraintCount = 0U;
    bool iterationCapReached = false;
};

/// Bounded main-thread positional constraint seam above the existing PhysicsSystemUVE.
/// Distance constraints preserve a configured anchor separation; hinge constraints preserve
/// coincident anchor positions and carry a validated world axis for future angular degrees of
/// freedom. v1 owns copied descriptors and generation-safe registry slots only: it does not own
/// entities, rotations, angular velocity, inertia, persistence, motors, limits, or backend state.
class PhysicsConstraintSystemUVE final {
public:
    static constexpr std::size_t kMaximumConstraintsUVE = 256U;
    static constexpr std::size_t kMaximumSolverIterationsUVE = 8U;
    static constexpr float kConstraintEpsilonUVE = 1.0e-4F;

    [[nodiscard]] PhysicsConstraintMutationResultUVE AddDistanceConstraintUVE(
        const DistanceConstraintUVE& constraint);
    [[nodiscard]] PhysicsConstraintMutationResultUVE AddHingeConstraintUVE(
        const HingeConstraintUVE& constraint);
    [[nodiscard]] PhysicsConstraintMutationResultUVE RemoveConstraintUVE(
        PhysicsConstraintHandleUVE handle) noexcept;
    void ClearUVE() noexcept;

    [[nodiscard]] std::size_t GetConstraintCountUVE() const noexcept;

    [[nodiscard]] PhysicsConstraintSolveResultUVE SolveUVE(
        Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
        std::size_t maximumIterations = kMaximumSolverIterationsUVE) const;

private:
    enum class KindUVE : std::uint8_t { Distance = 0, Hinge };

    struct SlotUVE final {
        KindUVE kind = KindUVE::Distance;
        DistanceConstraintUVE distance;
        HingeConstraintUVE hinge;
        std::uint32_t generation = 1U;
        bool occupied = false;
    };

    [[nodiscard]] PhysicsConstraintMutationResultUVE AddUVE(
        KindUVE kind, const DistanceConstraintUVE* distance, const HingeConstraintUVE* hinge);
    [[nodiscard]] static bool IsValidDistanceUVE(const DistanceConstraintUVE& constraint) noexcept;
    [[nodiscard]] static bool IsValidHingeUVE(const HingeConstraintUVE& constraint) noexcept;

    std::array<SlotUVE, kMaximumConstraintsUVE> slots_{};
};

} // namespace UVE::Physics
