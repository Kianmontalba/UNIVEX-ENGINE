// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/viewport/editor_gizmo_geometry_uve.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <utility>

namespace UVE::Editor {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;

struct GizmoAxisUVE final {
    Math::Vector3UVE direction;
    Math::Vector3UVE color;
};

[[nodiscard]] std::array<GizmoAxisUVE, 3> AxesOfUVE(const EditorGizmoStyleUVE& style) {
    return {{
        {Math::Vector3UVE{1.0F, 0.0F, 0.0F}, style.axisColorX},
        {Math::Vector3UVE{0.0F, 1.0F, 0.0F}, style.axisColorY},
        {Math::Vector3UVE{0.0F, 0.0F, 1.0F}, style.axisColorZ},
    }};
}

/// Two unit vectors perpendicular to `axis` and to each other. The helper vector is swapped near
/// the pole so the cross product never collapses.
void PerpBasisUVE(const Math::Vector3UVE& axis, Math::Vector3UVE& outU, Math::Vector3UVE& outV) {
    const Math::Vector3UVE normalizedAxis = Math::NormalizeUVE(axis);
    const Math::Vector3UVE helper = (std::fabs(normalizedAxis.y) < 0.98F)
                                        ? Math::Vector3UVE{0.0F, 1.0F, 0.0F}
                                        : Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    outU = Math::NormalizeUVE(Math::CrossUVE(helper, normalizedAxis));
    outV = Math::NormalizeUVE(Math::CrossUVE(normalizedAxis, outU));
}

[[nodiscard]] std::vector<Math::Vector3UVE> CirclePointsUVE(const Math::Vector3UVE& center,
                                                            const Math::Vector3UVE& u,
                                                            const Math::Vector3UVE& v,
                                                            const float radius, const int segments) {
    std::vector<Math::Vector3UVE> points;
    points.reserve(static_cast<std::size_t>(segments) + 1U);
    for (int index = 0; index <= segments; ++index) {
        const float t = (2.0F * kPiUVE * static_cast<float>(index)) / static_cast<float>(segments);
        points.push_back(center + (u * (std::cos(t) * radius)) + (v * (std::sin(t) * radius)));
    }
    return points;
}

void AddLineUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& a, const Math::Vector3UVE& b,
                const Math::Vector3UVE& color, const float widthPx) {
    mesh.lines.push_back(EditorGizmoLineUVE{a, b, color, widthPx});
}

void AddTriangleUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& a, const Math::Vector3UVE& b,
                    const Math::Vector3UVE& c, const Math::Vector3UVE& color, const float alpha) {
    mesh.triangles.push_back(EditorGizmoTriangleUVE{a, b, c, color, alpha});
}

void AddQuadUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& p0, const Math::Vector3UVE& p1,
                const Math::Vector3UVE& p2, const Math::Vector3UVE& p3, const Math::Vector3UVE& color,
                const float alpha) {
    AddTriangleUVE(mesh, p0, p1, p2, color, alpha);
    AddTriangleUVE(mesh, p0, p2, p3, color, alpha);
}

/// Component accessor by axis index, so the cube edge loop below can walk the three axes without
/// pointer arithmetic over struct members.
[[nodiscard]] float& ComponentUVE(Math::Vector3UVE& vector, const int axisIndex) noexcept {
    if (axisIndex == 0) {
        return vector.x;
    }
    if (axisIndex == 1) {
        return vector.y;
    }
    return vector.z;
}

