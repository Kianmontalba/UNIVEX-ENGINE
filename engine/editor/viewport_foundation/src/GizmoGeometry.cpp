#include "univex/gizmo/GizmoGeometry.h"

#include <array>
#include <cmath>
#include <numbers>

namespace univex::gizmo {

using univex::math::Cross;
using univex::math::Dot;
using univex::math::Normalize;

namespace {

constexpr float kPi = std::numbers::pi_v<float>;

struct Axis {
    Vec3 direction;
    Vec3 color;
};

std::array<Axis, 3> AxesOf(const GizmoStyle& style) {
    return {{
        {Vec3{1.f, 0.f, 0.f}, style.axisColorX},
        {Vec3{0.f, 1.f, 0.f}, style.axisColorY},
        {Vec3{0.f, 0.f, 1.f}, style.axisColorZ},
    }};
}

// Two unit vectors perpendicular to `axis` and to each other.
void PerpBasis(const Vec3& axis, Vec3& outU, Vec3& outV) {
    const Vec3 a = Normalize(axis);
    const Vec3 helper = (std::fabs(a.y) < 0.98f) ? Vec3{0.f, 1.f, 0.f} : Vec3{1.f, 0.f, 0.f};
    outU = Normalize(Cross(helper, a));
    outV = Normalize(Cross(a, outU));
}

std::vector<Vec3> CirclePoints(const Vec3& center, const Vec3& u, const Vec3& v,
                               float radius, int segments) {
    std::vector<Vec3> points;
    points.reserve(static_cast<std::size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        const float t = (2.f * kPi * static_cast<float>(i)) / static_cast<float>(segments);
        points.push_back(center + u * (std::cos(t) * radius) + v * (std::sin(t) * radius));
    }
    return points;
}

void AddLine(GizmoMesh& mesh, const Vec3& a, const Vec3& b, const Vec3& color, float widthPx) {
    mesh.lines.push_back(GizmoLine{a, b, color, widthPx});
}

void AddTriangle(GizmoMesh& mesh, const Vec3& a, const Vec3& b, const Vec3& c,
                 const Vec3& color, float alpha) {
    mesh.triangles.push_back(GizmoTriangle{a, b, c, color, alpha});
}

void AddQuad(GizmoMesh& mesh, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3,
             const Vec3& color, float alpha) {
    AddTriangle(mesh, p0, p1, p2, color, alpha);
    AddTriangle(mesh, p0, p2, p3, color, alpha);
}

// A solid, axis-aligned cube: six filled faces plus twelve darker edges, so
// the handle reads as a body with a defined silhouette rather than a wire
// box that disappears against the grid.
void AddSolidCube(GizmoMesh& mesh, const Vec3& center, float size,
                  const Vec3& color, float edgeWidthPx) {
    const float h = size * 0.5f;
    struct Face { Vec3 n, u, v; };
    const std::array<Face, 6> faces = {{
        {{ 1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
        {{-1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
        {{ 0, 1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{ 0,-1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{ 0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
        {{ 0, 0,-1}, {1, 0, 0}, {0, 1, 0}},
    }};
    for (const Face& f : faces) {
        const Vec3 c = center + f.n * h;
        AddQuad(mesh,
                c + f.u * -h + f.v * -h,
                c + f.u *  h + f.v * -h,
                c + f.u *  h + f.v *  h,
                c + f.u * -h + f.v *  h,
                color, 1.f);
    }

    const Vec3 edgeColor = color * 0.55f;
    for (int axis = 0; axis < 3; ++axis) {
        for (int i = 0; i < 4; ++i) {
            Vec3 a = center, b = center;
            const float s0 = (i & 1) ? h : -h;
            const float s1 = (i & 2) ? h : -h;
            const int a1 = (axis + 1) % 3;
            const int a2 = (axis + 2) % 3;
            float* pa[3] = {&a.x, &a.y, &a.z};
            float* pb[3] = {&b.x, &b.y, &b.z};
            *pa[a1] += s0; *pb[a1] += s0;
            *pa[a2] += s1; *pb[a2] += s1;
            *pa[axis] -= h; *pb[axis] += h;
            AddLine(mesh, a, b, edgeColor, edgeWidthPx);
        }
    }
}

// Arrow head: a cone of real triangles, capped so it stays solid when seen
// from behind.
void AddCone(GizmoMesh& mesh, const Vec3& baseCenter, const Vec3& axis,
             float length, float radius, int segments, const Vec3& color) {
    Vec3 u, v;
    PerpBasis(axis, u, v);
    const Vec3 tip = baseCenter + axis * length;
    const auto ring = CirclePoints(baseCenter, u, v, radius, segments);
    for (std::size_t i = 0; i + 1 < ring.size(); ++i) {
        AddTriangle(mesh, tip, ring[i], ring[i + 1], color, 1.f);
        AddTriangle(mesh, baseCenter, ring[i + 1], ring[i], color, 1.f);
    }
}

void AddMoveArrow(GizmoMesh& mesh, const Axis& axis, const GizmoStyle& style,
                  float shaftStart, float shaftEnd, float coneLength, float coneRadius,
                  float lineWidthPx) {
    AddLine(mesh, axis.direction * shaftStart, axis.direction * shaftEnd,
            axis.color, lineWidthPx);
    AddCone(mesh, axis.direction * shaftEnd, axis.direction,
            coneLength, coneRadius, style.moveConeSegments, axis.color);
}

// A ring drawn as a solid annulus: two concentric circles joined by quads.
// Seamless where a stroked polyline would show its joints.
//
// `keepSegment` decides which parts of the ring survive — that is where the
// near-side arc selection happens, so the same routine serves both the
// camera-facing arcs and the full free-rotation ring.
template <typename KeepFn>
void AddAnnulus(GizmoMesh& mesh, const Vec3& axis, const Vec3& color, float radius,
                float halfWidth, int segments, KeepFn keepSegment) {
    Vec3 u, v;
    PerpBasis(axis, u, v);
    const auto inner = CirclePoints(Vec3{0.f, 0.f, 0.f}, u, v, radius - halfWidth, segments);
    const auto outer = CirclePoints(Vec3{0.f, 0.f, 0.f}, u, v, radius + halfWidth, segments);
    const auto mid = CirclePoints(Vec3{0.f, 0.f, 0.f}, u, v, radius, segments);

    for (std::size_t i = 0; i + 1 < mid.size(); ++i) {
        if (!keepSegment(mid[i], mid[i + 1])) continue;
        AddTriangle(mesh, inner[i], outer[i], outer[i + 1], color, 1.f);
        AddTriangle(mesh, inner[i], outer[i + 1], inner[i + 1], color, 1.f);
    }
}

// Near-side arc only: a segment survives when both of its endpoints face the
// camera. On a ring centred at the pivot that is exactly the front half, and
// three full circles drawn over each other would read as a ball of spaghetti.
void AddRingArc(GizmoMesh& mesh, const Vec3& axis, const Vec3& color, float radius,
                int segments, float frontBias, const Vec3& viewDirection, float halfWidth) {
    AddAnnulus(mesh, axis, color, radius, halfWidth, segments,
               [&](const Vec3& a, const Vec3& b) {
                   return Dot(a, viewDirection) < -frontBias && Dot(b, viewDirection) < -frontBias;
               });
}

void AddFullRing(GizmoMesh& mesh, const Vec3& axis, const Vec3& color, float radius,
                 int segments, float halfWidth) {
    AddAnnulus(mesh, axis, color, radius, halfWidth, segments,
               [](const Vec3&, const Vec3&) { return true; });
}

void AddMovePlaneHandles(GizmoMesh& mesh, const GizmoStyle& style) {
    const auto axes = AxesOf(style);
    const std::array<std::pair<int, int>, 3> pairs = {{{0, 1}, {1, 2}, {2, 0}}};
    for (const auto& [i, j] : pairs) {
        const Vec3 a = axes[static_cast<std::size_t>(i)].direction;
        const Vec3 b = axes[static_cast<std::size_t>(j)].direction;
        const float o = style.planeHandleOffset;
        const float s = style.planeHandleSize;
        const Vec3 p0 = a * o + b * o;
        const Vec3 p1 = a * (o + s) + b * o;
        const Vec3 p2 = a * (o + s) + b * (o + s);
        const Vec3 p3 = a * o + b * (o + s);
        AddQuad(mesh, p0, p1, p2, p3, style.planeColor, style.planeHandleAlpha);
        AddLine(mesh, p0, p1, style.planeColor, 1.1f);
        AddLine(mesh, p1, p2, style.planeColor, 1.1f);
        AddLine(mesh, p2, p3, style.planeColor, 1.1f);
        AddLine(mesh, p3, p0, style.planeColor, 1.1f);
    }
}

void AddScalePlaneHandles(GizmoMesh& mesh, const GizmoStyle& style) {
    const auto axes = AxesOf(style);
    const std::array<std::pair<int, int>, 3> pairs = {{{0, 1}, {1, 2}, {2, 0}}};
    for (const auto& [i, j] : pairs) {
        const Vec3 a = axes[static_cast<std::size_t>(i)].direction;
        const Vec3 b = axes[static_cast<std::size_t>(j)].direction;
        const Vec3 p0 = a * style.scalePlaneOffset;
        const Vec3 p1 = b * style.scalePlaneOffset;
        const Vec3 p2 = (a + b) * style.scalePlanePull;
        AddTriangle(mesh, p0, p1, p2, style.planeColor, style.planeHandleAlpha);
        AddLine(mesh, p0, p1, style.planeColor, 1.1f);
    }
}

// A small solid block at the pivot. It used to be a wire box, but twelve
// edges a few pixels long read as scribble rather than as a cube.
void AddCenterCube(GizmoMesh& mesh, const GizmoStyle& style) {
    AddSolidCube(mesh, Vec3{0.f, 0.f, 0.f}, style.centerCubeSize,
                 style.centerColor, style.centerCubeWidthPx);
}

} // namespace

const char* GizmoModeName(GizmoMode mode) {
    switch (mode) {
        case GizmoMode::Select:    return "Select";
        case GizmoMode::Move:      return "Move";
        case GizmoMode::Rotate:    return "Rotate";
        case GizmoMode::Scale:     return "Scale";
        case GizmoMode::Universal: return "Universal";
    }
    return "Unknown";
}

GizmoMesh BuildGizmoMesh(GizmoMode mode, const GizmoStyle& style, const Vec3& viewDirection,
                         float unitsPerPixel) {
    GizmoMesh mesh;
    const auto axes = AxesOf(style);
    const Vec3 view = Normalize(viewDirection);

    // Ring strokes are solid geometry, so their pixel widths have to be
    // converted into gizmo units up front.
    const float scale = (unitsPerPixel > 0.f) ? unitsPerPixel : 1.f;
    const float ringHalfWidth = style.ringLineWidthPx * 0.5f * scale;
    const float freeRingHalfWidth = style.freeRingWidthPx * 0.5f * scale;
    const float universalRingHalfWidth = style.universalRingWidthPx * 0.5f * scale;

    switch (mode) {
        case GizmoMode::Select:
            AddCenterCube(mesh, style);
            break;

        case GizmoMode::Move:
            AddCenterCube(mesh, style);
            for (const Axis& axis : axes) {
                AddMoveArrow(mesh, axis, style,
                             style.moveShaftStart, style.moveShaftEnd,
                             style.moveConeLength, style.moveConeRadius,
                             style.axisLineWidthPx);
            }
            AddMovePlaneHandles(mesh, style);
            break;

        case GizmoMode::Rotate:
            AddCenterCube(mesh, style);
            for (const Axis& axis : axes) {
                AddRingArc(mesh, axis.direction, axis.color, style.ringRadius,
                           style.ringSegments, style.ringFrontBias, view, ringHalfWidth);
            }
            // The free ring always faces the viewer, so it is built in the
            // plane perpendicular to the view direction rather than to an axis.
            AddFullRing(mesh, view, style.freeRingColor, style.freeRingRadius,
                        style.ringSegments, freeRingHalfWidth);
            break;

        case GizmoMode::Scale:
            AddCenterCube(mesh, style);
            for (const Axis& axis : axes) {
                AddLine(mesh, axis.direction * style.scaleShaftStart,
                        axis.direction * style.scaleShaftEnd, axis.color, style.axisLineWidthPx);
                AddSolidCube(mesh, axis.direction * style.scaleShaftEnd, style.scaleBoxSize,
                             axis.color, style.cubeEdgeWidthPx);
            }
            AddScalePlaneHandles(mesh, style);
            break;

        case GizmoMode::Universal:
            AddCenterCube(mesh, style);
            for (const Axis& axis : axes) {
                // rotate: smallest radius, closest to the pivot
                AddRingArc(mesh, axis.direction, axis.color, style.universalRingRadius,
                           style.ringSegments, style.ringFrontBias, view, universalRingHalfWidth);
                // move: reaches well out past the ring
                AddMoveArrow(mesh, axis, style,
                             style.universalShaftStart, style.universalShaftEnd,
                             style.universalConeLength, style.universalConeRadius,
                             style.universalLineWidthPx);
                // scale: sits clear of the arrow tip, further out again
                AddSolidCube(mesh, axis.direction * style.universalScaleBoxOffset,
                             style.universalScaleBoxSize, axis.color, style.cubeEdgeWidthPx);
            }
            break;
    }
    return mesh;
}

} // namespace univex::gizmo
