// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_graph_uve.h"

#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "uve/debug/assert_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Scene {

namespace {

/// True iff `potentialAncestor` appears in `entity`'s parent chain, or the chain is malformed.
/// The visited set prevents a pre-existing malformed cycle from making this public mutation path
/// loop forever in a release build.
bool IsAncestorOrMalformedUVE(IEntityManagerUVE& entityManager, EntityUVE potentialAncestor,
                              EntityUVE entity) {
    std::unordered_set<EntityUVE> visited;
    EntityUVE current = entity;
    while (current != kInvalidEntityUVE) {
        if (!entityManager.IsAliveUVE(current) ||
            !entityManager.HasComponentUVE<HierarchyComponentUVE>(current) ||
            !visited.insert(current).second) {
            return true;
        }
        if (current == potentialAncestor) {
            return true;
        }
        current = entityManager.GetComponentUVE<HierarchyComponentUVE>(current).parent;
    }
    return false;
}

[[nodiscard]] bool IsFiniteWorldTransformUVE(const WorldTransformComponentUVE& transform) noexcept {
    return std::isfinite(transform.worldPosition.x) && std::isfinite(transform.worldPosition.y) &&
           std::isfinite(transform.worldPosition.z) && Math::IsFiniteUVE(transform.worldRotation) &&
           std::isfinite(transform.worldScale.x) && std::isfinite(transform.worldScale.y) &&
           std::isfinite(transform.worldScale.z);
}

struct WorldTransformPassStateUVE final {
    bool valid = false;
    bool recomputed = false;
};

} // namespace

void SceneGraphUVE::AttachTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                        const TransformComponentUVE& localTransform) {
    if (!entityManager.IsAliveUVE(entity) || entityManager.HasComponentUVE<TransformComponentUVE>(entity) ||
        entityManager.HasComponentUVE<WorldTransformComponentUVE>(entity) ||
        entityManager.HasComponentUVE<HierarchyComponentUVE>(entity) ||
        !IsTransformComponentValidUVE(localTransform)) {
        UVE_ASSERT(entityManager.IsAliveUVE(entity));
        UVE_ASSERT(!entityManager.HasComponentUVE<TransformComponentUVE>(entity));
        UVE_ASSERT(IsTransformComponentValidUVE(localTransform));
        return;
    }

    entityManager.AddComponentUVE<TransformComponentUVE>(entity, localTransform);
    entityManager.AddComponentUVE<WorldTransformComponentUVE>(entity);
    entityManager.AddComponentUVE<HierarchyComponentUVE>(entity);
}

void SceneGraphUVE::SetLocalTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                         const TransformComponentUVE& localTransform) {
    if (!entityManager.IsAliveUVE(entity) || !entityManager.HasComponentUVE<TransformComponentUVE>(entity) ||
        !entityManager.HasComponentUVE<WorldTransformComponentUVE>(entity) ||
        !IsTransformComponentValidUVE(localTransform)) {
        UVE_ASSERT(entityManager.HasComponentUVE<TransformComponentUVE>(entity));
        UVE_ASSERT(IsTransformComponentValidUVE(localTransform));
        return;
    }

    entityManager.GetComponentUVE<TransformComponentUVE>(entity) = localTransform;
    entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity).dirty = true;
}

void SceneGraphUVE::SetParentUVE(IEntityManagerUVE& entityManager, EntityUVE child, EntityUVE newParent) {
    const bool invalidChild = !entityManager.IsAliveUVE(child) ||
                              !entityManager.HasComponentUVE<HierarchyComponentUVE>(child) ||
                              !entityManager.HasComponentUVE<WorldTransformComponentUVE>(child);
    const bool invalidParent =
        newParent != kInvalidEntityUVE &&
        (!entityManager.IsAliveUVE(newParent) ||
         !entityManager.HasComponentUVE<HierarchyComponentUVE>(newParent));
    const bool createsCycle = !invalidChild && !invalidParent && newParent != kInvalidEntityUVE &&
                              IsAncestorOrMalformedUVE(entityManager, child, newParent);
    if (invalidChild || invalidParent || createsCycle) {
        UVE_ASSERT(!invalidChild);
        UVE_ASSERT(!invalidParent);
        UVE_ASSERT(!createsCycle);
        return;
    }

    entityManager.GetComponentUVE<HierarchyComponentUVE>(child).parent = newParent;
    entityManager.GetComponentUVE<WorldTransformComponentUVE>(child).dirty = true;
}

