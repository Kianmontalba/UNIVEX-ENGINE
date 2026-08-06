// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Physics {

/// One overlapping pair reported by ICollisionSystemUVE::DetectCollisionsUVE(). Deliberately a
/// plain, backend-agnostic value type — `separationAxis`/`penetrationDepth` are exactly the
/// minimum-translation-vector shape any convex-vs-convex narrow phase (box-vs-box today; GJK/EPA
/// for other shapes in the future) would produce, so this shape does not need to change when
/// CollisionSystemUVE's internals grow beyond axis-aligned boxes.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct CollisionPairUVE {
    Scene::EntityUVE first;
    Scene::EntityUVE second;
    /// Unit vector, points from `first` toward `second`.
    Math::Vector3UVE separationAxis;
    /// Minimum-translation-vector depth along separationAxis.
    float penetrationDepth = 0.0F;
};

} // namespace UVE::Physics
