#include "univex/gizmo/NavGizmo.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace univex::gizmo {

using univex::math::Cross;
using univex::math::Dot;
using univex::math::Normalize;

namespace {

constexpr float kPi = std::numbers::pi_v<float>;

void PerpBasis(const Vec3& axis, Vec3& outU, Vec3& outV) {
    const Vec3 a = Normalize(axis);
    const Vec3 helper = (std::fabs(a.y) < 0.98f) ? Vec3{0.f, 1.f, 0.f} : Vec3{1.f, 0.f, 0.f};
    outU = Normalize(Cross(helper, a));
    outV = Normalize(Cross(a, outU));
}

// A disc that always faces the camera, built from real triangles.
void AddFacingDisc(GizmoMesh& mesh, const Vec3& center, float radius,
                   const Vec3& viewDirection, const Vec3& color, float alpha, int segments) {
    Vec3 u, v;
    PerpBasis(viewDirection, u, v);
    Vec3 previous = center + u * radius;
    for (int i = 1; i <= segments; ++i) {
        const float t = (2.f * kPi * static_cast<float>(i)) / static_cast<float>(segments);
        const Vec3 current = center + u * (std::cos(t) * radius) + v * (std::sin(t) * radius);
        mesh.triangles.push_back(GizmoTriangle{center, previous, current, color, alpha});
        previous = current;
    }
}

// A hollow ball is drawn as a coloured disc with a smaller dark disc laid on
// top, rather than as a stroked circle. Stroking a small circle from
// independent per-segment quads leaves gear teeth wherever the chord is
// shorter than the stroke is wide; two discs are seamless at any size.
void AddFacingAnnulus(GizmoMesh& mesh, const Vec3& center, float outerRadius, float innerRadius,
                      const Vec3& viewDirection, const Vec3& ringColor, const Vec3& holeColor,
                      int segments) {
    // Nudge the hole a hair toward the camera so it always wins the tie when
    // both discs are coplanar and depth testing is off.
    const Vec3 towardCamera = viewDirection * -0.002f;
    AddFacingDisc(mesh, center, outerRadius, viewDirection, ringColor, 1.f, segments);
    AddFacingDisc(mesh, center + towardCamera, innerRadius, viewDirection, holeColor, 1.f, segments);
}

// X, Y and Z drawn as vector strokes rather than from a font. Three glyphs is
// not worth a texture atlas or a font dependency, and going through the line
// pass means the letters inherit its analytic anti-aliasing for free.
//
// Each glyph is defined in a unit box centred on the origin, then placed on
// the screen-facing basis (right, up) so it always reads upright.
void AddAxisLabel(GizmoMesh& mesh, const Vec3& center, const Vec3& right, const Vec3& up,
                  char letter, float halfSize, const Vec3& color, float widthPx) {
    const auto place = [&](float x, float y) {
        return center + right * (x * halfSize) + up * (y * halfSize);
    };
    const auto stroke = [&](float x0, float y0, float x1, float y1) {
        mesh.lines.push_back(GizmoLine{place(x0, y0), place(x1, y1), color, widthPx});
    };

    switch (letter) {
        case 'X':
            stroke(-0.62f,  1.f,  0.62f, -1.f);
            stroke(-0.62f, -1.f,  0.62f,  1.f);
            break;
        case 'Y':
            stroke(-0.62f,  1.f,  0.f,    0.f);
            stroke( 0.62f,  1.f,  0.f,    0.f);
            stroke( 0.f,    0.f,  0.f,   -1.f);
            break;
        case 'Z':
            stroke(-0.62f,  1.f,  0.62f,  1.f);
            stroke( 0.62f,  1.f, -0.62f, -1.f);
            stroke(-0.62f, -1.f,  0.62f, -1.f);
            break;
        default:
            break;
    }
}

// Screen-aligned basis for the nav gizmo's own camera, so labels stay upright
// however the view is orbited.
void ScreenBasis(const Vec3& viewDirection, Vec3& outRight, Vec3& outUp) {
    const Vec3 forward = Normalize(viewDirection);
    Vec3 worldUp{0.f, 1.f, 0.f};
    // Looking straight up or down, world-up is parallel to forward and the
    // cross product collapses; pick a different reference in that case.
    if (std::fabs(Dot(forward, worldUp)) > 0.999f) worldUp = Vec3{0.f, 0.f, 1.f};
    outRight = Normalize(Cross(forward, worldUp));
    outUp = Normalize(Cross(outRight, forward));
}

} // namespace

