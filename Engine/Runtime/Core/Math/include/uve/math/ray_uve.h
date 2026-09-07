// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// A ray in `origin + direction * t` form, used by RaycastSystemUVE (Part 7.5, Increment 16).
/// Deliberately minimal, matching PlaneUVE's precedent: no logic beyond the type itself.
/// Contract: `direction` is expected to be unit length (not enforced — matches NormalizeUVE's
/// documented-not-enforced precedent). A hit/miss result is still correct with a non-unit
/// direction, but any reported distance is then in units of `direction`'s own length rather than
/// a literal distance.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct RayUVE {
    Vector3UVE origin{};
    Vector3UVE direction{0.0F, 0.0F, -1.0F};
};

} // namespace UVE::Math
