// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/physics_constraint_system_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] float ConstraintLengthUVE(const Math::Vector3UVE& value) noexcept {
    const float squared = Math::LengthSquaredUVE(value);
    if (std::isinf(squared)) {
        return std::numeric_limits<float>::infinity();
    }
    return std::isfinite(squared) && squared >= 0.0F ? std::sqrt(squared) : 0.0F;
}

[[nodiscard]] Math::Vector3UVE NormalizeOrDefaultUVE(const Math::Vector3UVE& value,
                                                      Math::Vector3UVE fallback) noexcept {
    const float length = ConstraintLengthUVE(value);
    if (!std::isfinite(length) || length <= PhysicsConstraintSystemUVE::kConstraintEpsilonUVE) {
        return fallback;
    }
    return value * (1.0F / length);
}

void ApplyLocalDeltaUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                        Scene::EntityUVE entity, Math::Vector3UVE delta) {
    Scene::TransformComponentUVE transform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    transform.localPosition += delta;
    sceneGraph.SetLocalTransformUVE(entityManager, entity, transform);
}

[[nodiscard]] bool IsLocalDeltaValidUVE(Scene::IEntityManagerUVE& entityManager,
                                        Scene::EntityUVE entity,
                                        const Math::Vector3UVE delta) {
    if (!IsFiniteVectorUVE(delta)) {
        return false;
    }
    Scene::TransformComponentUVE candidate =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    candidate.localPosition += delta;
    return Scene::IsTransformComponentValidUVE(candidate);
}

[[nodiscard]] float InverseMassUVE(Scene::IEntityManagerUVE& entityManager,
                                   Scene::EntityUVE entity) noexcept {
    if (!entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(entity)) {
        return 0.0F;
    }
    const Scene::RigidBodyComponentUVE& rigidBody =
        entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(entity);
    if (!Scene::IsRigidBodyComponentValidUVE(rigidBody) || rigidBody.isKinematic || rigidBody.mass <= 0.0F) {
        return 0.0F;
    }
    return 1.0F / rigidBody.mass;
}

} // namespace

PhysicsConstraintMutationResultUVE PhysicsConstraintSystemUVE::AddDistanceConstraintUVE(
    const DistanceConstraintUVE& constraint) {
    return AddUVE(KindUVE::Distance, &constraint, nullptr);
}

PhysicsConstraintMutationResultUVE PhysicsConstraintSystemUVE::AddHingeConstraintUVE(
    const HingeConstraintUVE& constraint) {
    return AddUVE(KindUVE::Hinge, nullptr, &constraint);
}

PhysicsConstraintMutationResultUVE PhysicsConstraintSystemUVE::AddUVE(
    KindUVE kind, const DistanceConstraintUVE* distance, const HingeConstraintUVE* hinge) {
    if ((kind == KindUVE::Distance && (distance == nullptr || !IsValidDistanceUVE(*distance))) ||
        (kind == KindUVE::Hinge && (hinge == nullptr || !IsValidHingeUVE(*hinge)))) {
        return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::InvalidConstraint, {}};
    }
    for (std::size_t index = 0U; index < slots_.size(); ++index) {
        SlotUVE& slot = slots_[index];
        if (slot.occupied) {
            continue;
        }
        slot.kind = kind;
        if (kind == KindUVE::Distance) {
            slot.distance = *distance;
        } else {
            slot.hinge = *hinge;
        }
        slot.occupied = true;
        return PhysicsConstraintMutationResultUVE{
            PhysicsConstraintCodeUVE::Accepted,
            PhysicsConstraintHandleUVE{static_cast<std::uint32_t>(index), slot.generation}};
    }
    return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::CapacityExceeded, {}};
}

PhysicsConstraintMutationResultUVE PhysicsConstraintSystemUVE::RemoveConstraintUVE(
    PhysicsConstraintHandleUVE handle) noexcept {
    if (!handle.IsValidUVE() || handle.index >= slots_.size()) {
        return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::NotFound, handle};
    }
    SlotUVE& slot = slots_[handle.index];
    if (!slot.occupied) {
        return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::NotFound, handle};
    }
    if (slot.generation != handle.generation) {
        return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::StaleGeneration, handle};
    }
    slot.occupied = false;
    slot.generation = slot.generation == std::numeric_limits<std::uint32_t>::max() ? 1U : slot.generation + 1U;
    return PhysicsConstraintMutationResultUVE{PhysicsConstraintCodeUVE::Accepted, handle};
}

