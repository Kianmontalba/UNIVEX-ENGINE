// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/physics/physics_material_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Physics {

/// The closest hit reported by IRaycastSystemUVE::RaycastUVE(). Deliberately a plain value type,
/// matching CollisionPairUVE's precedent. `material` is a convenience snapshot of the hit
/// collider's friction/restitution/density (already clamped via the same helper
/// PhysicsSystemUVE's resolution uses) — callers that just want to know "what did I hit and what
/// does it feel like" don't need a second ECS lookup; anything else about the collider (its
/// halfExtents, collisionLayer/collisionMask) is available via `entity` if a caller needs it.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct RaycastHitUVE {
    Scene::EntityUVE entity;
    /// `ray.origin + ray.direction * distance`.
    Math::Vector3UVE point;
    /// Unit vector pointing outward from the hit target: an axis-aligned face for boxes, or the
    /// exact sphere/capsule surface normal for rounded targets. Zero when the ray starts inside.
    Math::Vector3UVE normal;
    float distance = 0.0F;
    PhysicsMaterialUVE material;
};

} // namespace UVE::Physics
