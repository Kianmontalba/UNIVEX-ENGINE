// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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

    /// Builds a right-handed orthographic projection matrix with the same Vulkan-style depth range
    /// `[0, 1]` and Y-up NDC as PerspectiveUVE (near plane maps to depth 0, far plane maps to depth
    /// 1). `left`/`right`/`bottom`/`top` describe the projection box in view space. Introduced for
    /// directional-light shadow mapping (Increment 26), where a light has no meaningful field of
    /// view to derive a perspective frustum from.
    [[nodiscard]] static Matrix4x4UVE OrthographicUVE(float left, float right, float bottom, float top,
                                                       float nearPlane, float farPlane) noexcept;

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

/// Returns `matrix` transposed (`result.m[row][col] == matrix.m[col][row]`). Always succeeds -
/// unlike TryInverseUVE(), transposition has no degenerate case.
[[nodiscard]] Matrix4x4UVE TransposeUVE(const Matrix4x4UVE& matrix) noexcept;

/// Attempts a general 4x4 matrix inverse via Gauss-Jordan elimination with partial pivoting.
/// Returns false (leaving `outInverse` unspecified) if `matrix` is non-finite or numerically
/// singular, matching the TryInverseUVE()/TryNormalizeUVE() convention used elsewhere in this
/// module - callers must check the return value rather than assume a result. Handles the general
/// case (any invertible 4x4, not just affine TRS matrices), since the primary use case - deriving
/// a normal matrix as TransposeUVE(inverse) for correct lighting under non-uniform scale - only
/// needs the upper-left 3x3 of the result, but a full 4x4 inverse is no more expensive to compute
/// and stays reusable for future non-affine cases.
[[nodiscard]] bool TryInverseUVE(const Matrix4x4UVE& matrix, Matrix4x4UVE& outInverse) noexcept;

} // namespace UVE::Math
