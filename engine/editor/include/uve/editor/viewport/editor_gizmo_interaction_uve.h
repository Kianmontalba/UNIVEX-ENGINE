// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

#include "uve/editor/viewport/editor_gizmo_style_uve.h"
#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Which family of handle a pointer is over. Plane handles and the free-rotation trackball are
/// deliberately absent: they are drawn but not yet draggable, and naming them here would imply a
/// pick result the interaction path cannot service.
enum class EditorGizmoHandleKindUVE : std::uint8_t {
    None,
    TranslateAxis,
    RotateAxis,
    ScaleAxis,
};

/// The handle under the pointer, if any. `distancePixels` is how far the pointer sits from the
/// handle's screen-space centreline, which is what lets the closest of several overlapping handles
/// win rather than whichever happened to be tested first.
struct EditorGizmoHandleHitUVE final {
    EditorGizmoHandleKindUVE kind = EditorGizmoHandleKindUVE::None;
    EditorTransformAxisUVE axis = EditorTransformAxisUVE::None;
    float distancePixels = 0.0F;

    [[nodiscard]] bool IsHitUVE() const noexcept {
        return kind != EditorGizmoHandleKindUVE::None && axis != EditorTransformAxisUVE::None;
    }
};

/// Everything needed to map between world space and the viewport's pixels, in plain math types so
/// the whole interaction path is testable without a UI toolkit. `origin` and `size` are the scene
/// region's top-left corner and extent in the same screen space the pointer is reported in.
struct EditorViewportProjectionUVE final {
    Math::Matrix4x4UVE viewProjection{};
    Math::Vector2UVE origin{};
    Math::Vector2UVE size{};
};

/// Projects a world point into viewport screen coordinates (Y down, matching pointer input).
/// Returns false for points behind the camera or a degenerate projection, so a caller drops the
/// handle rather than drawing or picking a mirrored ghost of it in front of the viewer.
[[nodiscard]] bool ProjectWorldPointUVE(const EditorViewportProjectionUVE& projection,
                                        const Math::Vector3UVE& worldPoint,
                                        Math::Vector2UVE& outScreenPoint);

/// The handle under `pointer`, for the gizmo of `mode` drawn at `pivotWorld` with `unitScale` world
/// units per gizmo unit. Returns a miss for Select mode, which draws no handles.
///
/// Picking is screen-space throughout: an axis handle is hit when the pointer falls within
/// `pickRadiusPixels` of the projected shaft segment, and a rotation ring when the pointer is that
/// close to the projected ring's rim. Doing it in pixels rather than with world-space ray tests is
/// what keeps the grab area a constant size on screen at any camera distance, which is the same
/// reason the widget itself is authored in pixels.
[[nodiscard]] EditorGizmoHandleHitUVE PickGizmoHandleUVE(EditorGizmoModeUVE mode,
                                                         const EditorGizmoStyleUVE& style,
                                                         const EditorViewportProjectionUVE& projection,
                                                         const Math::Vector3UVE& pivotWorld,
                                                         float unitScale, Math::Vector2UVE pointer,
                                                         float pickRadiusPixels = 9.0F);

/// How far along `axis`, in world units, a pointer that has moved from `startPointer` to
/// `currentPointer` should drag the pivot.
///
/// The axis is projected to screen and the pointer travel is resolved onto it, then converted back
/// to world units by the projected axis's own screen length - so dragging tracks the handle under
/// the cursor at any zoom or viewing angle. Returns false when the axis is too foreshortened to
/// resolve a direction (looking straight down it), rather than returning a wildly amplified
/// distance, which is what makes a near-edge-on drag sit still instead of flinging the object away.
[[nodiscard]] bool ComputeAxisDragDistanceUVE(const EditorViewportProjectionUVE& projection,
                                              const Math::Vector3UVE& pivotWorld,
                                              const Math::Vector3UVE& axisDirection, float unitScale,
                                              Math::Vector2UVE startPointer,
                                              Math::Vector2UVE currentPointer, float& outWorldDistance);

/// The signed angle, in radians, that a pointer dragged from `startPointer` to `currentPointer`
/// sweeps around the pivot in the plane whose normal is `axisDirection`.
///
/// The sign follows the axis: turning the pointer clockwise on screen rotates negatively about an
/// axis pointing toward the viewer, so the object turns the way the cursor does. Returns false when
/// the pointer is on top of the pivot, where the angle is undefined.
[[nodiscard]] bool ComputeAxisDragAngleUVE(const EditorViewportProjectionUVE& projection,
                                           const Math::Vector3UVE& pivotWorld,
                                           const Math::Vector3UVE& axisDirection,
                                           const Math::Vector3UVE& cameraPosition,
                                           Math::Vector2UVE startPointer,
                                           Math::Vector2UVE currentPointer, float& outRadians);

} // namespace UVE::Editor
