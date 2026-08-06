// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_graph_uve.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "uve/debug/assert_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Scene {

namespace {

#if UVE_DEBUG
/// True iff `potentialAncestor` appears in `entity`'s parent chain (including `entity` itself).
/// Used by SetParentUVE() to reject reparenting that would create a cycle. Only referenced from
/// inside UVE_ASSERT, whose condition is never evaluated in Release builds (see
/// engine/debug/include/uve/debug/assert_uve.h) — guarded by #if UVE_DEBUG so it isn't left as
/// an unreferenced internal-linkage function in Release.
bool IsAncestorUVE(IEntityManagerUVE& entityManager, EntityUVE potentialAncestor, EntityUVE entity) {
    EntityUVE current = entity;
    while (current != kInvalidEntityUVE) {
        if (current == potentialAncestor) {
            return true;
        }
        current = entityManager.GetComponentUVE<HierarchyComponentUVE>(current).parent;
    }
    return false;
}
#endif

} // namespace

void SceneGraphUVE::AttachTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                        const TransformComponentUVE& localTransform) {
    UVE_ASSERT(entityManager.IsAliveUVE(entity));
    UVE_ASSERT(!entityManager.HasComponentUVE<TransformComponentUVE>(entity));

    entityManager.AddComponentUVE<TransformComponentUVE>(entity, localTransform);
    entityManager.AddComponentUVE<WorldTransformComponentUVE>(entity);
    entityManager.AddComponentUVE<HierarchyComponentUVE>(entity);
}

void SceneGraphUVE::SetLocalTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                         const TransformComponentUVE& localTransform) {
    UVE_ASSERT(entityManager.HasComponentUVE<TransformComponentUVE>(entity));

    entityManager.GetComponentUVE<TransformComponentUVE>(entity) = localTransform;
    entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity).dirty = true;
}

void SceneGraphUVE::SetParentUVE(IEntityManagerUVE& entityManager, EntityUVE child, EntityUVE newParent) {
    UVE_ASSERT(entityManager.HasComponentUVE<HierarchyComponentUVE>(child));
    UVE_ASSERT(newParent == kInvalidEntityUVE || entityManager.HasComponentUVE<HierarchyComponentUVE>(newParent));
#if UVE_DEBUG
    UVE_ASSERT(newParent == kInvalidEntityUVE || !IsAncestorUVE(entityManager, child, newParent));
#endif

    entityManager.GetComponentUVE<HierarchyComponentUVE>(child).parent = newParent;
    entityManager.GetComponentUVE<WorldTransformComponentUVE>(child).dirty = true;
}

void SceneGraphUVE::UpdateUVE(IEntityManagerUVE& entityManager) {
    std::vector<EntityUVE> pending;
    entityManager.ForEachUVE<HierarchyComponentUVE, TransformComponentUVE, WorldTransformComponentUVE>(
        [&pending](EntityUVE entity, HierarchyComponentUVE&, TransformComponentUVE&,
                   WorldTransformComponentUVE&) { pending.push_back(entity); });

    // Level-order sweep, root-first: repeatedly process any pending entity whose parent has
    // already been processed this pass (or is a root), tracking whether each processed entity's
    // world transform was actually recomputed (as opposed to merely visited) — a processed
    // parent's recomputation unconditionally forces every child to recompute too, even if the
    // child's own dirty flag is false. No persistent tree structure is needed:
    // HierarchyComponentUVE::parent is already the full source of truth, and SetParentUVE()
    // already prevents cycles.
    std::unordered_map<EntityUVE, bool> recomputedThisPass;

    bool madeProgress = true;
    while (madeProgress && !pending.empty()) {
        madeProgress = false;
        for (std::size_t index = 0; index < pending.size();) {
            const EntityUVE entity = pending[index];
            const EntityUVE parent = entityManager.GetComponentUVE<HierarchyComponentUVE>(entity).parent;
            const bool parentIsRoot = (parent == kInvalidEntityUVE);
            const auto parentIt = parentIsRoot ? recomputedThisPass.end() : recomputedThisPass.find(parent);
            const bool parentReady = parentIsRoot || parentIt != recomputedThisPass.end();

            if (!parentReady) {
                ++index;
                continue;
            }

            WorldTransformComponentUVE& world = entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity);
            const bool parentWasRecomputed = !parentIsRoot && parentIt->second;
            const bool shouldRecompute = world.dirty || parentWasRecomputed;

            if (shouldRecompute) {
                const TransformComponentUVE& local = entityManager.GetComponentUVE<TransformComponentUVE>(entity);
                if (parentIsRoot) {
                    world.worldPosition = local.localPosition;
                    world.worldRotation = local.localRotation;
                    world.worldScale = local.localScale;
                } else {
                    const WorldTransformComponentUVE& parentWorld =
                        entityManager.GetComponentUVE<WorldTransformComponentUVE>(parent);
                    world.worldScale = parentWorld.worldScale * local.localScale;
                    world.worldRotation = Math::MultiplyUVE(parentWorld.worldRotation, local.localRotation);
                    world.worldPosition =
                        parentWorld.worldPosition + Math::RotateVectorUVE(parentWorld.worldRotation,
                                                                            parentWorld.worldScale * local.localPosition);
                }
                world.dirty = false;
            }

            recomputedThisPass.emplace(entity, shouldRecompute);
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
