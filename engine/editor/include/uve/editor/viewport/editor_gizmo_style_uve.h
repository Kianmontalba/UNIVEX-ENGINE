// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Every tunable of the transform gizmos and the orientation (nav) gizmo.
///
/// Line widths are in PIXELS, not world units. The renderer expands each segment into a
/// screen-space quad, so `2.3` here means 2.3 px on screen whether the camera is 20 cm or 2 km
/// away - the only way a gizmo stays legible across an editor's whole zoom range.
///
/// Likewise the widget as a whole is sized in pixels (`gizmoPixelRadius`): the geometry is
/// authored in abstract gizmo units and scaled per frame so it occupies a constant slice of the
/// screen.
///
/// The universal (all-in-one) layout keeps its three tools separated along each axis - rotate
/// ring innermost, then the move arrow, then the scale cube with a deliberate gap before it.
/// `tests/editor/viewport/editor_gizmo_geometry_uve_tests.cpp` asserts that ordering and that
/// gap, so a future tweak cannot quietly collapse them back into one another.
struct EditorGizmoStyleUVE final {
    // ---- overall on-screen size ---------------------------------------------------------------
    float gizmoPixelRadius = 155.0F;

    // ---- axis colours (shared with the grid's axis lines) -------------------------------------
    Math::Vector3UVE axisColorX{1.000F, 0.365F, 0.365F};
    Math::Vector3UVE axisColorY{0.373F, 0.878F, 0.541F};
    Math::Vector3UVE axisColorZ{0.357F, 0.616F, 1.000F};
    Math::Vector3UVE planeColor{0.933F, 0.945F, 0.965F};
    Math::Vector3UVE freeRingColor{0.906F, 0.918F, 0.949F};
    Math::Vector3UVE centerColor{0.643F, 0.678F, 0.749F};

    // ---- line weights, in pixels --------------------------------------------------------------
    float axisLineWidthPx = 2.3F;
    float ringLineWidthPx = 2.4F;
    float freeRingWidthPx = 1.5F;
    float cubeEdgeWidthPx = 0.9F;
    float centerCubeWidthPx = 1.3F;

    // ---- move gizmo ---------------------------------------------------------------------------
    float moveShaftStart = 0.18F;
    float moveShaftEnd = 1.28F;
    float moveConeLength = 0.34F;
    float moveConeRadius = 0.105F;
    int moveConeSegments = 28;

    float planeHandleOffset = 0.42F;
    float planeHandleSize = 0.30F;
    float planeHandleAlpha = 0.22F;

    // ---- rotate gizmo -------------------------------------------------------------------------
    float ringRadius = 1.30F;
    float freeRingRadius = 1.55F;
    int ringSegments = 96;
    /// Ring samples whose camera-space depth is behind this are dropped, so only the near-side arc
    /// is drawn and the three rings never turn into an unreadable ball of overlapping circles.
    float ringFrontBias = 0.0F;

    // ---- scale gizmo --------------------------------------------------------------------------
    float scaleShaftStart = 0.18F;
    float scaleShaftEnd = 1.34F;
    float scaleBoxSize = 0.19F;
    float scalePlaneOffset = 0.60F;
    float scalePlanePull = 0.28F;

    // ---- universal (all-in-one) gizmo ---------------------------------------------------------
    float universalRingRadius = 0.72F;
    float universalShaftStart = 0.18F;
    float universalShaftEnd = 1.24F;
    float universalConeLength = 0.30F;
    float universalConeRadius = 0.090F;
    float universalScaleBoxOffset = 1.86F;
    float universalScaleBoxSize = 0.165F;
    float universalLineWidthPx = 2.0F;
    float universalRingWidthPx = 2.0F;

    // ---- pivot marker -------------------------------------------------------------------------
    float centerCubeSize = 0.20F;

    // ---- orientation (nav) gizmo --------------------------------------------------------------
    float navPixelSize = 154.0F;
    float navMarginPx = 16.0F;
    float navAxisLineWidthPx = 2.6F;
    float navBallRadius = 0.30F;
    int navBallSegments = 32;

    /// Axis letters on the positive balls, drawn as vector strokes (no font dependency for three
    /// glyphs) sized as a fraction of the ball radius.
    float navLabelScale = 0.58F;
    float navLabelWidthPx = 2.0F;
    Math::Vector3UVE navLabelColor{0.078F, 0.090F, 0.125F};
    Math::Vector3UVE navHollowFillColor{0.078F, 0.090F, 0.125F};
};

} // namespace UVE::Editor
