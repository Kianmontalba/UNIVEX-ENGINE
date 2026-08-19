// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>

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
    /// Local-space angular velocity in radians/second. Zero preserves legacy orientation behavior.
    Math::Vector3UVE angularVelocity{};
    /// Caller-owned persistent torque in local-space force units; zero means no angular acceleration.
    Math::Vector3UVE torque{};
    /// Diagonal local-space inverse inertia. Zero components make the corresponding axis immovable.
    Math::Vector3UVE inverseInertia{};
    /// Simple linear damping: velocity *= (1 - drag * dt) each physics step. 0 = no damping.
    float drag = 0.0F;
    /// Multiplies PhysicsSystemUVE's gravity before integration — 1 = normal gravity, 0 =
    /// unaffected by gravity (but still collides/integrates other forces), > 1 = falls faster.
    float gravityScale = 1.0F;
};

/// Validates the value-only rigid-body contract before scene persistence and physics integration.
/// Zero mass remains valid and means an immovable body under the existing inverse-mass policy;
/// kinematic state is orthogonal to mass and remains an explicit authoring choice.
[[nodiscard]] inline bool IsRigidBodyComponentValidUVE(const RigidBodyComponentUVE& rigidBody) noexcept {
    return std::isfinite(rigidBody.mass) && rigidBody.mass >= 0.0F && std::isfinite(rigidBody.velocity.x) &&
           std::isfinite(rigidBody.velocity.y) && std::isfinite(rigidBody.velocity.z) &&
           std::isfinite(rigidBody.angularVelocity.x) && std::isfinite(rigidBody.angularVelocity.y) &&
           std::isfinite(rigidBody.angularVelocity.z) && std::isfinite(rigidBody.torque.x) &&
           std::isfinite(rigidBody.torque.y) && std::isfinite(rigidBody.torque.z) &&
           std::isfinite(rigidBody.inverseInertia.x) && rigidBody.inverseInertia.x >= 0.0F &&
           std::isfinite(rigidBody.inverseInertia.y) && rigidBody.inverseInertia.y >= 0.0F &&
           std::isfinite(rigidBody.inverseInertia.z) && rigidBody.inverseInertia.z >= 0.0F &&
           std::isfinite(rigidBody.drag) && rigidBody.drag >= 0.0F && std::isfinite(rigidBody.gravityScale) &&
           rigidBody.gravityScale >= 0.0F;
}

} // namespace UVE::Scene
