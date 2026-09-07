// univex/gizmo/GizmoGeometry.h
// -----------------------------------------------------------------------
// Builds the transform gizmos as plain geometry: line segments (which the
// renderer turns into screen-space quads — real triangles, never GL_LINES)
// and solid triangles.
//
// Everything is authored around the pivot in abstract "gizmo units"; the
// renderer scales it per frame so the widget keeps a constant size on
// screen. Nothing here knows about OpenGL.
//
// The rotate rings need the view direction: only the near-side arc of each
// ring is emitted, because three full circles drawn over each other read
// as a ball of spaghetti rather than as three axes.
// -----------------------------------------------------------------------
#pragma once

#include <vector>

#include "univex/gizmo/GizmoStyle.h"
#include "univex/math/Vec.h"

namespace univex::gizmo {

using univex::math::Vec3;

enum class GizmoMode {
    Select,
    Move,
    Rotate,
    Scale,
    Universal,
};

[[nodiscard]] const char* GizmoModeName(GizmoMode mode);

struct GizmoLine {
    Vec3 a{}, b{};
    Vec3 color{};
    float widthPx = 2.f;
};

struct GizmoTriangle {
    Vec3 a{}, b{}, c{};
    Vec3 color{};
    float alpha = 1.f;
};

struct GizmoMesh {
    std::vector<GizmoLine> lines;
    std::vector<GizmoTriangle> triangles;

    void Clear() { lines.clear(); triangles.clear(); }
    [[nodiscard]] bool Empty() const { return lines.empty() && triangles.empty(); }
};

// `viewDirection` points from the camera toward the pivot (normalised).
// Used for near-side arc selection and to orient the screen-facing free
// rotation ring.
//
// `unitsPerPixel` converts a pixel width into gizmo units. Curved strokes
// (the rotation rings) are built as solid annuli rather than as stroked
// polylines: a circle stroked from independent per-segment quads shows a
// hatched, gear-toothed edge wherever the chord gets shorter than the line
// is wide, which is exactly what happens on a small ring. An annulus tiles
// seamlessly by construction, so it needs the width up front, in units.
[[nodiscard]] GizmoMesh BuildGizmoMesh(GizmoMode mode,
                                       const GizmoStyle& style,
                                       const Vec3& viewDirection,
                                       float unitsPerPixel);

} // namespace univex::gizmo
