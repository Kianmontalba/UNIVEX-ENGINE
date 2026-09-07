// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "viewport/editor_viewport_widgets_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "uve/editor/viewport/editor_nav_gizmo_uve.h"

namespace UVE::Editor::ViewportWidgets {

namespace {

constexpr float kPiUVE = std::numbers::pi_v<float>;

/// One stroke weight and one inset for the whole icon set, so every icon in the toolbar row reads
/// at the same optical weight and fills the same box.
constexpr float kIconStrokeUVE = 1.6F;
constexpr float kIconInsetRatioUVE = 0.18F;

[[nodiscard]] ImU32 ScaleAlphaUVE(const ImU32 color, const float factor) {
    const ImU32 alpha = (color >> IM_COL32_A_SHIFT) & 0xFFU;
    const auto scaled = static_cast<ImU32>(std::clamp(static_cast<float>(alpha) * factor, 0.0F, 255.0F));
    return (color & ~(0xFFU << IM_COL32_A_SHIFT)) | (scaled << IM_COL32_A_SHIFT);
}

void StrokeArrowUVE(ImDrawList& drawList, const ImVec2 from, const ImVec2 to, const float headSize,
                    const ImU32 color) {
    drawList.AddLine(from, to, color, kIconStrokeUVE);
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt((dx * dx) + (dy * dy));
    if (length < 1e-4F) {
        return;
    }
    const float ux = dx / length;
    const float uy = dy / length;
    const ImVec2 left{to.x - (ux * headSize) - (uy * headSize * 0.55F),
                      to.y - (uy * headSize) + (ux * headSize * 0.55F)};
    const ImVec2 right{to.x - (ux * headSize) + (uy * headSize * 0.55F),
                       to.y - (uy * headSize) - (ux * headSize * 0.55F)};
    drawList.AddTriangleFilled(to, left, right, color);
}

/// An ellipse stroked as a closed polyline - ImGui has no ellipse primitive in every build, and a
/// polyline keeps the ring the same weight as every other stroke in the set.
void StrokeEllipseUVE(ImDrawList& drawList, const ImVec2 center, const float radiusX,
                      const float radiusY, const ImU32 color, const int segments = 28) {
    std::vector<ImVec2> points;
    points.reserve(static_cast<std::size_t>(segments));
    for (int index = 0; index < segments; ++index) {
        const float t = (2.0F * kPiUVE * static_cast<float>(index)) / static_cast<float>(segments);
        points.push_back(ImVec2{center.x + (std::cos(t) * radiusX), center.y + (std::sin(t) * radiusY)});
    }
    drawList.AddPolyline(points.data(), segments, color, ImDrawFlags_Closed, kIconStrokeUVE);
}

} // namespace

void DrawToolIconUVE(ImDrawList& drawList, const ToolIconUVE icon, const ImVec2 center,
                     const float size, const ImU32 color) {
    const float half = (size * 0.5F) * (1.0F - kIconInsetRatioUVE);
    const ImVec2 topLeft{center.x - half, center.y - half};
    const ImVec2 bottomRight{center.x + half, center.y + half};

    switch (icon) {
        case ToolIconUVE::Select: {
            // A pointer arrow, drawn as a filled wedge with a tail.
            const ImVec2 tip{center.x - (half * 0.45F), center.y - half};
            const ImVec2 wing{center.x - (half * 0.45F), center.y + (half * 0.55F)};
            const ImVec2 barb{center.x + (half * 0.55F), center.y + (half * 0.05F)};
            drawList.AddTriangleFilled(tip, wing, barb, color);
            drawList.AddLine(ImVec2{center.x - (half * 0.05F), center.y + (half * 0.25F)},
                             ImVec2{center.x + (half * 0.35F), center.y + half}, color,
                             kIconStrokeUVE * 1.4F);
            break;
        }
        case ToolIconUVE::Move: {
            StrokeArrowUVE(drawList, center, ImVec2{center.x, topLeft.y}, half * 0.36F, color);
            StrokeArrowUVE(drawList, center, ImVec2{center.x, bottomRight.y}, half * 0.36F, color);
            StrokeArrowUVE(drawList, center, ImVec2{topLeft.x, center.y}, half * 0.36F, color);
            StrokeArrowUVE(drawList, center, ImVec2{bottomRight.x, center.y}, half * 0.36F, color);
            break;
        }
        case ToolIconUVE::Rotate: {
            // An open arc with an arrow head, so it reads as rotation rather than as a ring.
            drawList.PathArcTo(center, half * 0.82F, kPiUVE * 0.75F, kPiUVE * 2.35F, 24);
            drawList.PathStroke(color, ImDrawFlags_None, kIconStrokeUVE);
            const ImVec2 headAt{center.x + (std::cos(kPiUVE * 2.35F) * half * 0.82F),
                                center.y + (std::sin(kPiUVE * 2.35F) * half * 0.82F)};
            StrokeArrowUVE(drawList, ImVec2{headAt.x - (half * 0.22F), headAt.y - (half * 0.30F)},
                           headAt, half * 0.34F, color);
            break;
        }
        case ToolIconUVE::Scale: {
            drawList.AddLine(ImVec2{topLeft.x + (half * 0.15F), bottomRight.y - (half * 0.15F)},
                             ImVec2{bottomRight.x - (half * 0.25F), topLeft.y + (half * 0.25F)}, color,
                             kIconStrokeUVE);
            const float boxSize = half * 0.42F;
            drawList.AddRectFilled(ImVec2{bottomRight.x - boxSize, topLeft.y},
                                   ImVec2{bottomRight.x, topLeft.y + boxSize}, color, 1.0F);
            drawList.AddRect(ImVec2{topLeft.x, bottomRight.y - (boxSize * 0.72F)},
                             ImVec2{topLeft.x + (boxSize * 0.72F), bottomRight.y}, color, 1.0F,
                             ImDrawFlags_None, kIconStrokeUVE);
            break;
        }
        case ToolIconUVE::Universal: {
            // The all-in-one tool: a ring with axis stubs, echoing the widget it activates.
            StrokeEllipseUVE(drawList, center, half * 0.46F, half * 0.46F, color, 20);
            drawList.AddLine(center, ImVec2{center.x, topLeft.y}, color, kIconStrokeUVE);
            drawList.AddLine(center, ImVec2{bottomRight.x, center.y}, color, kIconStrokeUVE);
            drawList.AddLine(center, ImVec2{topLeft.x + (half * 0.18F), bottomRight.y - (half * 0.18F)},
                             color, kIconStrokeUVE);
            break;
        }
        case ToolIconUVE::Grid: {
            for (int step = 0; step <= 3; ++step) {
                const float t = static_cast<float>(step) / 3.0F;
                const float x = topLeft.x + ((bottomRight.x - topLeft.x) * t);
                const float y = topLeft.y + ((bottomRight.y - topLeft.y) * t);
                drawList.AddLine(ImVec2{x, topLeft.y}, ImVec2{x, bottomRight.y}, color, 1.0F);
                drawList.AddLine(ImVec2{topLeft.x, y}, ImVec2{bottomRight.x, y}, color, 1.0F);
            }
            break;
        }
        case ToolIconUVE::Snap: {
            // A magnet: two prongs under a bridge.
            drawList.PathArcTo(ImVec2{center.x, center.y + (half * 0.10F)}, half * 0.62F, kPiUVE,
                               2.0F * kPiUVE, 20);
            drawList.PathStroke(color, ImDrawFlags_None, kIconStrokeUVE * 1.5F);
            drawList.AddLine(ImVec2{center.x - (half * 0.62F), center.y + (half * 0.10F)},
                             ImVec2{center.x - (half * 0.62F), bottomRight.y}, color,
                             kIconStrokeUVE * 1.5F);
            drawList.AddLine(ImVec2{center.x + (half * 0.62F), center.y + (half * 0.10F)},
                             ImVec2{center.x + (half * 0.62F), bottomRight.y}, color,
                             kIconStrokeUVE * 1.5F);
            break;
        }
        case ToolIconUVE::Camera: {
            drawList.AddRect(ImVec2{topLeft.x, center.y - (half * 0.45F)},
                             ImVec2{center.x + (half * 0.30F), center.y + (half * 0.50F)}, color, 2.0F,
                             ImDrawFlags_None, kIconStrokeUVE);
            const ImVec2 lensTop{bottomRight.x, center.y - (half * 0.45F)};
            const ImVec2 lensBottom{bottomRight.x, center.y + (half * 0.50F)};
            drawList.AddTriangleFilled(ImVec2{center.x + (half * 0.30F), center.y}, lensTop, lensBottom,
                                       color);
            break;
        }
        case ToolIconUVE::Sun: {
            drawList.AddCircleFilled(center, half * 0.42F, color, 20);
            for (int ray = 0; ray < 8; ++ray) {
                const float t = (2.0F * kPiUVE * static_cast<float>(ray)) / 8.0F;
                const float cosT = std::cos(t);
                const float sinT = std::sin(t);
                drawList.AddLine(ImVec2{center.x + (cosT * half * 0.66F), center.y + (sinT * half * 0.66F)},
                                 ImVec2{center.x + (cosT * half), center.y + (sinT * half)}, color,
                                 kIconStrokeUVE);
            }
            break;
        }
        case ToolIconUVE::Environment: {
            // A globe: outline plus one meridian and one parallel.
            StrokeEllipseUVE(drawList, center, half * 0.88F, half * 0.88F, color, 24);
            StrokeEllipseUVE(drawList, center, half * 0.36F, half * 0.88F, color, 24);
            drawList.AddLine(ImVec2{center.x - (half * 0.88F), center.y},
                             ImVec2{center.x + (half * 0.88F), center.y}, color, kIconStrokeUVE);
            break;
        }
        case ToolIconUVE::Menu: {
            for (int line = 0; line < 3; ++line) {
                const float y = center.y + ((static_cast<float>(line) - 1.0F) * half * 0.62F);
                drawList.AddLine(ImVec2{topLeft.x, y}, ImVec2{bottomRight.x, y}, color,
                                 kIconStrokeUVE * 1.2F);
            }
            break;
        }
        case ToolIconUVE::Eye: {
            drawList.PathArcTo(ImVec2{center.x, center.y + (half * 0.62F)}, half * 1.05F,
                               kPiUVE * 1.22F, kPiUVE * 1.78F, 20);
            drawList.PathStroke(color, ImDrawFlags_None, kIconStrokeUVE);
            drawList.PathArcTo(ImVec2{center.x, center.y - (half * 0.62F)}, half * 1.05F,
                               kPiUVE * 0.22F, kPiUVE * 0.78F, 20);
            drawList.PathStroke(color, ImDrawFlags_None, kIconStrokeUVE);
            drawList.AddCircleFilled(center, half * 0.28F, color, 16);
            break;
        }
    }
}

bool ToolIconButtonUVE(const char* const id, const ToolIconUVE icon, const bool active,
                       const float size, const char* const tooltip) {
    ImGui::PushID(id);
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##tool", ImVec2{size, size});
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const ImVec2 center{cursor.x + (size * 0.5F), cursor.y + (size * 0.5F)};
    if (active || hovered) {
        const ImU32 background = ImGui::GetColorU32(active ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        drawList->AddRectFilled(cursor, ImVec2{cursor.x + size, cursor.y + size}, background, 4.0F);
    }
    const ImU32 tint = ImGui::GetColorU32(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    DrawToolIconUVE(*drawList, icon, center, size, active ? tint : ScaleAlphaUVE(tint, hovered ? 1.0F : 0.85F));

    if (hovered && tooltip != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return clicked;
}

bool ProjectToScreenUVE(const ViewportProjectionUVE& projection, const Math::Vector3UVE& worldPoint,
                        ImVec2& outScreen) {
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

    // NDC is Y-up; ImGui screen space is Y-down, so the vertical axis flips here.
    const float ndcX = clipX / clipW;
    const float ndcY = clipY / clipW;
    outScreen = ImVec2{projection.origin.x + (((ndcX * 0.5F) + 0.5F) * projection.size.x),
                       projection.origin.y + ((1.0F - ((ndcY * 0.5F) + 0.5F)) * projection.size.y)};
    return true;
}

namespace {

/// View-space depth of a world point, used only to painter-sort gizmo triangles. The camera looks
/// down -Z, so a more negative value is further away.
[[nodiscard]] float ViewDepthUVE(const Math::Matrix4x4UVE& view, const Math::Vector3UVE& point) {
    return (view.m[2][0] * point.x) + (view.m[2][1] * point.y) + (view.m[2][2] * point.z) +
           view.m[2][3];
}

[[nodiscard]] ImU32 ToColorUVE(const Math::Vector3UVE& color, const float alpha) {
    const auto channel = [](const float value) {
        return static_cast<int>(std::clamp(value, 0.0F, 1.0F) * 255.0F + 0.5F);
    };
    return IM_COL32(channel(color.x), channel(color.y), channel(color.z),
                    channel(alpha));
}

} // namespace

void DrawGizmoMeshUVE(ImDrawList& drawList, const EditorGizmoMeshUVE& mesh,
                      const ViewportProjectionUVE& projection, const Math::Vector3UVE& pivotWorld,
                      const float unitScale) {
    const auto toWorld = [&pivotWorld, unitScale](const Math::Vector3UVE& local) {
        return pivotWorld + (local * unitScale);
    };

    // Triangles first, painter-sorted back-to-front: the widget hides its own back faces while
    // still compositing over the object it acts on, because a handle buried inside the thing it
    // moves is useless.
    struct SortedTriangleUVE final {
        std::size_t index = 0U;
        float depth = 0.0F;
    };
    std::vector<SortedTriangleUVE> order;
    order.reserve(mesh.triangles.size());
    for (std::size_t index = 0U; index < mesh.triangles.size(); ++index) {
        const EditorGizmoTriangleUVE& triangle = mesh.triangles[index];
        const Math::Vector3UVE centroid =
            (toWorld(triangle.a) + toWorld(triangle.b) + toWorld(triangle.c)) * (1.0F / 3.0F);
        order.push_back(SortedTriangleUVE{index, ViewDepthUVE(projection.view, centroid)});
    }
    std::sort(order.begin(), order.end(),
              [](const SortedTriangleUVE& lhs, const SortedTriangleUVE& rhs) {
                  return lhs.depth < rhs.depth;
              });

    for (const SortedTriangleUVE& entry : order) {
        const EditorGizmoTriangleUVE& triangle = mesh.triangles[entry.index];
        ImVec2 a{};
        ImVec2 b{};
        ImVec2 c{};
        if (!ProjectToScreenUVE(projection, toWorld(triangle.a), a) ||
            !ProjectToScreenUVE(projection, toWorld(triangle.b), b) ||
            !ProjectToScreenUVE(projection, toWorld(triangle.c), c)) {
            continue;
        }
        drawList.AddTriangleFilled(a, b, c, ToColorUVE(triangle.color, triangle.alpha));
    }

    for (const EditorGizmoLineUVE& line : mesh.lines) {
        ImVec2 a{};
        ImVec2 b{};
        if (!ProjectToScreenUVE(projection, toWorld(line.a), a) ||
            !ProjectToScreenUVE(projection, toWorld(line.b), b)) {
            continue;
        }
        drawList.AddLine(a, b, ToColorUVE(line.color, 1.0F), line.widthPx);
    }
}

void DrawNavGizmoMeshUVE(ImDrawList& drawList, const EditorGizmoMeshUVE& mesh,
                         const Math::Matrix4x4UVE& viewRotation, const ImVec2 widgetOrigin,
                         const float widgetSizePx, const float halfExtent) {
    if (!(widgetSizePx > 0.0F) || !(halfExtent > 0.0F)) {
        return;
    }
    const float pixelsPerUnit = (widgetSizePx * 0.5F) / halfExtent;
    const ImVec2 center{widgetOrigin.x + (widgetSizePx * 0.5F), widgetOrigin.y + (widgetSizePx * 0.5F)};

    // The nav widget is always orthographic: only the camera's rotation is applied, so a near ball
    // is never drawn larger than a far one.
    const auto project = [&](const Math::Vector3UVE& local) {
        const Math::Vector3UVE viewSpace = TransformDirectionUVE(viewRotation, local);
        return ImVec2{center.x + (viewSpace.x * pixelsPerUnit),
                      center.y - (viewSpace.y * pixelsPerUnit)};
    };

    for (const EditorGizmoTriangleUVE& triangle : mesh.triangles) {
        drawList.AddTriangleFilled(project(triangle.a), project(triangle.b), project(triangle.c),
                                   ToColorUVE(triangle.color, triangle.alpha));
    }
    for (const EditorGizmoLineUVE& line : mesh.lines) {
        drawList.AddLine(project(line.a), project(line.b), ToColorUVE(line.color, 1.0F), line.widthPx);
    }
}

} // namespace UVE::Editor::ViewportWidgets
