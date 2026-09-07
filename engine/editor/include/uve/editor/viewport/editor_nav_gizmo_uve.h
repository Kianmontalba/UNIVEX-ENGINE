// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>

#include "uve/editor/viewport/editor_gizmo_geometry_uve.h"
#include "uve/editor/viewport/editor_gizmo_style_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Rotates `direction` by `matrix`'s upper-left 3x3, ignoring translation - the direction-only
/// counterpart of `Math::TransformPointUVE`, which the math module does not expose. Used to carry
/// a world axis into view space for nav-gizmo picking and for the gizmo renderer's screen basis.
[[nodiscard]] constexpr Math::Vector3UVE TransformDirectionUVE(const Math::Matrix4x4UVE& matrix,
                                                               const Math::Vector3UVE& direction) noexcept {
    return Math::Vector3UVE{
        (matrix.m[0][0] * direction.x) + (matrix.m[0][1] * direction.y) + (matrix.m[0][2] * direction.z),
        (matrix.m[1][0] * direction.x) + (matrix.m[1][1] * direction.y) + (matrix.m[1][2] * direction.z),
        (matrix.m[2][0] * direction.x) + (matrix.m[2][1] * direction.y) + (matrix.m[2][2] * direction.z),
    };
}

/// One end of one axis stub on the orientation widget: the unit direction it snaps the camera to,
/// its colour, and which end of the axis it is. Positive ends are solid discs with a letter;
/// negative ends are hollow rings, so the two directions of an axis are never confused.
struct EditorNavHandleUVE final {
    Math::Vector3UVE direction{};
    Math::Vector3UVE color{};
    bool positive = true;
    char axisLabel = 'X';
};

/// The six handles: +X -X +Y -Y +Z -Z, in that order.
[[nodiscard]] std::array<EditorNavHandleUVE, 6> GetNavHandlesUVE(const EditorGizmoStyleUVE& style);

/// Half-extent of the nav gizmo's orthographic view volume, in nav units - sized so the balls
/// never clip the edge of the corner viewport.
[[nodiscard]] float GetNavViewHalfExtentUVE(const EditorGizmoStyleUVE& style) noexcept;

/// The widget comes back in two layers, because it needs three alternating passes and one mesh
/// can only express two: axis stubs UNDER the balls, and the axis letters OVER them. Draw
/// `underlay` first, then `overlay`.
struct EditorNavGizmoMeshesUVE final {
    EditorGizmoMeshUVE underlay;
    EditorGizmoMeshUVE overlay;
};

/// Builds the widget for the current view direction, in nav-local space (which is just world
/// space - the nav camera only ever rotates).
[[nodiscard]] EditorNavGizmoMeshesUVE BuildNavGizmoMeshesUVE(const EditorGizmoStyleUVE& style,
                                                             const Math::Vector3UVE& viewDirection);

struct EditorNavPickResultUVE final {
    bool hit = false;
    Math::Vector3UVE direction{};
    char axisLabel = 0;
    bool positive = true;
};

/// Hit-tests a pointer position given in pixels relative to the top-left of the nav viewport.
/// `viewRotation` is the main camera's view matrix (only its rotation is used). Returns the
/// frontmost ball under the cursor, so when two balls project on top of each other - looking
/// straight down an axis - the nearer one wins.
[[nodiscard]] EditorNavPickResultUVE PickNavGizmoUVE(const EditorGizmoStyleUVE& style,
                                                     const Math::Matrix4x4UVE& viewRotation,
                                                     float localX, float localY,
                                                     float viewportSizePx);

} // namespace UVE::Editor