std::array<NavHandle, 6> NavHandles(const GizmoStyle& style) {
    return {{
        {{ 1.f, 0.f, 0.f}, style.axisColorX, true,  'X'},
        {{-1.f, 0.f, 0.f}, style.axisColorX, false, 'X'},
        {{ 0.f, 1.f, 0.f}, style.axisColorY, true,  'Y'},
        {{ 0.f,-1.f, 0.f}, style.axisColorY, false, 'Y'},
        {{ 0.f, 0.f, 1.f}, style.axisColorZ, true,  'Z'},
        {{ 0.f, 0.f,-1.f}, style.axisColorZ, false, 'Z'},
    }};
}

float NavViewHalfExtent(const GizmoStyle& style) {
    // One unit out to each ball centre, plus its radius, plus a little air.
    return 1.f + style.navBallRadius + 0.12f;
}

NavGizmoMeshes BuildNavGizmoMeshes(const GizmoStyle& style, const Vec3& viewDirection) {
    NavGizmoMeshes meshes;
    GizmoMesh& mesh = meshes.overlay;
    const Vec3 view = Normalize(viewDirection);
    const auto handles = NavHandles(style);

    // Axis stubs go in the underlay, so the balls sit on top of them.
    for (const NavHandle& handle : handles) {
        if (!handle.positive) continue; // one stub per axis, drawn full length both ways
        meshes.underlay.lines.push_back(GizmoLine{handle.direction * -1.f, handle.direction,
                                                  handle.color * 0.75f, style.navAxisLineWidthPx});
    }

    // Balls, drawn back-to-front so the near ones cover the far ones. The
    // nav viewport has no depth buffer of its own worth relying on, and a
    // painter's sort over six discs is exact.
    std::vector<const NavHandle*> sorted;
    sorted.reserve(handles.size());
    for (const NavHandle& handle : handles) sorted.push_back(&handle);
    std::sort(sorted.begin(), sorted.end(), [&](const NavHandle* a, const NavHandle* b) {
        return Dot(a->direction, view) > Dot(b->direction, view); // most negative dot == nearest, drawn last
    });

    Vec3 right, up;
    ScreenBasis(view, right, up);

    for (const NavHandle* handle : sorted) {
        const Vec3 center = handle->direction;
        if (handle->positive) {
            AddFacingDisc(mesh, center, style.navBallRadius, view, handle->color, 1.f,
                          style.navBallSegments);
            // Only the positive ends are labelled: putting a letter in the
            // hollow negative rings as well doubles the clutter without adding
            // anything, since the ring already says which end it is.
            AddAxisLabel(mesh, center + view * -0.01f, right, up, handle->axisLabel,
                         style.navBallRadius * style.navLabelScale,
                         style.navLabelColor, style.navLabelWidthPx);
        } else {
            // Negative ends read as hollow rings, so the two directions of an
            // axis are never confused at a glance.
            AddFacingAnnulus(mesh, center, style.navBallRadius, style.navBallRadius * 0.62f,
                             view, handle->color, Vec3{0.078f, 0.090f, 0.125f},
                             style.navBallSegments);
        }
    }
    return meshes;
}

NavPickResult PickNavGizmo(const GizmoStyle& style,
                           const Mat4& viewRotation,
                           float localX,
                           float localY,
                           float viewportSizePx) {
    NavPickResult result;
    if (viewportSizePx <= 0.f) return result;

    const float halfExtent = NavViewHalfExtent(style);
    const float pixelsPerUnit = (viewportSizePx * 0.5f) / halfExtent;
    const float centerPx = viewportSizePx * 0.5f;
    const float ballRadiusPx = style.navBallRadius * pixelsPerUnit;

    float bestDepth = -1e30f;
    for (const NavHandle& handle : NavHandles(style)) {
        const Vec3 viewSpace = viewRotation.TransformDirection(handle.direction);
        const float screenX = centerPx + viewSpace.x * pixelsPerUnit;
        const float screenY = centerPx - viewSpace.y * pixelsPerUnit;
        const float dx = localX - screenX;
        const float dy = localY - screenY;
        if (dx * dx + dy * dy > ballRadiusPx * ballRadiusPx) continue;

        // Camera looks down -Z in view space, so the largest z is nearest.
        if (viewSpace.z > bestDepth) {
            bestDepth = viewSpace.z;
            result.hit = true;
            result.direction = handle.direction;
            result.axisLabel = handle.axisLabel;
            result.positive = handle.positive;
        }
    }
    return result;
}

} // namespace univex::gizmo