void SceneGraphUVE::UpdateUVE(IEntityManagerUVE& entityManager) {
    std::vector<EntityUVE> pending;
    entityManager.ForEachUVE<HierarchyComponentUVE, TransformComponentUVE, WorldTransformComponentUVE>(
        [&pending](EntityUVE entity, HierarchyComponentUVE&, TransformComponentUVE&,
                   WorldTransformComponentUVE&) { pending.push_back(entity); });

    // Level-order sweep, root-first: repeatedly process any pending entity whose parent has
    // already been processed this pass (or is a root), tracking valid/invalid derived state and
    // whether each valid processed entity's world transform was actually recomputed (as opposed
    // to merely visited) — a processed parent's recomputation unconditionally forces every child
    // to recompute too, even if the
    // child's own dirty flag is false. No persistent tree structure is needed:
    // HierarchyComponentUVE::parent is already the full source of truth, and SetParentUVE()
    // already prevents cycles.
    std::unordered_map<EntityUVE, WorldTransformPassStateUVE> passState;

    bool madeProgress = true;
    while (madeProgress && !pending.empty()) {
        madeProgress = false;
        for (std::size_t index = 0; index < pending.size();) {
            const EntityUVE entity = pending[index];
            const EntityUVE parent = entityManager.GetComponentUVE<HierarchyComponentUVE>(entity).parent;
            const bool parentIsRoot = (parent == kInvalidEntityUVE);
            const auto parentIt = parentIsRoot ? passState.end() : passState.find(parent);
            const bool parentReady = parentIsRoot || parentIt != passState.end();

            if (!parentReady) {
                ++index;
                continue;
            }

            WorldTransformComponentUVE& world = entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity);
            if (!parentIsRoot && !parentIt->second.valid) {
                world.dirty = true;
                passState.emplace(entity, WorldTransformPassStateUVE{});
                pending.erase(pending.begin() + static_cast<std::ptrdiff_t>(index));
                madeProgress = true;
                continue;
            }

            const bool parentWasRecomputed = !parentIsRoot && parentIt->second.recomputed;
            const bool shouldRecompute = world.dirty || parentWasRecomputed;
            bool publishedValid = IsFiniteWorldTransformUVE(world);
            if (shouldRecompute) {
                const TransformComponentUVE& local = entityManager.GetComponentUVE<TransformComponentUVE>(entity);
                WorldTransformComponentUVE candidate = world;
                if (parentIsRoot) {
                    candidate.worldPosition = local.localPosition;
                    candidate.worldRotation = local.localRotation;
                    candidate.worldScale = local.localScale;
                } else {
                    const WorldTransformComponentUVE& parentWorld =
                        entityManager.GetComponentUVE<WorldTransformComponentUVE>(parent);
                    candidate.worldScale = parentWorld.worldScale * local.localScale;
                    candidate.worldRotation = Math::MultiplyUVE(parentWorld.worldRotation, local.localRotation);
                    candidate.worldPosition =
                        parentWorld.worldPosition + Math::RotateVectorUVE(parentWorld.worldRotation,
                                                                            parentWorld.worldScale * local.localPosition);
                }
                publishedValid = IsFiniteWorldTransformUVE(candidate);
                if (publishedValid) {
                    candidate.dirty = false;
                    world = candidate;
                } else {
                    world.dirty = true;
                }
            }

            passState.emplace(entity, WorldTransformPassStateUVE{publishedValid, shouldRecompute && publishedValid});
            pending.erase(pending.begin() + static_cast<std::ptrdiff_t>(index));
            madeProgress = true;
        }
    }

    // A non-empty remainder here means a cycle slipped past SetParentUVE()'s guard — a genuine
    // engine bug, not user error, worth catching in debug builds.
    UVE_ASSERT(pending.empty());
}

std::vector<EntityUVE> SceneGraphUVE::GetChildrenUVE(IEntityManagerUVE& entityManager, EntityUVE parent) {
    std::vector<EntityUVE> children;
    entityManager.ForEachUVE<HierarchyComponentUVE>(
        [&children, parent](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == parent) {
                children.push_back(entity);
            }
        });
    return children;
}

} // namespace UVE::Scene
