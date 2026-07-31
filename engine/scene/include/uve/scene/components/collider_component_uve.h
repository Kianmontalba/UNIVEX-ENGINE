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

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data: the simplest possible collider shape (an axis-aligned box half-extent). A
/// real shape-type enum (box/sphere/capsule/mesh) is PhysicsSystemUVE's job (Part 7.5), not
/// invented ahead of it.
struct ColliderComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
};

} // namespace UVE::Scene
