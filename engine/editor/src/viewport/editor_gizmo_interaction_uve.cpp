// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/viewport/editor_gizmo_interaction_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace UVE::Editor {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;

struct PickAxisUVE final {
    EditorTransformAxisUVE axis;
    Math::Vector3UVE direction;
};

[[nodiscard]] std::array<PickAxisUVE, 3> PickAxesUVE() {
    return {{
        {EditorTransformAxisUVE::X, Math::Vector3UVE{1.0F, 0.0F, 0.0F}},
        {EditorTransformAxisUVE::Y, Math::Vector3UVE{0.0F, 1.0F, 0.0F}},
        {EditorTransformAxisUVE::Z, Math::Vector3UVE{0.0F, 0.0F, 1.0F}},
    }};
}

/// Shortest distance from `point` to the segment `[a, b]`, in the same units as the inputs. A
/// degenerate segment collapses to the distance from its single endpoint.
[[nodiscard]] float DistanceToSegmentUVE(const Math::Vector2UVE point, const Math::Vector2UVE a,
                                         const Math::Vector2UVE b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float lengthSquared = (dx * dx) + (dy * dy);
    if (lengthSquared < 1e-8F) {
        const float px = point.x - a.x;
        const float py = point.y - a.y;
        return std::sqrt((px * px) + (py * py));
    }
    const float t =
        std::clamp((((point.x - a.x) * dx) + ((point.y - a.y) * dy)) / lengthSquared, 0.0F, 1.0F);
    const float closestX = a.x + (dx * t);
    const float closestY = a.y + (dy * t);
    const float ox = point.x - closestX;
    const float oy = point.y - closestY;
    return std::sqrt((ox * ox) + (oy * oy));
}

/// Two unit vectors perpendicular to `axis` and to each other, matching the basis the ring geometry
/// is built on so picking tests the ring that is actually drawn.
void PerpBasisUVE(const Math::Vector3UVE& axis, Math::Vector3UVE& outU, Math::Vector3UVE& outV) {
    const Math::Vector3UVE normalizedAxis = Math::NormalizeUVE(axis);
    const Math::Vector3UVE helper = (std::fabs(normalizedAxis.y) < 0.98F)
                                        ? Math::Vector3UVE{0.0F, 1.0F, 0.0F}
                                        : Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    outU = Math::NormalizeUVE(Math::CrossUVE(helper, normalizedAxis));
    outV = Math::NormalizeUVE(Math::CrossUVE(normalizedAxis, outU));
}

/// Closest approach, in pixels, from `pointer` to the projected rim of a ring of `radius` gizmo
/// units about `axis`. Sampled rather than solved: the projection of a circle is an ellipse whose
/// closest point has no cheap closed form, and at this sample count the error is far below the pick
/// tolerance.
[[nodiscard]] float DistanceToProjectedRingUVE(const EditorViewportProjectionUVE& projection,
                                               const Math::Vector3UVE& pivotWorld,
                                               const Math::Vector3UVE& axis, const float radius,
                                               const float unitScale, const Math::Vector2UVE pointer) {
    Math::Vector3UVE u{};
    Math::Vector3UVE v{};
    PerpBasisUVE(axis, u, v);

    constexpr int kSampleCountUVE = 64;
    float best = std::numeric_limits<float>::max();
    Math::Vector2UVE previous{};
    bool hasPrevious = false;
    for (int index = 0; index <= kSampleCountUVE; ++index) {
        const float t = (2.0F * kPiUVE * static_cast<float>(index)) / static_cast<float>(kSampleCountUVE);
        const Math::Vector3UVE world =
            pivotWorld + ((u * (std::cos(t) * radius)) + (v * (std::sin(t) * radius))) * unitScale;
        Math::Vector2UVE screen{};
        if (!ProjectWorldPointUVE(projection, world, screen)) {
            hasPrevious = false;
            continue;
        }
        if (hasPrevious) {
            best = std::min(best, DistanceToSegmentUVE(pointer, previous, screen));
        }
        previous = screen;
        hasPrevious = true;
    }
    return best;
}

} // namespace