void PhysicsConstraintSystemUVE::ClearUVE() noexcept {
    for (SlotUVE& slot : slots_) {
        slot.occupied = false;
        slot.generation = slot.generation == std::numeric_limits<std::uint32_t>::max() ? 1U : slot.generation + 1U;
    }
}

std::size_t PhysicsConstraintSystemUVE::GetConstraintCountUVE() const noexcept {
    return static_cast<std::size_t>(std::count_if(slots_.begin(), slots_.end(), [](const SlotUVE& slot) {
        return slot.occupied;
    }));
}

bool PhysicsConstraintSystemUVE::IsValidDistanceUVE(const DistanceConstraintUVE& constraint) noexcept {
    return constraint.firstEntity != Scene::kInvalidEntityUVE &&
           constraint.secondEntity != Scene::kInvalidEntityUVE &&
           constraint.firstEntity != constraint.secondEntity && IsFiniteVectorUVE(constraint.firstAnchor) &&
           IsFiniteVectorUVE(constraint.secondAnchor) && std::isfinite(constraint.restLength) &&
           constraint.restLength >= 0.0F;
}

bool PhysicsConstraintSystemUVE::IsValidHingeUVE(const HingeConstraintUVE& constraint) noexcept {
    const float worldAxisLength = ConstraintLengthUVE(constraint.worldAxis);
    return constraint.firstEntity != Scene::kInvalidEntityUVE &&
           constraint.secondEntity != Scene::kInvalidEntityUVE &&
           constraint.firstEntity != constraint.secondEntity && IsFiniteVectorUVE(constraint.firstAnchor) &&
           IsFiniteVectorUVE(constraint.secondAnchor) && IsFiniteVectorUVE(constraint.worldAxis) &&
           std::isfinite(worldAxisLength) &&
           worldAxisLength > PhysicsConstraintSystemUVE::kConstraintEpsilonUVE;
}

