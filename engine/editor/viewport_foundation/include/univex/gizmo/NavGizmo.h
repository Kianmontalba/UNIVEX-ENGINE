// univex/gizmo/NavGizmo.h
// -----------------------------------------------------------------------
// The orientation widget that lives in the corner of the viewport: three
// coloured axis stubs with a ball on each end, six balls in all.
//
// It answers two different gestures, which is the point of it:
//   - DRAG anywhere on it  -> free orbit, exactly like dragging the scene
//   - CLICK a ball         -> eased snap to that axis
// The renderer decides which happened by whether the pointer moved past a
// small threshold before release, so a click never jerks the view and a
// drag never snaps at the end of it.
//
// Rendered through the same GizmoMesh path as the transform gizmos, into
// its own small square viewport with an orthographic camera that carries
// the main camera's rotation and nothing else.
// -----------------------------------------------------------------------
#pragma once

#include <array>

#include "univex/gizmo/GizmoGeometry.h"
#include "univex/gizmo/GizmoStyle.h"
#include "univex/math/Mat4.h"
#include "univex/math/Vec.h"

namespace univex::gizmo {

using univex::math::Mat4;
using univex::math::Vec3;

struct NavHandle {
    Vec3 direction{};  // unit axis this ball snaps the camera to
    Vec3 color{};
    bool positive = true;
    char axisLabel = 'X';
};

// The six handles: +X -X +Y -Y +Z -Z, in that order.
[[nodiscard]] std::array<NavHandle, 6> NavHandles(const GizmoStyle& style);

// Half-extent of the nav gizmo's orthographic view volume, in nav units.
// Sized so the balls never clip the edge of the corner viewport.
[[nodiscard]] float NavViewHalfExtent(const GizmoStyle& style);

// The widget comes back in two layers, because it needs three alternating
// passes and one mesh can only express two: axis stubs UNDER the balls, and
// the axis letters OVER them. Draw `underlay` first, then `overlay`.
struct NavGizmoMeshes {
    GizmoMesh underlay; // axis stubs (lines)
    GizmoMesh overlay;  // balls (triangles) then their letters (lines)
};

// Builds the widget for the current view direction (in nav-local space,
// which is just world space — the nav camera only ever rotates).
[[nodiscard]] NavGizmoMeshes BuildNavGizmoMeshes(const GizmoStyle& style,
                                                 const Vec3& viewDirection);

struct NavPickResult {
    bool hit = false;
    Vec3 direction{};
    char axisLabel = 0;
    bool positive = true;
};

// Hit-tests a pointer position given in pixels relative to the top-left of
// the nav viewport. `viewRotation` is the main camera's view matrix (only
// its rotation is used). Returns the frontmost ball under the cursor.
[[nodiscard]] NavPickResult PickNavGizmo(const GizmoStyle& style,
                                         const Mat4& viewRotation,
                                         float localX,
                                         float localY,
                                         float viewportSizePx);

} // namespace univex::gizmo
