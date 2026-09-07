// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <vector>

#include "uve/editor/viewport/editor_gizmo_style_uve.h"
#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// One straight stroke of a gizmo. `widthPx` is a screen-space width: the renderer expands the
/// segment into a screen-space quad carrying a distance-from-centreline value, so the stroke is
/// exactly this many pixels wide at any camera distance. Core-profile GL only guarantees 1 px
/// `GL_LINES`, which is why nothing here is drawn as a GL line.
struct EditorGizmoLineUVE final {
    Math::Vector3UVE a{};
    Math::Vector3UVE b{};
    Math::Vector3UVE color{};
    float widthPx = 2.0F;
};

/// One solid triangle of a gizmo (cone, cube face, ring annulus, plane handle).
struct EditorGizmoTriangleUVE final {
    Math::Vector3UVE a{};
    Math::Vector3UVE b{};
    Math::Vector3UVE c{};
    Math::Vector3UVE color{};
    float alpha = 1.0F;
};

/// A built gizmo: screen-space strokes plus solid triangles, all authored around the pivot in
/// abstract gizmo units. Nothing here knows about OpenGL; the renderer scales and transforms it.
struct EditorGizmoMeshUVE final {
    std::vector<EditorGizmoLineUVE> lines;
    std::vector<EditorGizmoTriangleUVE> triangles;

    void ClearUVE() noexcept {
        lines.clear();
        triangles.clear();
    }

    [[nodiscard]] bool IsEmptyUVE() const noexcept { return lines.empty() && triangles.empty(); }
};

/// Builds the transform gizmo for `mode`, centred on the origin in gizmo units - the caller
/// places it at the selected entity's transform pivot.
///
/// `viewDirection` points from the camera toward the pivot (it is normalized internally). It
/// selects the near-side arc of each rotation ring - three full circles drawn over each other
/// read as a ball of spaghetti rather than as three axes - and orients the screen-facing free
/// rotation ring.
///
/// `unitsPerPixel` converts a pixel width into gizmo units. Curved strokes (the rotation rings)
/// are built as solid annuli rather than stroked polylines: a circle stroked from independent
/// per-segment quads shows a gear-toothed edge wherever the chord gets shorter than the stroke is
/// wide, which is exactly what happens on a small ring. An annulus tiles seamlessly, so it needs
/// its width up front, in units.
[[nodiscard]] EditorGizmoMeshUVE BuildGizmoMeshUVE(EditorGizmoModeUVE mode,
                                                   const EditorGizmoStyleUVE& style,
                                                   const Math::Vector3UVE& viewDirection,
                                                   float unitsPerPixel);

} // namespace UVE::Editor
