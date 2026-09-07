// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "uve/editor/viewport/editor_gizmo_geometry_uve.h"
#include "uve/editor/viewport/editor_gizmo_style_uve.h"
#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kUnitsPerPixelUVE = 0.01F;
const Math::Vector3UVE kViewDirectionUVE{0.0F, 0.0F, -1.0F};

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

/// Farthest distance from the pivot any vertex of the mesh reaches, projected onto `axis`. This is
/// how the universal layout's tool ordering is measured.
[[nodiscard]] float MaxExtentAlongAxisUVE(const EditorGizmoMeshUVE& mesh,
                                          const Math::Vector3UVE& axis) {
    float maximum = 0.0F;
    for (const EditorGizmoLineUVE& line : mesh.lines) {
        maximum = std::max(maximum, Math::DotUVE(line.a, axis));
        maximum = std::max(maximum, Math::DotUVE(line.b, axis));
    }
    for (const EditorGizmoTriangleUVE& triangle : mesh.triangles) {
        maximum = std::max(maximum, Math::DotUVE(triangle.a, axis));
        maximum = std::max(maximum, Math::DotUVE(triangle.b, axis));
        maximum = std::max(maximum, Math::DotUVE(triangle.c, axis));
    }
    return maximum;
}

} // namespace

TEST(EditorGizmoGeometryUVETest, EveryModeProducesFiniteGeometry) {
    const EditorGizmoStyleUVE style{};
    for (const EditorGizmoModeUVE mode :
         {EditorGizmoModeUVE::Select, EditorGizmoModeUVE::Move, EditorGizmoModeUVE::Rotate,
          EditorGizmoModeUVE::Scale, EditorGizmoModeUVE::Universal}) {
        const EditorGizmoMeshUVE mesh =
            BuildGizmoMeshUVE(mode, style, kViewDirectionUVE, kUnitsPerPixelUVE);
        EXPECT_FALSE(mesh.IsEmptyUVE()) << GetGizmoModeNameUVE(mode);

        for (const EditorGizmoLineUVE& line : mesh.lines) {
            ASSERT_TRUE(IsFiniteVectorUVE(line.a)) << GetGizmoModeNameUVE(mode);
            ASSERT_TRUE(IsFiniteVectorUVE(line.b)) << GetGizmoModeNameUVE(mode);
            ASSERT_TRUE(IsFiniteVectorUVE(line.color)) << GetGizmoModeNameUVE(mode);
            ASSERT_GT(line.widthPx, 0.0F) << GetGizmoModeNameUVE(mode);
        }
        for (const EditorGizmoTriangleUVE& triangle : mesh.triangles) {
            ASSERT_TRUE(IsFiniteVectorUVE(triangle.a)) << GetGizmoModeNameUVE(mode);
            ASSERT_TRUE(IsFiniteVectorUVE(triangle.b)) << GetGizmoModeNameUVE(mode);
            ASSERT_TRUE(IsFiniteVectorUVE(triangle.c)) << GetGizmoModeNameUVE(mode);
            ASSERT_GE(triangle.alpha, 0.0F) << GetGizmoModeNameUVE(mode);
            ASSERT_LE(triangle.alpha, 1.0F) << GetGizmoModeNameUVE(mode);
        }
    }
}

TEST(EditorGizmoGeometryUVETest, SelectModeDrawsOnlyThePivotMarkerAndNoHandles) {
    const EditorGizmoStyleUVE style{};
    const EditorGizmoMeshUVE selectMesh =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Select, style, kViewDirectionUVE, kUnitsPerPixelUVE);
    const EditorGizmoMeshUVE moveMesh =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Move, style, kViewDirectionUVE, kUnitsPerPixelUVE);

    // The pivot cube is six quads plus twelve edges, and nothing else.
    EXPECT_EQ(selectMesh.triangles.size(), 12U);
    EXPECT_EQ(selectMesh.lines.size(), 12U);
    EXPECT_LT(selectMesh.triangles.size(), moveMesh.triangles.size());

    // No handle reaches out past the pivot marker itself.
    const float extent = MaxExtentAlongAxisUVE(selectMesh, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    EXPECT_LE(extent, style.centerCubeSize);
}

TEST(EditorGizmoGeometryUVETest, UniversalModeKeepsRotateMoveAndScaleSeparatedAlongEachAxis) {
    const EditorGizmoStyleUVE style{};

    // This is the layout assertion the viewport foundation shipped with, carried over so a future
    // tweak cannot quietly collapse the three tools back into one another: rotate ring innermost,
    // then the move arrow, then the scale cube with a real gap before it.
    EXPECT_LT(style.universalRingRadius, style.universalShaftEnd);

    const float arrowTip = style.universalShaftEnd + style.universalConeLength;
    const float scaleBoxStart = style.universalScaleBoxOffset - (style.universalScaleBoxSize * 0.5F);
    EXPECT_LT(arrowTip, scaleBoxStart) << "the scale cube must not touch the move arrow tip";
    EXPECT_GT(scaleBoxStart - arrowTip, 0.1F) << "the deliberate gap between move and scale is gone";

    // And the built geometry actually honours it.
    const EditorGizmoMeshUVE mesh = BuildGizmoMeshUVE(EditorGizmoModeUVE::Universal, style,
                                                      kViewDirectionUVE, kUnitsPerPixelUVE);
    const float reach = MaxExtentAlongAxisUVE(mesh, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    const float scaleBoxOuter = style.universalScaleBoxOffset + (style.universalScaleBoxSize * 0.5F);
    EXPECT_NEAR(reach, scaleBoxOuter, 1e-4F);
}

TEST(EditorGizmoGeometryUVETest, RotateModeEmitsOnlyTheNearSideArcOfEachAxisRing) {
    const EditorGizmoStyleUVE style{};
    const EditorGizmoMeshUVE mesh =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Rotate, style, kViewDirectionUVE, kUnitsPerPixelUVE);

    // The free ring is a full circle; the three axis rings are half arcs. A full set of four
    // complete rings would be 4 * ringSegments * 2 triangles, so the real count sits well under
    // that - three full circles drawn over each other read as a ball of spaghetti.
    const std::size_t fullRingTriangles = static_cast<std::size_t>(style.ringSegments) * 2U;
    const std::size_t pivotCubeTriangles = 12U;
    const std::size_t ringTriangles = mesh.triangles.size() - pivotCubeTriangles;

    EXPECT_LT(ringTriangles, fullRingTriangles * 4U);
    // One full free ring plus three roughly-half arcs.
    EXPECT_GT(ringTriangles, fullRingTriangles);
}

