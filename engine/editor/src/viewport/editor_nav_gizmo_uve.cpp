// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/viewport/editor_nav_gizmo_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <vector>

namespace UVE::Editor {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;

void PerpBasisUVE(const Math::Vector3UVE& axis, Math::Vector3UVE& outU, Math::Vector3UVE& outV) {
    const Math::Vector3UVE normalizedAxis = Math::NormalizeUVE(axis);
    const Math::Vector3UVE helper = (std::fabs(normalizedAxis.y) < 0.98F)
                                        ? Math::Vector3UVE{0.0F, 1.0F, 0.0F}
                                        : Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    outU = Math::NormalizeUVE(Math::CrossUVE(helper, normalizedAxis));
    outV = Math::NormalizeUVE(Math::CrossUVE(normalizedAxis, outU));
}

/// A disc that always faces the camera, built from real triangles.
void AddFacingDiscUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& center, const float radius,
                      const Math::Vector3UVE& viewDirection, const Math::Vector3UVE& color,
                      const float alpha, const int segments) {
    Math::Vector3UVE u{};
    Math::Vector3UVE v{};
    PerpBasisUVE(viewDirection, u, v);
    Math::Vector3UVE previous = center + (u * radius);
    for (int index = 1; index <= segments; ++index) {
        const float t = (2.0F * kPiUVE * static_cast<float>(index)) / static_cast<float>(segments);
        const Math::Vector3UVE current =
            center + (u * (std::cos(t) * radius)) + (v * (std::sin(t) * radius));
        mesh.triangles.push_back(EditorGizmoTriangleUVE{center, previous, current, color, alpha});
        previous = current;
    }
}

/// A hollow ball is a coloured disc with a smaller dark disc laid on top, rather than a stroked
/// circle: stroking a small circle from independent per-segment quads leaves gear teeth wherever
/// the chord is shorter than the stroke is wide. Two discs are seamless at any size.
void AddFacingAnnulusUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& center,
                         const float outerRadius, const float innerRadius,
                         const Math::Vector3UVE& viewDirection, const Math::Vector3UVE& ringColor,
                         const Math::Vector3UVE& holeColor, const int segments) {
    // Nudge the hole a hair toward the camera so it always wins the tie when both discs are
    // coplanar and depth testing is off.
    const Math::Vector3UVE towardCamera = viewDirection * -0.002F;
    AddFacingDiscUVE(mesh, center, outerRadius, viewDirection, ringColor, 1.0F, segments);
    AddFacingDiscUVE(mesh, center + towardCamera, innerRadius, viewDirection, holeColor, 1.0F,
                     segments);
}

/// X, Y and Z drawn as vector strokes rather than from a font - three glyphs are not worth a
/// texture atlas or a font dependency, and routing them through the line pass means they inherit
/// its analytic anti-aliasing. Each glyph is defined in a unit box centred on the origin, then
/// placed on the screen-facing basis so it always reads upright.
void AddAxisLabelUVE(EditorGizmoMeshUVE& mesh, const Math::Vector3UVE& center,
                     const Math::Vector3UVE& right, const Math::Vector3UVE& up, const char letter,
                     const float halfSize, const Math::Vector3UVE& color, const float widthPx) {
    const auto place = [&center, &right, &up, halfSize](const float x, const float y) {
        return center + (right * (x * halfSize)) + (up * (y * halfSize));
    };
    const auto stroke = [&mesh, &place, &color, widthPx](const float x0, const float y0,
                                                         const float x1, const float y1) {
        mesh.lines.push_back(EditorGizmoLineUVE{place(x0, y0), place(x1, y1), color, widthPx});
    };

    switch (letter) {
        case 'X':
            stroke(-0.62F, 1.0F, 0.62F, -1.0F);
            stroke(-0.62F, -1.0F, 0.62F, 1.0F);
            break;
        case 'Y':
            stroke(-0.62F, 1.0F, 0.0F, 0.0F);
            stroke(0.62F, 1.0F, 0.0F, 0.0F);
            stroke(0.0F, 0.0F, 0.0F, -1.0F);
            break;
        case 'Z':
            stroke(-0.62F, 1.0F, 0.62F, 1.0F);
            stroke(0.62F, 1.0F, -0.62F, -1.0F);
            stroke(-0.62F, -1.0F, 0.62F, -1.0F);
            break;
        default:
            break;
    }
}

/// Screen-aligned basis for the nav gizmo's own camera, so labels stay upright however the view
/// is orbited. Looking straight up or down, world-up is parallel to forward and the cross product
/// collapses, so a different reference axis is picked in that case.
void ScreenBasisUVE(const Math::Vector3UVE& viewDirection, Math::Vector3UVE& outRight,
                    Math::Vector3UVE& outUp) {
    const Math::Vector3UVE forward = Math::NormalizeUVE(viewDirection);
    Math::Vector3UVE worldUp{0.0F, 1.0F, 0.0F};
    if (std::fabs(Math::DotUVE(forward, worldUp)) > 0.999F) {
        worldUp = Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    }
    outRight = Math::NormalizeUVE(Math::CrossUVE(forward, worldUp));
    outUp = Math::NormalizeUVE(Math::CrossUVE(outRight, forward));
}

} // namespace

