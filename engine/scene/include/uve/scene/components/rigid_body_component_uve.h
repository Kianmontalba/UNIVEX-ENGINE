//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data — mass and the kinematic/dynamic flag are the two universally-understood
/// basics of a rigid body. Will be extended once PhysicsSystemUVE (Part 7.5) exists to consume
/// it (velocity, drag, constraints, etc.).
struct RigidBodyComponentUVE final {
    float mass = 1.0F;
    bool isKinematic = false;
};

} // namespace UVE::Scene