bool ProjectWorldPointUVE(const EditorViewportProjectionUVE& projection,
                          const Math::Vector3UVE& worldPoint, Math::Vector2UVE& outScreenPoint) {
    const Math::Matrix4x4UVE& viewProjection = projection.viewProjection;
    const float clipX = (viewProjection.m[0][0] * worldPoint.x) + (viewProjection.m[0][1] * worldPoint.y) +
                        (viewProjection.m[0][2] * worldPoint.z) + viewProjection.m[0][3];
    const float clipY = (viewProjection.m[1][0] * worldPoint.x) + (viewProjection.m[1][1] * worldPoint.y) +
                        (viewProjection.m[1][2] * worldPoint.z) + viewProjection.m[1][3];
    const float clipW = (viewProjection.m[3][0] * worldPoint.x) + (viewProjection.m[3][1] * worldPoint.y) +
                        (viewProjection.m[3][2] * worldPoint.z) + viewProjection.m[3][3];
    if (!(clipW > 1e-6F) || !std::isfinite(clipX) || !std::isfinite(clipY)) {
        return false;
    }

    // NDC is Y-up; pointer and screen space are Y-down, so the vertical axis flips here.
    const float ndcX = clipX / clipW;
    const float ndcY = clipY / clipW;
    outScreenPoint = Math::Vector2UVE{projection.origin.x + (((ndcX * 0.5F) + 0.5F) * projection.size.x),
                                      projection.origin.y +
                                          ((1.0F - ((ndcY * 0.5F) + 0.5F)) * projection.size.y)};
    return true;
}

EditorGizmoHandleHitUVE PickGizmoHandleUVE(const EditorGizmoModeUVE mode,
                                           const EditorGizmoStyleUVE& style,
                                           const EditorViewportProjectionUVE& projection,
                                           const Math::Vector3UVE& pivotWorld, const float unitScale,
                                           const Math::Vector2UVE pointer,
                                           const float pickRadiusPixels) {
    EditorGizmoHandleHitUVE best{};
    if (mode == EditorGizmoModeUVE::Select || !(unitScale > 0.0F) || !(pickRadiusPixels > 0.0F)) {
        return best;
    }

    Math::Vector2UVE pivotScreen{};
    if (!ProjectWorldPointUVE(projection, pivotWorld, pivotScreen)) {
        return best;
    }

    float bestDistance = pickRadiusPixels;
    const auto consider = [&best, &bestDistance](const EditorGizmoHandleKindUVE kind,
                                                 const EditorTransformAxisUVE axis,
                                                 const float distance) {
        if (distance < bestDistance) {
            bestDistance = distance;
            best.kind = kind;
            best.axis = axis;
            best.distancePixels = distance;
        }
    };

    // Rotation rings are tested first and translate/scale shafts after, so that where a ring
    // crosses a shaft the shaft wins on an equal distance - the shafts are the more frequently used
    // handles, and a ring is still reachable everywhere else along its rim.
    if (mode == EditorGizmoModeUVE::Rotate || mode == EditorGizmoModeUVE::Universal) {
        const float ringRadius = (mode == EditorGizmoModeUVE::Universal) ? style.universalRingRadius
                                                                         : style.ringRadius;
        for (const PickAxisUVE& entry : PickAxesUVE()) {
            consider(EditorGizmoHandleKindUVE::RotateAxis, entry.axis,
                     DistanceToProjectedRingUVE(projection, pivotWorld, entry.direction, ringRadius,
                                                unitScale, pointer));
        }
    }

    if (mode == EditorGizmoModeUVE::Move || mode == EditorGizmoModeUVE::Universal) {
        const float shaftStart =
            (mode == EditorGizmoModeUVE::Universal) ? style.universalShaftStart : style.moveShaftStart;
        const float shaftEnd = (mode == EditorGizmoModeUVE::Universal)
                                   ? style.universalShaftEnd + style.universalConeLength
                                   : style.moveShaftEnd + style.moveConeLength;
        for (const PickAxisUVE& entry : PickAxesUVE()) {
            Math::Vector2UVE startScreen{};
            Math::Vector2UVE endScreen{};
            if (!ProjectWorldPointUVE(projection, pivotWorld + (entry.direction * (shaftStart * unitScale)),
                                      startScreen) ||
                !ProjectWorldPointUVE(projection, pivotWorld + (entry.direction * (shaftEnd * unitScale)),
                                      endScreen)) {
                continue;
            }
            consider(EditorGizmoHandleKindUVE::TranslateAxis, entry.axis,
                     DistanceToSegmentUVE(pointer, startScreen, endScreen));
        }
    }

    if (mode == EditorGizmoModeUVE::Scale || mode == EditorGizmoModeUVE::Universal) {
        // Universal carries its scale cubes past the arrow tips, so only the cube itself is a scale
        // target there - the shaft below it already belongs to the move handle.
        const float shaftStart = (mode == EditorGizmoModeUVE::Universal)
                                     ? style.universalScaleBoxOffset - (style.universalScaleBoxSize * 0.5F)
                                     : style.scaleShaftStart;
        const float shaftEnd = (mode == EditorGizmoModeUVE::Universal)
                                   ? style.universalScaleBoxOffset + (style.universalScaleBoxSize * 0.5F)
                                   : style.scaleShaftEnd + (style.scaleBoxSize * 0.5F);
        for (const PickAxisUVE& entry : PickAxesUVE()) {
            Math::Vector2UVE startScreen{};
            Math::Vector2UVE endScreen{};
            if (!ProjectWorldPointUVE(projection, pivotWorld + (entry.direction * (shaftStart * unitScale)),
                                      startScreen) ||
                !ProjectWorldPointUVE(projection, pivotWorld + (entry.direction * (shaftEnd * unitScale)),
                                      endScreen)) {
                continue;
            }
            consider(EditorGizmoHandleKindUVE::ScaleAxis, entry.axis,
                     DistanceToSegmentUVE(pointer, startScreen, endScreen));
        }
    }

    return best;
}