/// A solid, axis-aligned cube: six filled faces plus twelve darker edges, so the handle reads as
/// a body with a defined silhouette rather than a wire box that disappears against the grid.
void AddSolidCubeUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& center, const float size,
                     const Math::Vector3UVE& color, const float edgeWidthPx) {
    const float halfSize = size * 0.5F;

    struct CubeFaceUVE final {
        Math::Vector3UVE normal;
        Math::Vector3UVE u;
        Math::Vector3UVE v;
    };

    const std::array<CubeFaceUVE, 6> faces = {{
        {{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{0.0F, 1.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{0.0F, -1.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}},
        {{0.0F, 0.0F, -1.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}},
    }};

    for (const CubeFaceUVE& face : faces) {
        const Math::Vector3UVE faceCenter = center + (face.normal * halfSize);
        AddQuadUVE(mesh, faceCenter + (face.u * -halfSize) + (face.v * -halfSize),
                   faceCenter + (face.u * halfSize) + (face.v * -halfSize),
                   faceCenter + (face.u * halfSize) + (face.v * halfSize),
                   faceCenter + (face.u * -halfSize) + (face.v * halfSize), color, 1.0F);
    }

    const Math::Vector3UVE edgeColor = color * 0.55F;
    for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
        for (int corner = 0; corner < 4; ++corner) {
            Math::Vector3UVE start = center;
            Math::Vector3UVE end = center;
            const float offsetU = ((corner & 1) != 0) ? halfSize : -halfSize;
            const float offsetV = ((corner & 2) != 0) ? halfSize : -halfSize;
            const int axisU = (axisIndex + 1) % 3;
            const int axisV = (axisIndex + 2) % 3;
            ComponentUVE(start, axisU) += offsetU;
            ComponentUVE(end, axisU) += offsetU;
            ComponentUVE(start, axisV) += offsetV;
            ComponentUVE(end, axisV) += offsetV;
            ComponentUVE(start, axisIndex) -= halfSize;
            ComponentUVE(end, axisIndex) += halfSize;
            AddLineUVE(mesh, start, end, edgeColor, edgeWidthPx);
        }
    }
}

/// Arrow head: a cone of real triangles, capped so it stays solid when seen from behind.
void AddConeUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& baseCenter,
                const Math::Vector3UVE& axis, const float length, const float radius,
                const int segments, const Math::Vector3UVE& color) {
    Math::Vector3UVE u{};
    Math::Vector3UVE v{};
    PerpBasisUVE(axis, u, v);
    const Math::Vector3UVE tip = baseCenter + (axis * length);
    const std::vector<Math::Vector3UVE> ring = CirclePointsUVE(baseCenter, u, v, radius, segments);
    for (std::size_t index = 0U; index + 1U < ring.size(); ++index) {
        AddTriangleUVE(mesh, tip, ring[index], ring[index + 1U], color, 1.0F);
        AddTriangleUVE(mesh, baseCenter, ring[index + 1U], ring[index], color, 1.0F);
    }
}

void AddMoveArrowUVE(EditorGizmoMeshUVE& mesh, const GizmoAxisUVE& axis,
                     const EditorGizmoStyleUVE& style, const float shaftStart, const float shaftEnd,
                     const float coneLength, const float coneRadius, const float lineWidthPx) {
    AddLineUVE(mesh, axis.direction * shaftStart, axis.direction * shaftEnd, axis.color, lineWidthPx);
    AddConeUVE(mesh, axis.direction * shaftEnd, axis.direction, coneLength, coneRadius,
               style.moveConeSegments, axis.color);
}

/// A ring drawn as a solid annulus: two concentric circles joined by quads, seamless where a
/// stroked polyline would show its joints. `keepSegment` decides which parts survive, which is
/// where the near-side arc selection happens.
template <typename TKeepSegment>
void AddAnnulusUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& axis,
                   const Math::Vector3UVE& color, const float radius, const float halfWidth,
                   const int segments, TKeepSegment keepSegment) {
    Math::Vector3UVE u{};
    Math::Vector3UVE v{};
    PerpBasisUVE(axis, u, v);
    const Math::Vector3UVE origin{0.0F, 0.0F, 0.0F};
    const std::vector<Math::Vector3UVE> inner = CirclePointsUVE(origin, u, v, radius - halfWidth, segments);
    const std::vector<Math::Vector3UVE> outer = CirclePointsUVE(origin, u, v, radius + halfWidth, segments);
    const std::vector<Math::Vector3UVE> mid = CirclePointsUVE(origin, u, v, radius, segments);

    for (std::size_t index = 0U; index + 1U < mid.size(); ++index) {
        if (!keepSegment(mid[index], mid[index + 1U])) {
            continue;
        }
        AddTriangleUVE(mesh, inner[index], outer[index], outer[index + 1U], color, 1.0F);
        AddTriangleUVE(mesh, inner[index], outer[index + 1U], inner[index + 1U], color, 1.0F);
    }
}

