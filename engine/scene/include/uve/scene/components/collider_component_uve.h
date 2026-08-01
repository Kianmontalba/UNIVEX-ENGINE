//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Increment 15
/// (CollisionSystemUVE, Part 7.5) keeps the shape itself minimal — an axis-aligned box
/// half-extent; a real shape-type enum (box/sphere/capsule/mesh) is future work, not invented
/// ahead of a second shape actually existing. `collisionLayer` is consulted by RaycastSystemUVE
/// (Increment 16) via a query's layer mask; `collisionMask` remains reserved — present so a
/// future collider-vs-collider filtering feature never has to make a breaking change to this
/// component's serialized shape, but CollisionSystemUVE::DetectCollisionsUVE() does not yet
/// consult it. `friction`/`restitution` (Increment 16) are consumed by PhysicsSystemUVE's
/// collision resolution; `density` remains reserved (captured for spec completeness, not yet
/// wired to mass).
struct ColliderComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
    std::uint32_t collisionLayer = 1;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    float friction = 0.0F;
    float restitution = 0.0F;
    float density = 1.0F;
};

} // namespace UVE::Scene
