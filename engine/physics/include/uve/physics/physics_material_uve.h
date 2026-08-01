//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <algorithm>

#include "uve/scene/components/collider_component_uve.h"

namespace UVE::Physics {

/// PhysicsMaterialUVE is the spec's `PhysicsMaterialUVE` (Part 7.5): friction, restitution,
/// density. It is deliberately a transient, derived, read-only value — never stored in the ECS.
/// `Scene::ColliderComponentUVE` already owns the source-of-truth friction/restitution/density
/// fields as plain floats (storing a `Physics::PhysicsMaterialUVE` field there instead would
/// require `engine/scene` to depend on `engine/physics`, which already depends on `engine/scene`
/// — a circular module dependency this codebase has never had). This type exists purely so
/// PhysicsSystemUVE's collision resolution and RaycastHitUVE (Increment 16) have a small,
/// self-contained value to combine/return, matching CollisionPairUVE's precedent for a plain
/// physics-internal value type.
struct PhysicsMaterialUVE {
    float friction = 0.0F;
    float restitution = 0.0F;
    float density = 1.0F;
};

/// Combines two colliding surfaces' materials by simple average — the simplest well-defined rule.
/// No per-pair configurable combine mode (min/max/multiply) this increment, matching the "don't
/// build ahead of a real second consumer" discipline; revisitable later without an interface
/// change, since this is a pure implementation detail, not part of any public interface signature.
[[nodiscard]] constexpr PhysicsMaterialUVE CombineMaterialsUVE(const PhysicsMaterialUVE& a,
                                                                 const PhysicsMaterialUVE& b) noexcept {
    return PhysicsMaterialUVE{(a.friction + b.friction) * 0.5F, (a.restitution + b.restitution) * 0.5F,
                               (a.density + b.density) * 0.5F};
}

/// Builds a PhysicsMaterialUVE from a collider's friction/restitution/density fields, clamping
/// friction and restitution to [0,1] at this read boundary — a defensive guard against corrupted
/// or hand-edited scene data producing energy-gaining bounces (restitution > 1) or inverted
/// friction (negative), without a general NaN/validation framework (out of scope this increment).
/// Shared by PhysicsSystemUVE's resolution and RaycastSystemUVE's hit reporting so both read this
/// data through the exact same clamp, never two independently-maintained copies of it.
[[nodiscard]] constexpr PhysicsMaterialUVE MaterialOfUVE(const Scene::ColliderComponentUVE& collider) noexcept {
    return PhysicsMaterialUVE{std::clamp(collider.friction, 0.0F, 1.0F), std::clamp(collider.restitution, 0.0F, 1.0F),
                               collider.density};
}

} // namespace UVE::Physics