bool ComputeAxisDragDistanceUVE(const EditorViewportProjectionUVE& projection,
                                const Math::Vector3UVE& pivotWorld,
                                const Math::Vector3UVE& axisDirection, const float unitScale,
                                const Math::Vector2UVE startPointer,
                                const Math::Vector2UVE currentPointer, float& outWorldDistance) {
    if (!(unitScale > 0.0F)) {
        return false;
    }

    Math::Vector2UVE pivotScreen{};
    Math::Vector2UVE axisTipScreen{};
    if (!ProjectWorldPointUVE(projection, pivotWorld, pivotScreen) ||
        !ProjectWorldPointUVE(projection, pivotWorld + (axisDirection * unitScale), axisTipScreen)) {
        return false;
    }

    const float axisScreenX = axisTipScreen.x - pivotScreen.x;
    const float axisScreenY = axisTipScreen.y - pivotScreen.y;
    const float axisScreenLength = std::sqrt((axisScreenX * axisScreenX) + (axisScreenY * axisScreenY));
    // Below a few pixels the axis is nearly end-on and its screen direction carries almost no
    // information: resolving a drag onto it would amplify pointer noise into a huge world distance.
    // Refusing is what makes an edge-on handle sit still rather than fling the object away.
    constexpr float kMinimumAxisScreenLengthUVE = 4.0F;
    if (!(axisScreenLength > kMinimumAxisScreenLengthUVE)) {
        return false;
    }

    const float unitX = axisScreenX / axisScreenLength;
    const float unitY = axisScreenY / axisScreenLength;
    const float travelX = currentPointer.x - startPointer.x;
    const float travelY = currentPointer.y - startPointer.y;
    const float travelAlongAxisPixels = (travelX * unitX) + (travelY * unitY);

    // One gizmo unit spans axisScreenLength pixels, and one gizmo unit is unitScale world units.
    outWorldDistance = (travelAlongAxisPixels / axisScreenLength) * unitScale;
    return std::isfinite(outWorldDistance);
}

bool ComputeAxisDragAngleUVE(const EditorViewportProjectionUVE& projection,
                             const Math::Vector3UVE& pivotWorld, const Math::Vector3UVE& axisDirection,
                             const Math::Vector3UVE& cameraPosition, const Math::Vector2UVE startPointer,
                             const Math::Vector2UVE currentPointer, float& outRadians) {
    Math::Vector2UVE pivotScreen{};
    if (!ProjectWorldPointUVE(projection, pivotWorld, pivotScreen)) {
        return false;
    }

    const float startX = startPointer.x - pivotScreen.x;
    const float startY = startPointer.y - pivotScreen.y;
    const float currentX = currentPointer.x - pivotScreen.x;
    const float currentY = currentPointer.y - pivotScreen.y;
    constexpr float kMinimumRadiusPixelsUVE = 6.0F;
    if (((startX * startX) + (startY * startY)) < (kMinimumRadiusPixelsUVE * kMinimumRadiusPixelsUVE) ||
        ((currentX * currentX) + (currentY * currentY)) <
            (kMinimumRadiusPixelsUVE * kMinimumRadiusPixelsUVE)) {
        return false;
    }

    // Screen space is Y-down, so a positive atan2 difference here is a clockwise sweep on screen.
    const float sweep = std::atan2(currentY, currentX) - std::atan2(startY, startX);
    float wrapped = std::fmod(sweep + kPiUVE, 2.0F * kPiUVE);
    if (wrapped < 0.0F) {
        wrapped += 2.0F * kPiUVE;
    }
    wrapped -= kPiUVE;

    // An axis pointing toward the viewer must turn the object the same way the cursor sweeps, and
    // one pointing away must turn it the other way - otherwise the far side of a ring drags
    // backwards under the cursor.
    const Math::Vector3UVE towardCamera = Math::NormalizeUVE(cameraPosition - pivotWorld);
    const float facing = Math::DotUVE(Math::NormalizeUVE(axisDirection), towardCamera);
    outRadians = (facing >= 0.0F) ? -wrapped : wrapped;
    return std::isfinite(outRadians);
}

} // namespace UVE::Editor