std::array<EditorNavHandleUVE, 6> GetNavHandlesUVE(const EditorGizmoStyleUVE& style) {
    return {{
        {{1.0F, 0.0F, 0.0F}, style.axisColorX, true, 'X'},
        {{-1.0F, 0.0F, 0.0F}, style.axisColorX, false, 'X'},
        {{0.0F, 1.0F, 0.0F}, style.axisColorY, true, 'Y'},
        {{0.0F, -1.0F, 0.0F}, style.axisColorY, false, 'Y'},
        {{0.0F, 0.0F, 1.0F}, style.axisColorZ, true, 'Z'},
        {{0.0F, 0.0F, -1.0F}, style.axisColorZ, false, 'Z'},
    }};
}

float GetNavViewHalfExtentUVE(const EditorGizmoStyleUVE& style) noexcept {
    // One unit out to each ball centre, plus its radius, plus a little air.
    return 1.0F + style.navBallRadius + 0.12F;
}

EditorNavGizmoMeshesUVE BuildNavGizmoMeshesUVE(const EditorGizmoStyleUVE& style,
                                               const Math::Vector3UVE& viewDirection) {
    EditorNavGizmoMeshesUVE meshes;
    EditorGizmoMeshUVE& mesh = meshes.overlay;
    const Math::Vector3UVE view = Math::NormalizeUVE(viewDirection);
    const std::array<EditorNavHandleUVE, 6> handles = GetNavHandlesUVE(style);

    // Axis stubs go in the underlay, so the balls sit on top of them. One stub per axis, drawn
    // full length in both directions.
    for (const EditorNavHandleUVE& handle : handles) {
        if (!handle.positive) {
            continue;
        }
        meshes.underlay.lines.push_back(EditorGizmoLineUVE{handle.direction * -1.0F, handle.direction,
                                                           handle.color * 0.75F,
                                                           style.navAxisLineWidthPx});
    }

    // Balls, drawn back-to-front so the near ones cover the far ones. The nav viewport has no
    // depth buffer worth relying on, and a painter's sort over six discs is exact.
    std::vector<const EditorNavHandleUVE*> sorted;
    sorted.reserve(handles.size());
    for (const EditorNavHandleUVE& handle : handles) {
        sorted.push_back(&handle);
    }
    std::sort(sorted.begin(), sorted.end(),
              [&view](const EditorNavHandleUVE* lhs, const EditorNavHandleUVE* rhs) {
                  // Most negative dot == nearest, so it sorts last and is drawn last.
                  return Math::DotUVE(lhs->direction, view) > Math::DotUVE(rhs->direction, view);
              });

    Math::Vector3UVE right{};
    Math::Vector3UVE up{};
    ScreenBasisUVE(view, right, up);

    for (const EditorNavHandleUVE* handle : sorted) {
        const Math::Vector3UVE center = handle->direction;
        if (handle->positive) {
            AddFacingDiscUVE(mesh, center, style.navBallRadius, view, handle->color, 1.0F,
                             style.navBallSegments);
            // Only the positive ends are labelled: a letter in the hollow negative rings as well
            // doubles the clutter without adding anything, since the ring already says which end
            // it is.
            AddAxisLabelUVE(mesh, center + (view * -0.01F), right, up, handle->axisLabel,
                            style.navBallRadius * style.navLabelScale, style.navLabelColor,
                            style.navLabelWidthPx);
        } else {
            AddFacingAnnulusUVE(mesh, center, style.navBallRadius, style.navBallRadius * 0.62F, view,
                                handle->color, style.navHollowFillColor, style.navBallSegments);
        }
    }
    return meshes;
}

EditorNavPickResultUVE PickNavGizmoUVE(const EditorGizmoStyleUVE& style,
                                       const Math::Matrix4x4UVE& viewRotation, const float localX,
                                       const float localY, const float viewportSizePx) {
    EditorNavPickResultUVE result;
    if (!(viewportSizePx > 0.0F)) {
        return result;
    }

    const float halfExtent = GetNavViewHalfExtentUVE(style);
    const float pixelsPerUnit = (viewportSizePx * 0.5F) / halfExtent;
    const float centerPx = viewportSizePx * 0.5F;
    const float ballRadiusPx = style.navBallRadius * pixelsPerUnit;

    float bestDepth = -std::numeric_limits<float>::max();
    for (const EditorNavHandleUVE& handle : GetNavHandlesUVE(style)) {
        const Math::Vector3UVE viewSpace = TransformDirectionUVE(viewRotation, handle.direction);
        const float screenX = centerPx + (viewSpace.x * pixelsPerUnit);
        const float screenY = centerPx - (viewSpace.y * pixelsPerUnit);
        const float dx = localX - screenX;
        const float dy = localY - screenY;
        if (((dx * dx) + (dy * dy)) > (ballRadiusPx * ballRadiusPx)) {
            continue;
        }

        // The camera looks down -Z in view space, so the largest z is nearest.
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

} // namespace UVE::Editor
