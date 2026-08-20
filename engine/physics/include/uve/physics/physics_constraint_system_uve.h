// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

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
    /// Number of deterministic connected constraint islands consumed by this solve.
    std::size_t islandCount = 0U;
    /// False only if the bounded copied island plan could not be built; no solver mutation follows.
    bool islandPlanValid = false;
    bool iterationCapReached = false;
};

struct ConstraintIslandEdgeUVE final {
    Scene::EntityUVE firstEntity;
    Scene::EntityUVE secondEntity;
};

struct ConstraintIslandPlanUVE final {
    static constexpr std::size_t kMaximumEntitiesUVE = 512U;
    std::array<Scene::EntityUVE, kMaximumEntitiesUVE> entities{};
    std::array<std::size_t, kMaximumEntitiesUVE> islandIndices{};
    std::size_t entityCount = 0U;
    std::size_t islandCount = 0U;
};

/// Builds deterministic connected components from copied constraint edges. The planner owns no
/// constraints, entities, solver iterations, persistence, or backend state.
[[nodiscard]] inline bool BuildConstraintIslandPlanUVE(
    const std::span<const ConstraintIslandEdgeUVE> edges, ConstraintIslandPlanUVE& outPlan) noexcept {
    if (edges.size() > ConstraintIslandPlanUVE::kMaximumEntitiesUVE) {
        return false;
    }
    ConstraintIslandPlanUVE plan;
    std::array<std::size_t, ConstraintIslandPlanUVE::kMaximumEntitiesUVE> parent{};
    for (std::size_t index = 0U; index < parent.size(); ++index) {
        parent[index] = index;
    }
    const auto addEntity = [&plan, &parent](const Scene::EntityUVE entity) noexcept {
        const std::size_t existing = [&plan, entity]() noexcept {
            for (std::size_t index = 0U; index < plan.entityCount; ++index) {
                if (plan.entities[index] == entity) {
                    return index;
                }
            }
            return ConstraintIslandPlanUVE::kMaximumEntitiesUVE;
        }();
        if (existing != ConstraintIslandPlanUVE::kMaximumEntitiesUVE) {
            return existing;
        }
        if (plan.entityCount >= ConstraintIslandPlanUVE::kMaximumEntitiesUVE) {
            return ConstraintIslandPlanUVE::kMaximumEntitiesUVE;
        }
        const std::size_t index = plan.entityCount++;
        plan.entities[index] = entity;
        parent[index] = index;
        return index;
    };
    const auto findRoot = [&parent](std::size_t index) noexcept {
        while (parent[index] != index) {
            parent[index] = parent[parent[index]];
            index = parent[index];
        }
        return index;
    };
    for (const ConstraintIslandEdgeUVE& edge : edges) {
        if (edge.firstEntity == Scene::kInvalidEntityUVE || edge.secondEntity == Scene::kInvalidEntityUVE) {
            return false;
        }
        const std::size_t first = addEntity(edge.firstEntity);
        const std::size_t second = addEntity(edge.secondEntity);
        if (first == ConstraintIslandPlanUVE::kMaximumEntitiesUVE ||
            second == ConstraintIslandPlanUVE::kMaximumEntitiesUVE) {
            return false;
        }
        const std::size_t firstRoot = findRoot(first);
        const std::size_t secondRoot = findRoot(second);
        if (firstRoot != secondRoot) {
            parent[secondRoot] = firstRoot;
        }
    }
    for (std::size_t index = 0U; index < plan.entityCount; ++index) {
        const std::size_t root = findRoot(index);
        std::size_t islandIndex = ConstraintIslandPlanUVE::kMaximumEntitiesUVE;
        for (std::size_t prior = 0U; prior < index; ++prior) {
            if (findRoot(prior) == root) {
                islandIndex = plan.islandIndices[prior];
                break;
            }
        }
        if (islandIndex == ConstraintIslandPlanUVE::kMaximumEntitiesUVE) {
            islandIndex = plan.islandCount++;
        }
        plan.islandIndices[index] = islandIndex;
    }
    outPlan = plan;
    return true;
}

/// Bounded main-thread positional constraint seam above the existing PhysicsSystemUVE.
/// Distance constraints preserve a configured anchor separation; hinge constraints preserve
/// coincident anchor positions and carry a validated world axis for future angular degrees of
/// freedom. v1 owns copied descriptors and generation-safe registry slots only: it does not own
/// entities, rotations, angular velocity, inertia, persistence, motors, limits, or backend state.
/// SolveUVE consumes the bounded deterministic connected-component plan to order work by island,
/// preserving slot order within each island and leaving the positional constraint equations unchanged.
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
