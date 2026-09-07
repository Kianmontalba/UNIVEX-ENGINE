// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

#include "imgui.h"
#include "uve/editor/viewport/editor_gizmo_geometry_uve.h"
#include "uve/editor/viewport/editor_gizmo_style_uve.h"
#include "uve/editor/viewport/editor_viewport_types_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"

/// Module-private viewport chrome: the vector-drawn toolbar icon set and the projection path that
/// puts a built gizmo mesh onto an ImGui draw list. Kept out of editor_uve.cpp, which is already
/// long, and out of the public headers, which never see an ImGui type.
namespace UVE::Editor::ViewportWidgets {

/// The viewport toolbar's icon set. These are stroked from primitives rather than taken from the
/// icon font: the subsetted Tabler face carries only the thirteen menu/panel glyphs, and drawing
/// them here guarantees every icon in the row is authored on the same grid at the same optical
/// weight - which a mix of font glyphs and texture icons had not been.
enum class ToolIconUVE : std::uint8_t {
    Select,
    Move,
    Rotate,
    Scale,
    Universal,
    Grid,
    Snap,
    Camera,
    Sun,
    Environment,
    Menu,
    Eye,
};

/// Every icon is drawn inside a square of `size` centred on `center`, inset by a shared padding
/// so all twelve share one optical bounding box and one stroke weight. `size` is the icon box, not
/// the button box - the caller sizes the button.
void DrawToolIconUVE(ImDrawList& drawList, ToolIconUVE icon, ImVec2 center, float size,
                     ImU32 color);

/// One toolbar button: a `size` x `size` square carrying `icon`, with hover/active states drawn
/// from the editor theme. Returns true when clicked. `active` draws the pressed/selected state,
/// which is how the current transform tool is indicated.
[[nodiscard]] bool ToolIconButtonUVE(const char* id, ToolIconUVE icon, bool active, float size,
                                     const char* tooltip);

/// Where the viewport's 3D content is on screen, in ImGui screen coordinates, plus the matrices
/// needed to project world points into it. Plain data, filled once per frame by the panel.
struct ViewportProjectionUVE final {
    Math::Matrix4x4UVE viewProjection{};
    Math::Matrix4x4UVE view{};
    ImVec2 origin{};
    ImVec2 size{};
};

/// Projects a world point into ImGui screen coordinates. Returns false when the point is behind
/// the camera or the projection is degenerate, so callers drop the primitive rather than drawing a
/// mirrored ghost of it in front of the viewer.
[[nodiscard]] bool ProjectToScreenUVE(const ViewportProjectionUVE& projection,
                                      const Math::Vector3UVE& worldPoint, ImVec2& outScreen);

/// Draws a built gizmo mesh onto `drawList`, placing it at `pivotWorld` and scaling it so the
/// widget keeps a constant on-screen radius. Triangles are painter-sorted back-to-front by view
/// depth and drawn before the strokes, matching how the widget composites over the scene: it hides
/// its own back faces without being hidden by the object it acts on.
///
/// `unitScale` is the world size of one gizmo unit, which the caller derives from the desired
/// pixel radius; see EditorGizmoStyleUVE::gizmoPixelRadius.
void DrawGizmoMeshUVE(ImDrawList& drawList, const EditorGizmoMeshUVE& mesh,
                      const ViewportProjectionUVE& projection, const Math::Vector3UVE& pivotWorld,
                      float unitScale);

/// Draws a nav-gizmo mesh into the square corner widget. The nav widget has its own orthographic
/// framing rather than the scene projection: `viewRotation` supplies only the camera's rotation,
/// so near balls are never drawn larger than far ones.
void DrawNavGizmoMeshUVE(ImDrawList& drawList, const EditorGizmoMeshUVE& mesh,
                         const Math::Matrix4x4UVE& viewRotation, ImVec2 widgetOrigin,
                         float widgetSizePx, float halfExtent);

} // namespace UVE::Editor::ViewportWidgets