PhysicsConstraintSolveResultUVE PhysicsConstraintSystemUVE::SolveUVE(
    Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
    std::size_t maximumIterations) const {
    PhysicsConstraintSolveResultUVE result;
    const std::size_t constraintCount = GetConstraintCountUVE();
    if (constraintCount == 0U) {
        result.islandPlanValid = true;
        return result;
    }

    std::array<ConstraintIslandEdgeUVE, kMaximumConstraintsUVE> edges{};
    std::size_t edgeCount = 0U;
    const auto getEntities = [](const SlotUVE& slot) noexcept {
        return std::pair<Scene::EntityUVE, Scene::EntityUVE>{
            slot.kind == KindUVE::Distance ? slot.distance.firstEntity : slot.hinge.firstEntity,
            slot.kind == KindUVE::Distance ? slot.distance.secondEntity : slot.hinge.secondEntity};
    };
    for (const SlotUVE& slot : slots_) {
        if (!slot.occupied) {
            continue;
        }
        const auto [firstEntity, secondEntity] = getEntities(slot);
        edges[edgeCount++] = ConstraintIslandEdgeUVE{firstEntity, secondEntity};
    }

    ConstraintIslandPlanUVE islandPlan;
    if (!BuildConstraintIslandPlanUVE(std::span<const ConstraintIslandEdgeUVE>{edges.data(), edgeCount}, islandPlan)) {
        return result;
    }
    result.islandPlanValid = true;
    result.islandCount = islandPlan.islandCount;

    const std::size_t iterationLimit = std::min(maximumIterations, kMaximumSolverIterationsUVE);
    if (iterationLimit == 0U) {
        return result;
    }

    std::array<std::size_t, kMaximumConstraintsUVE> orderedIndices{};
    std::size_t orderedCount = 0U;
    for (std::size_t island = 0U; island < islandPlan.islandCount; ++island) {
        for (std::size_t slotIndex = 0U; slotIndex < slots_.size(); ++slotIndex) {
            const SlotUVE& slot = slots_[slotIndex];
            if (!slot.occupied) {
                continue;
            }
            const auto [firstEntity, secondEntity] = getEntities(slot);
            std::size_t firstEntityIndex = ConstraintIslandPlanUVE::kMaximumEntitiesUVE;
            for (std::size_t entityIndex = 0U; entityIndex < islandPlan.entityCount; ++entityIndex) {
                if (islandPlan.entities[entityIndex] == firstEntity) {
                    firstEntityIndex = entityIndex;
                    break;
                }
            }
            if (firstEntityIndex != ConstraintIslandPlanUVE::kMaximumEntitiesUVE &&
                islandPlan.islandIndices[firstEntityIndex] == island) {
                orderedIndices[orderedCount++] = slotIndex;
            }
        }
    }

    std::array<bool, kMaximumConstraintsUVE> reportedSkipped{};
    sceneGraph.UpdateUVE(entityManager);
    for (std::size_t iteration = 0U; iteration < iterationLimit; ++iteration) {
        bool changed = false;
        for (std::size_t orderIndex = 0U; orderIndex < orderedCount; ++orderIndex) {
            const std::size_t index = orderedIndices[orderIndex];
            const SlotUVE& slot = slots_[index];
            if (!slot.occupied) {
                continue;
            }
            const Scene::EntityUVE firstEntity =
                slot.kind == KindUVE::Distance ? slot.distance.firstEntity : slot.hinge.firstEntity;
            const Scene::EntityUVE secondEntity =
                slot.kind == KindUVE::Distance ? slot.distance.secondEntity : slot.hinge.secondEntity;
            if (!entityManager.IsAliveUVE(firstEntity) || !entityManager.IsAliveUVE(secondEntity) ||
                !entityManager.HasComponentUVE<Scene::TransformComponentUVE>(firstEntity) ||
                !entityManager.HasComponentUVE<Scene::TransformComponentUVE>(secondEntity) ||
                !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(firstEntity) ||
                !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(secondEntity)) {
                if (!reportedSkipped[index]) {
                    ++result.skippedConstraintCount;
                    reportedSkipped[index] = true;
                }
                continue;
            }

            const Scene::WorldTransformComponentUVE& firstWorld =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(firstEntity);
            const Scene::WorldTransformComponentUVE& secondWorld =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(secondEntity);
            const Math::Vector3UVE firstAnchor = firstWorld.worldPosition +
                (slot.kind == KindUVE::Distance ? slot.distance.firstAnchor : slot.hinge.firstAnchor);
            const Math::Vector3UVE secondAnchor = secondWorld.worldPosition +
                (slot.kind == KindUVE::Distance ? slot.distance.secondAnchor : slot.hinge.secondAnchor);
            const Math::Vector3UVE delta = secondAnchor - firstAnchor;
            if (!IsFiniteVectorUVE(delta)) {
                if (!reportedSkipped[index]) {
                    ++result.skippedConstraintCount;
                    reportedSkipped[index] = true;
                }
                continue;
            }

            Math::Vector3UVE correction{};
            if (slot.kind == KindUVE::Distance) {
                const float distance = ConstraintLengthUVE(delta);
                const float error = distance - slot.distance.restLength;
                if (!std::isfinite(error)) {
                    if (!reportedSkipped[index]) {
                        ++result.skippedConstraintCount;
                        reportedSkipped[index] = true;
                    }
                    continue;
                }
                if (std::fabs(error) <= kConstraintEpsilonUVE) {
                    continue;
                }
                const Math::Vector3UVE direction =
                    NormalizeOrDefaultUVE(delta, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
                correction = direction * error;
            } else {
                if (ConstraintLengthUVE(delta) <= kConstraintEpsilonUVE) {
                    continue;
                }
                correction = delta;
            }

            const float firstInverseMass = InverseMassUVE(entityManager, firstEntity);
            const float secondInverseMass = InverseMassUVE(entityManager, secondEntity);
            const float totalInverseMass = firstInverseMass + secondInverseMass;
            if (!std::isfinite(firstInverseMass) || !std::isfinite(secondInverseMass) ||
                !std::isfinite(totalInverseMass)) {
                if (!reportedSkipped[index]) {
                    ++result.skippedConstraintCount;
                    reportedSkipped[index] = true;
                }
                continue;
            }
            if (totalInverseMass <= 0.0F) {
                continue;
            }
            const Math::Vector3UVE firstDelta = correction * (firstInverseMass / totalInverseMass);
            const Math::Vector3UVE secondDelta = correction * (-secondInverseMass / totalInverseMass);
            if (!IsLocalDeltaValidUVE(entityManager, firstEntity, firstDelta) ||
                !IsLocalDeltaValidUVE(entityManager, secondEntity, secondDelta)) {
                if (!reportedSkipped[index]) {
                    ++result.skippedConstraintCount;
                    reportedSkipped[index] = true;
                }
                continue;
            }
            ApplyLocalDeltaUVE(entityManager, sceneGraph, firstEntity, firstDelta);
            ApplyLocalDeltaUVE(entityManager, sceneGraph, secondEntity, secondDelta);
            ++result.solvedConstraintCount;
            changed = true;
            sceneGraph.UpdateUVE(entityManager);
        }
        result.iterations = iteration + 1U;
        if (!changed) {
            return result;
        }
    }
    result.iterationCapReached = true;
    return result;
}

} // namespace UVE::Physics
