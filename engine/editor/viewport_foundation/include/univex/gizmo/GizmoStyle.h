// univex/gizmo/GizmoStyle.h
// -----------------------------------------------------------------------
// Every tunable of the transform gizmos and the orientation nav gizmo.
//
// Line widths are in PIXELS, not world units. The renderer expands each
// segment into a screen-space quad, so a 2.4 here means 2.4 px on screen
// whether the camera is 20 cm or 2 km away — which is the only way a
// gizmo stays legible across an editor's whole zoom range, and the reason
// the old "thick at some zooms, hairline at others" look is gone.
//
// Likewise the whole gizmo is sized in pixels (gizmoPixelRadius): the
// geometry below is authored in abstract gizmo units and scaled per frame
// so the widget occupies a constant slice of the screen.
// -----------------------------------------------------------------------
#pragma once

#include "univex/math/Vec.h"

namespace univex::gizmo {

using univex::math::Vec3;

struct GizmoStyle {
    // ---- overall on-screen size ------------------------------------------
    float gizmoPixelRadius = 155.f; // screen radius of the widget, in pixels

    // ---- axis colours (shared with the grid's axis lines) ----------------
    Vec3 axisColorX{1.000f, 0.365f, 0.365f}; // #ff5d5d
    Vec3 axisColorY{0.373f, 0.878f, 0.541f}; // #5fe08a
    Vec3 axisColorZ{0.357f, 0.616f, 1.000f}; // #5b9dff
    Vec3 planeColor{0.933f, 0.945f, 0.965f};
    Vec3 freeRingColor{0.906f, 0.918f, 0.949f};
    Vec3 centerColor{0.643f, 0.678f, 0.749f};

    // ---- line weights, in pixels -----------------------------------------
    float axisLineWidthPx = 2.3f;
    float ringLineWidthPx = 2.4f;
    float freeRingWidthPx = 1.5f;
    float cubeEdgeWidthPx = 0.9f;
    float centerCubeWidthPx = 1.3f;

    // ---- move gizmo -------------------------------------------------------
    float moveShaftStart = 0.18f;
    float moveShaftEnd = 1.28f;
    float moveConeLength = 0.34f;
    float moveConeRadius = 0.105f;
    int   moveConeSegments = 28;

    float planeHandleOffset = 0.42f;
    float planeHandleSize = 0.30f;
    float planeHandleAlpha = 0.22f;

    // ---- rotate gizmo -----------------------------------------------------
    float ringRadius = 1.30f;
    float freeRingRadius = 1.55f;
    int   ringSegments = 96;
    // Ring samples whose camera-space depth is behind this are dropped, so
    // only the near-side arc is drawn and the three rings never turn into
    // an unreadable ball of overlapping circles.
    float ringFrontBias = 0.0f;

    // ---- scale gizmo ------------------------------------------------------
    float scaleShaftStart = 0.18f;
    float scaleShaftEnd = 1.34f;
    float scaleBoxSize = 0.19f;
    float scalePlaneOffset = 0.60f;
    float scalePlanePull = 0.28f;

    // ---- universal (all-in-one) gizmo -------------------------------------
    // Retuned after the compact version read as one cramped blob: the move
    // arrow reaches noticeably further out, the scale cube sits well past
    // its tip instead of touching it, the rotate ring pulls in closer to
    // the pivot, and every line is thinner so the three tools on one axis
    // stay separately readable.
    float universalRingRadius = 0.72f;
    float universalShaftStart = 0.18f;
    float universalShaftEnd = 1.24f;
    float universalConeLength = 0.30f;
    float universalConeRadius = 0.090f;
    float universalScaleBoxOffset = 1.86f;
    float universalScaleBoxSize = 0.165f;
    float universalLineWidthPx = 2.0f;
    float universalRingWidthPx = 2.0f;

    // ---- selected-object stand-in ----------------------------------------
    float centerCubeSize = 0.20f;

    // ---- orientation (nav) gizmo -----------------------------------------
    float navPixelSize = 154.f;   // side of the square corner viewport, px
    float navMarginPx = 16.f;
    float navCubeSize = 1.10f;    // in nav-gizmo units
    float navAxisLineWidthPx = 2.6f;
    float navBallRadius = 0.30f;  // radius of the axis end balls
    int   navBallSegments = 32;
    float navFaceAlpha = 0.16f;
    float navEdgeWidthPx = 1.4f;

    // Axis letters on the positive balls, drawn as vector strokes (no font
    // dependency for three glyphs) sized as a fraction of the ball radius.
    float navLabelScale = 0.58f;
    float navLabelWidthPx = 2.0f;
    Vec3  navLabelColor{0.078f, 0.090f, 0.125f}; // dark, to read on the bright balls
};

} // namespace univex::gizmo
