//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string>

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// A row-major 4x4 single-precision matrix (`m[row][col]`), used for world/view/projection
/// transforms. Column-vector convention: a point is transformed via `M * [x, y, z, 1]^T`, and
/// composing `lhs * rhs` means "apply `rhs` first, then `lhs`" (so a view-projection matrix is
/// `projection * view`). Deliberately minimal, matching Vector3UVE/QuaternionUVE's precedent:
/// only what CameraSystemUVE/MeshRendererUVE (Part 7.2, Increments 10-14) actually need —
/// identity, TRS composition, a Vulkan-depth-range ([0,1]) perspective projection, a
/// closed-form camera view matrix (no generic 4x4 inverse is implemented; the one place an
/// inverse is conceptually needed, world-to-view, has a cheaper direct construction from
/// position+rotation since a camera is never non-uniformly scaled in practice), matrix
/// multiply, and affine point transform. `PerspectiveUVE` produces Y-up NDC (matching this
/// engine's Y-up convention throughout, e.g. `WorldTransformComponentUVE`) rather than
/// Vulkan's native Y-down NDC; reconciling that (negating a row, or a negative-height
/// viewport) is a real Vulkan-backend decision deferred to whichever future increment first
/// implements one — it has no effect on any CPU-side math or test in this engine today.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct Matrix4x4UVE {
    float m[4][4] = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F},
    };

    /// Returns the identity matrix. Equivalent to the default constructor; provided for
    /// readability at call sites.
    [[nodiscard]] static constexpr Matrix4x4UVE IdentityUVE() noexcept { return Matrix4x4UVE{}; }

    /// Builds a world matrix from translation, rotation, and (possibly non-uniform) scale,
    /// applied in the usual scale-then-rotate-then-translate order.
    [[nodiscard]] static Matrix4x4UVE ComposeTrsUVE(Vector3UVE translation, QuaternionUVE rotation,
                                                     Vector3UVE scale) noexcept;

    /// Builds a right-handed perspective projection matrix with Vulkan-style depth range
    /// `[0, 1]` (near plane maps to depth 0, far plane maps to depth 1) and Y-up NDC (see the
    /// class doc comment). `fovYRadians` is the full vertical field of view.
    [[nodiscard]] static Matrix4x4UVE PerspectiveUVE(float fovYRadians, float aspectRatio, float nearPlane,
                                                      float farPlane) noexcept;

    /// Builds a camera view matrix directly from world position and rotation (equivalent to,
    /// but cheaper than, inverting a TRS world matrix with unit scale). The camera's local
    /// forward is `-Z`, local up is `+Y`, local right is `+X`, matching this matrix's
    /// column-vector convention.
    [[nodiscard]] static Matrix4x4UVE ViewFromPositionAndRotationUVE(Vector3UVE position,
                                                                      QuaternionUVE rotation) noexcept;
};

[[nodiscard]] constexpr bool operator==(const Matrix4x4UVE& lhs, const Matrix4x4UVE& rhs) noexcept {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (lhs.m[row][col] != rhs.m[row][col]) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] constexpr bool operator!=(const Matrix4x4UVE& lhs, const Matrix4x4UVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Matrix multiplication: `lhs * rhs` means "apply `rhs` first, then `lhs`" (column-vector
/// convention), e.g. a view-projection matrix is `PerspectiveUVE(...) * ViewFromPositionAndRotationUVE(...)`.
[[nodiscard]] Matrix4x4UVE operator*(const Matrix4x4UVE& lhs, const Matrix4x4UVE& rhs) noexcept;

/// Transforms `point` by `matrix`, treating it as an affine point (`w = 1`). Does not perform a
/// perspective divide — callers must pass an affine matrix (e.g. from `ComposeTrsUVE` or
/// `ViewFromPositionAndRotationUVE`), not a projection matrix.
[[nodiscard]] Vector3UVE TransformPointUVE(const Matrix4x4UVE& matrix, Vector3UVE point) noexcept;

/// Formats `matrix` as four `"(row0; row1; row2; row3)"`-style rows, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const Matrix4x4UVE& matrix);

} // namespace UVE::Math