TEST(EditorGizmoGeometryUVETest, RingWidthTracksUnitsPerPixelSoStrokesStayConstantOnScreen) {
    const EditorGizmoStyleUVE style{};

    // Ring strokes are solid annuli, so their pixel width has to be converted into gizmo units up
    // front. Zooming in halves unitsPerPixel, which must halve the annulus thickness in units so
    // the stroke stays the same number of pixels wide.
    const EditorGizmoMeshUVE wide =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Rotate, style, kViewDirectionUVE, 0.02F);
    const EditorGizmoMeshUVE narrow =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Rotate, style, kViewDirectionUVE, 0.01F);

    const Math::Vector3UVE probeAxis{0.0F, 1.0F, 0.0F};
    const float wideReach = MaxExtentAlongAxisUVE(wide, probeAxis);
    const float narrowReach = MaxExtentAlongAxisUVE(narrow, probeAxis);

    // The outer edge of the free ring sits at radius + halfWidth, so the wider stroke reaches
    // further out in gizmo units.
    EXPECT_GT(wideReach, narrowReach);

    // A non-positive unitsPerPixel must not produce a degenerate or inverted annulus.
    const EditorGizmoMeshUVE guarded =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Rotate, style, kViewDirectionUVE, 0.0F);
    EXPECT_FALSE(guarded.IsEmptyUVE());
}

TEST(EditorGizmoGeometryUVETest, MoveAndScaleUseTheStyledPixelLineWidths) {
    const EditorGizmoStyleUVE style{};

    const EditorGizmoMeshUVE moveMesh =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Move, style, kViewDirectionUVE, kUnitsPerPixelUVE);
    bool sawAxisWidth = false;
    for (const EditorGizmoLineUVE& line : moveMesh.lines) {
        if (std::fabs(line.widthPx - style.axisLineWidthPx) < 1e-5F) {
            sawAxisWidth = true;
            break;
        }
    }
    EXPECT_TRUE(sawAxisWidth) << "move shafts must carry the styled axis stroke width in pixels";

    const EditorGizmoMeshUVE universalMesh = BuildGizmoMeshUVE(
        EditorGizmoModeUVE::Universal, style, kViewDirectionUVE, kUnitsPerPixelUVE);
    bool sawUniversalWidth = false;
    for (const EditorGizmoLineUVE& line : universalMesh.lines) {
        if (std::fabs(line.widthPx - style.universalLineWidthPx) < 1e-5F) {
            sawUniversalWidth = true;
            break;
        }
    }
    EXPECT_TRUE(sawUniversalWidth) << "the all-in-one gizmo uses its own thinner stroke width";
}

TEST(EditorGizmoGeometryUVETest, AxisHandlesCarryTheDistinctPerAxisColours) {
    const EditorGizmoStyleUVE style{};
    const EditorGizmoMeshUVE mesh =
        BuildGizmoMeshUVE(EditorGizmoModeUVE::Move, style, kViewDirectionUVE, kUnitsPerPixelUVE);

    bool sawX = false;
    bool sawY = false;
    bool sawZ = false;
    for (const EditorGizmoLineUVE& line : mesh.lines) {
        sawX = sawX || line.color == style.axisColorX;
        sawY = sawY || line.color == style.axisColorY;
        sawZ = sawZ || line.color == style.axisColorZ;
    }
    EXPECT_TRUE(sawX);
    EXPECT_TRUE(sawY);
    EXPECT_TRUE(sawZ);
}

TEST(EditorGizmoGeometryUVETest, GizmoModeNamesAreStableForTheViewportToolbar) {
    EXPECT_STREQ(GetGizmoModeNameUVE(EditorGizmoModeUVE::Select), "Select");
    EXPECT_STREQ(GetGizmoModeNameUVE(EditorGizmoModeUVE::Move), "Move");
    EXPECT_STREQ(GetGizmoModeNameUVE(EditorGizmoModeUVE::Rotate), "Rotate");
    EXPECT_STREQ(GetGizmoModeNameUVE(EditorGizmoModeUVE::Scale), "Scale");
    EXPECT_STREQ(GetGizmoModeNameUVE(EditorGizmoModeUVE::Universal), "Universal");
}

} // namespace UVE::Editor