/// Near-side arc only: a segment survives when both endpoints face the camera. On a ring centred
/// at the pivot that is exactly the front half.
void AddRingArcUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& axis,
                   const Math::Vector3UVE& color, const float radius, const int segments,
                   const float frontBias, const Math::Vector3UVE& viewDirection,
                   const float halfWidth) {
    AddAnnulusUVE(mesh, axis, color, radius, halfWidth, segments,
                  [&viewDirection, frontBias](const Math::Vector3UVE& a, const Math::Vector3UVE& b) {
                      return Math::DotUVE(a, viewDirection) < -frontBias &&
                             Math::DotUVE(b, viewDirection) < -frontBias;
                  });
}

void AddFullRingUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& axis,
                    const Math::Vector3UVE& color, const float radius, const int segments,
                    const float halfWidth) {
    AddAnnulusUVE(mesh, axis, color, radius, halfWidth, segments,
                  [](const Math::Vector3UVE&, const Math::Vector3UVE&) { return true; });
}

void AddMovePlaneHandlesUVE(EditorGizmoMeshUVE& mesh, const EditorGizmoStyleUVE& style) {
    const std::array<GizmoAxisUVE, 3> axes = AxesOfUVE(style);
    const std::array<std::pair<int, int>, 3> pairs = {{{0, 1}, {1, 2}, {2, 0}}};
    for (const std::pair<int, int>& pair : pairs) {
        const Math::Vector3UVE a = axes[static_cast<std::size_t>(pair.first)].direction;
        const Math::Vector3UVE b = axes[static_cast<std::size_t>(pair.second)].direction;
        const float offset = style.planeHandleOffset;
        const float size = style.planeHandleSize;
        const Math::Vector3UVE p0 = (a * offset) + (b * offset);
        const Math::Vector3UVE p1 = (a * (offset + size)) + (b * offset);
        const Math::Vector3UVE p2 = (a * (offset + size)) + (b * (offset + size));
        const Math::Vector3UVE p3 = (a * offset) + (b * (offset + size));
        AddQuadUVE(mesh, p0, p1, p2, p3, style.planeColor, style.planeHandleAlpha);
        AddLineUVE(mesh, p0, p1, style.planeColor, 1.1F);
        AddLineUVE(mesh, p1, p2, style.planeColor, 1.1F);
        AddLineUVE(mesh, p2, p3, style.planeColor, 1.1F);
        AddLineUVE(mesh, p3, p0, style.planeColor, 1.1F);
    }
}

void AddScalePlaneHandlesUVE(EditorGizmoMeshUVE& mesh, const EditorGizmoStyleUVE& style) {
    const std::array<GizmoAxisUVE, 3> axes = AxesOfUVE(style);
    const std::array<std::pair<int, int>, 3> pairs = {{{0, 1}, {1, 2}, {2, 0}}};
    for (const std::pair<int, int>& pair : pairs) {
        const Math::Vector3UVE a = axes[static_cast<std::size_t>(pair.first)].direction;
        const Math::Vector3UVE b = axes[static_cast<std::size_t>(pair.second)].direction;
        const Math::Vector3UVE p0 = a * style.scalePlaneOffset;
        const Math::Vector3UVE p1 = b * style.scalePlaneOffset;
        const Math::Vector3UVE p2 = (a + b) * style.scalePlanePull;
        AddTriangleUVE(mesh, p0, p1, p2, style.planeColor, style.planeHandleAlpha);
        AddLineUVE(mesh, p0, p1, style.planeColor, 1.1F);
    }
}

