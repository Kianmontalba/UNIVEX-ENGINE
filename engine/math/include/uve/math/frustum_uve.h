//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <array>

#include "uve/math/aabb_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/plane_uve.h"

namespace UVE::Math {

/// A camera's view frustum as 6 inward-facing planes (left, right, bottom, top, near, far, in
/// that order), used for frustum culling (Part 7.2, Increments 11-14).
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct FrustumUVE {
    std::array<PlaneUVE, 6> planes{};

    /// Extracts the 6 frustum planes from a combined view-projection matrix (standard
    /// Gribb-Hartmann plane extraction, a well-known public-domain technique), matching
    /// Matrix4x4UVE::PerspectiveUVE's `[0, 1]` depth-range convention.
    [[nodiscard]] static FrustumUVE FromViewProjectionUVE(const Matrix4x4UVE& viewProjection) noexcept;

    /// True iff `box` is at least partially inside every plane (i.e. not fully rejected by any
    /// single plane) — the standard conservative "is this box visible" test used for culling.
    [[nodiscard]] bool IntersectsUVE(const AabbUVE& box) const noexcept;
};

} // namespace UVE::Math
