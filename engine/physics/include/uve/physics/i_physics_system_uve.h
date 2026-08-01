//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_scene_graph_uve.h"

namespace UVE::Physics {

/// IPhysicsSystemUVE is the spec's `PhysicsSystemUVE` (Part 7.5): dynamics integration +
/// collision resolution, driven once per fixed simulation step by `EngineCoreUVE::Update()`
/// (via `Utilities::FixedStepResultUVE`, already computed every frame but previously unconsumed).
/// A from-scratch CPU implementation this increment (see docs/CODING_STANDARDS.md and the
/// Increment 15 plan for why a real Jolt/Bullet backend is deferred) — deliberately interfaced
/// so a future backend-wrapping implementation can replace `PhysicsSystemUVE` without any ECS or
/// call-site change, the same seam `IRenderDeviceUVE` provides for graphics.
/// Thread-safety: not thread-safe — `StepUVE()` must be called only from the main engine thread,
/// once per fixed step, matching `SceneGraphUVE::UpdateUVE()`'s contract (and must run before it
/// each frame, so world transforms this step just wrote are picked up by the same frame's
/// propagation pass).
class IPhysicsSystemUVE {
public:
    virtual ~IPhysicsSystemUVE() = default;

    /// Runs exactly one fixed-size simulation step: integrates every non-kinematic
    /// `RigidBodyComponentUVE` (gravity * gravityScale, semi-implicit Euler, linear drag),
    /// calls `sceneGraph.UpdateUVE()` so `WorldTransformComponentUVE` reflects this step's new
    /// positions, detects collisions among every `ColliderComponentUVE` entity, resolves
    /// overlaps with mass-weighted positional correction and into-surface-only velocity removal
    /// (preserves sliding), then calls `sceneGraph.UpdateUVE()` again so the resolved positions
    /// are immediately visible too. Every position write goes through
    /// `ISceneGraphUVE::SetLocalTransformUVE()` — never `GetComponentUVE()` directly, so
    /// dirty-flag propagation stays correct throughout.
    virtual void StepUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                          float fixedDeltaTimeSeconds) = 0;
};

} // namespace UVE::Physics