/// A small solid block at the pivot. A wire box was tried first: twelve edges a few pixels long
/// read as scribble rather than as a cube.
void AddCenterCubeUVE(EditorGizmoMeshUVE& mesh, const EditorGizmoStyleUVE& style) {
    AddSolidCubeUVE(mesh, Math::Vector3UVE{0.0F, 0.0F, 0.0F}, style.centerCubeSize, style.centerColor,
                    style.centerCubeWidthPx);
}

} // namespace

EditorGizmoMeshUVE BuildGizmoMeshUVE(const EditorGizmoModeUVE mode, const EditorGizmoStyleUVE& style,
                                     const Math::Vector3UVE& viewDirection, const float unitsPerPixel) {
    EditorGizmoMeshUVE mesh;
    const std::array<GizmoAxisUVE, 3> axes = AxesOfUVE(style);
    const Math::Vector3UVE view = Math::NormalizeUVE(viewDirection);

    // Ring strokes are solid geometry, so their pixel widths are converted into gizmo units here.
    const float scale = (unitsPerPixel > 0.0F) ? unitsPerPixel : 1.0F;
    const float ringHalfWidth = style.ringLineWidthPx * 0.5F * scale;
    const float freeRingHalfWidth = style.freeRingWidthPx * 0.5F * scale;
    const float universalRingHalfWidth = style.universalRingWidthPx * 0.5F * scale;

    switch (mode) {
        case EditorGizmoModeUVE::Select:
            AddCenterCubeUVE(mesh, style);
            break;

        case EditorGizmoModeUVE::Move:
            AddCenterCubeUVE(mesh, style);
            for (const GizmoAxisUVE& axis : axes) {
                AddMoveArrowUVE(mesh, axis, style, style.moveShaftStart, style.moveShaftEnd,
                                style.moveConeLength, style.moveConeRadius, style.axisLineWidthPx);
            }
            AddMovePlaneHandlesUVE(mesh, style);
            break;

        case EditorGizmoModeUVE::Rotate:
            AddCenterCubeUVE(mesh, style);
            for (const GizmoAxisUVE& axis : axes) {
                AddRingArcUVE(mesh, axis.direction, axis.color, style.ringRadius, style.ringSegments,
                              style.ringFrontBias, view, ringHalfWidth);
            }
            // The free ring always faces the viewer, so it is built in the plane perpendicular to
            // the view direction rather than to an axis.
            AddFullRingUVE(mesh, view, style.freeRingColor, style.freeRingRadius, style.ringSegments,
                           freeRingHalfWidth);
            break;

        case EditorGizmoModeUVE::Scale:
            AddCenterCubeUVE(mesh, style);
            for (const GizmoAxisUVE& axis : axes) {
                AddLineUVE(mesh, axis.direction * style.scaleShaftStart,
                           axis.direction * style.scaleShaftEnd, axis.color, style.axisLineWidthPx);
                AddSolidCubeUVE(mesh, axis.direction * style.scaleShaftEnd, style.scaleBoxSize,
                                axis.color, style.cubeEdgeWidthPx);
            }
            AddScalePlaneHandlesUVE(mesh, style);
            break;

        case EditorGizmoModeUVE::Universal:
            AddCenterCubeUVE(mesh, style);
            for (const GizmoAxisUVE& axis : axes) {
                // rotate: smallest radius, closest to the pivot
                AddRingArcUVE(mesh, axis.direction, axis.color, style.universalRingRadius,
                              style.ringSegments, style.ringFrontBias, view, universalRingHalfWidth);
                // move: reaches well out past the ring
                AddMoveArrowUVE(mesh, axis, style, style.universalShaftStart, style.universalShaftEnd,
                                style.universalConeLength, style.universalConeRadius,
                                style.universalLineWidthPx);
                // scale: sits clear of the arrow tip, further out again
                AddSolidCubeUVE(mesh, axis.direction * style.universalScaleBoxOffset,
                                style.universalScaleBoxSize, axis.color, style.cubeEdgeWidthPx);
            }
            break;
    }
    return mesh;
}

} // namespace UVE::Editor
