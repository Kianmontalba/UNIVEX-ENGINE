//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"

namespace UVE::Physics {

/// PhysicsSystemUVE is the concrete, engine-standard implementation of IPhysicsSystemUVE.
/// Composes an ICollisionSystemUVE& (dependency injection, matching Renderer3DUVE's precedent
/// for a service that needs a constructor-injected collaborator) and holds one small piece of
/// genuinely-owned config (gravity) — no per-entity state, which is what all persists in
/// RigidBodyComponentUVE::velocity instead, so PhysicsSystemUVE stays trivially testable in
/// isolation despite not being fully stateless like CameraSystemUVE/MeshRendererUVE.
class PhysicsSystemUVE final : public IPhysicsSystemUVE {
public:
    /// `gravity` defaults to Earth-like {0, -9.81, 0} (Y-up, matching this engine's convention
    /// throughout). `collisionSystem` must outlive this PhysicsSystemUVE.
    explicit PhysicsSystemUVE(ICollisionSystemUVE& collisionSystem,
                               Math::Vector3UVE gravity = Math::Vector3UVE{0.0F, -9.81F, 0.0F});

    void StepUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                 float fixedDeltaTimeSeconds) override;

private:
    ICollisionSystemUVE* m_collisionSystem;
    Math::Vector3UVE m_gravity;
};

} // namespace UVE::Physics
