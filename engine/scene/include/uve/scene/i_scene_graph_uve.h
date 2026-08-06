// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <vector>

#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Scene {

/// ISceneGraphUVE is the hierarchical-transform-tree interface: parent/child relationships and
/// dirty-flag-propagated world-space transforms, built on top of IEntityManagerUVE's
/// TransformComponentUVE/HierarchyComponentUVE/WorldTransformComponentUVE rather than a
/// separate tree structure (see docs — "Node3D is a thin handle over ECS data"). Stateless:
/// every method takes the IEntityManagerUVE it operates on explicitly, so a conforming
/// implementation has no ordering dependency on any specific manager instance's construction.
/// Thread-safety: not thread-safe — every method must be called only from the main/scene
/// thread, matching IEntityManagerUVE's own contract.
class ISceneGraphUVE {
public:
    virtual ~ISceneGraphUVE() = default;

    /// Makes `entity` a scene-graph node: adds TransformComponentUVE (set to `localTransform`),
    /// WorldTransformComponentUVE (dirty), and HierarchyComponentUVE (parent = invalid) in one
    /// call, so callers never have to remember all three individually. Asserts `entity` is
    /// alive and does not already have a TransformComponentUVE.
    virtual void AttachTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                     const TransformComponentUVE& localTransform) = 0;

    /// Overwrites `entity`'s authored local transform and marks its WorldTransformComponentUVE
    /// dirty. Asserts `entity` already has a TransformComponentUVE (see AttachTransformUVE()).
    virtual void SetLocalTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                                       const TransformComponentUVE& localTransform) = 0;

    /// Reparents `child` under `newParent` (or makes it a root if `newParent ==
    /// kInvalidEntityUVE`), marking `child` dirty. Asserts neither entity would create a cycle
    /// (i.e. `child` is not currently an ancestor of `newParent`) — UVE_ASSERT fires rather than
    /// silently corrupting the hierarchy.
    virtual void SetParentUVE(IEntityManagerUVE& entityManager, EntityUVE child, EntityUVE newParent) = 0;

    /// The dirty-flag-propagation pass: recomputes every scene-graph entity's
    /// WorldTransformComponentUVE from its parent chain, root-first. A processed parent's
    /// recomputation unconditionally forces every direct child to recompute too, even if the
    /// child's own dirty flag was false — the correct semantics for propagation, not just
    /// per-entity dirty tracking. Called once per frame by EngineCoreUVE::Update().
    virtual void UpdateUVE(IEntityManagerUVE& entityManager) = 0;

    /// Returns every entity whose HierarchyComponentUVE::parent equals `parent`. O(n) over
    /// every scene-graph entity — acceptable per the coding standards' "profile before
    /// optimizing" rule; a cached parent→children index is a documented future optimization.
    [[nodiscard]] virtual std::vector<EntityUVE> GetChildrenUVE(IEntityManagerUVE& entityManager,
                                                                 EntityUVE parent) = 0;
};

} // namespace UVE::Scene
