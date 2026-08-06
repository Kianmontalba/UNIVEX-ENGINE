// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3), extended in Part 7.5's
/// PhysicsSystemUVE (Increment 15) exactly as this component's own doc comment predicted:
/// velocity and drag. `velocity` is the one piece of state PhysicsSystemUVE needs to persist
/// across frames — storing it here (rather than inside PhysicsSystemUVE itself) is what lets
/// PhysicsSystemUVE stay a stateless service, matching CameraSystemUVE/MeshRendererUVE's
/// precedent. A collider-only entity (ColliderComponentUVE with no RigidBodyComponentUVE) is
/// static world geometry: detected and collided against, never moved.
struct RigidBodyComponentUVE final {
    float mass = 1.0F;
    bool isKinematic = false;
    Math::Vector3UVE velocity{};
    /// Simple linear damping: velocity *= (1 - drag * dt) each physics step. 0 = no damping.
    float drag = 0.0F;
    /// Multiplies PhysicsSystemUVE's gravity before integration — 1 = normal gravity, 0 =
    /// unaffected by gravity (but still collides/integrates other forces), > 1 = falls faster.
    float gravityScale = 1.0F;
};

} // namespace UVE::Scene
