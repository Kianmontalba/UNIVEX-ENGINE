// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"

namespace UVE::Physics {
class PhysicsConstraintSystemUVE;
}

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

    /// Integrates finite rigid-body positions with widened intermediates before safe float-domain
    /// publication; invalid inputs and results preserve the prior state atomically.
    void StepUVE(Scene::IEntityManagerUVE& entityManager, Scene::ISceneGraphUVE& sceneGraph,
                 float fixedDeltaTimeSeconds) override;

    /// Attaches a caller-owned bounded positional constraint system. The solver is invoked after
    /// collision resolution; passing nullptr restores the legacy integration-only behavior.
    void SetConstraintSystemUVE(PhysicsConstraintSystemUVE* constraintSystem) noexcept;

private:
    ICollisionSystemUVE* m_collisionSystem;
    PhysicsConstraintSystemUVE* m_constraintSystem = nullptr;
    Math::Vector3UVE m_gravity;
};

} // namespace UVE::Physics
