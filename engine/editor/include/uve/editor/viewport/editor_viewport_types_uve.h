// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

/// Which projection the editor viewport camera builds. This is a real projection switch, not a
/// label: EditorViewportCameraUVE::ProjectionMatrixUVE() returns a genuinely different matrix per
/// mode (see its doc comment), so orthographic really does drop perspective convergence.
enum class EditorViewportProjectionModeUVE : std::uint8_t {
    Perspective,
    Orthographic,
};

/// Viewport shading mode - the "Lit / Unlit / Wireframe" set an editor viewport header normally
/// exposes. Carried as viewport state here; the renderer consumes it.
enum class EditorViewportDisplayModeUVE : std::uint8_t {
    Normal,
    Wireframe,
    Unshaded,
};

/// The six axis-aligned standard views plus the free user view. Snapping to an axis view is a
/// camera orientation change only - it never touches document entities or history.
enum class EditorStandardViewUVE : std::uint8_t {
    User,
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
};

/// The active transform-handle family. `Select` is the plain pick tool that draws no handles at
/// all; `Universal` is the all-in-one widget that carries rotate, move, and scale on every axis
/// at once.
enum class EditorGizmoModeUVE : std::uint8_t {
    Select,
    Move,
    Rotate,
    Scale,
    Universal,
};

[[nodiscard]] constexpr const char* GetProjectionModeNameUVE(
    const EditorViewportProjectionModeUVE mode) noexcept {
    return mode == EditorViewportProjectionModeUVE::Perspective ? "Perspective" : "Orthographic";
}

[[nodiscard]] constexpr const char* GetDisplayModeNameUVE(
    const EditorViewportDisplayModeUVE mode) noexcept {
    switch (mode) {
        case EditorViewportDisplayModeUVE::Normal:
            return "Lit";
        case EditorViewportDisplayModeUVE::Wireframe:
            return "Wireframe";
        case EditorViewportDisplayModeUVE::Unshaded:
            return "Unlit";
    }
    return "Unknown";
}

[[nodiscard]] constexpr const char* GetStandardViewNameUVE(const EditorStandardViewUVE view) noexcept {
    switch (view) {
        case EditorStandardViewUVE::User:
            return "User";
        case EditorStandardViewUVE::Top:
            return "Top";
        case EditorStandardViewUVE::Bottom:
            return "Bottom";
        case EditorStandardViewUVE::Front:
            return "Front";
        case EditorStandardViewUVE::Rear:
            return "Rear";
        case EditorStandardViewUVE::Left:
            return "Left";
        case EditorStandardViewUVE::Right:
            return "Right";
    }
    return "Unknown";
}

[[nodiscard]] constexpr const char* GetGizmoModeNameUVE(const EditorGizmoModeUVE mode) noexcept {
    switch (mode) {
        case EditorGizmoModeUVE::Select:
            return "Select";
        case EditorGizmoModeUVE::Move:
            return "Move";
        case EditorGizmoModeUVE::Rotate:
            return "Rotate";
        case EditorGizmoModeUVE::Scale:
            return "Scale";
        case EditorGizmoModeUVE::Universal:
            return "Universal";
    }
    return "Unknown";
}

/// The world direction the camera sits along for each standard view - it looks back at the pivot
/// from here. Y-up, right-handed, matching this engine's world basis. `User` has no canonical
/// direction; it returns the Front direction so callers never read an uninitialized value.
[[nodiscard]] constexpr Math::Vector3UVE GetStandardViewDirectionUVE(
    const EditorStandardViewUVE view) noexcept {
    switch (view) {
        case EditorStandardViewUVE::Top:
            return Math::Vector3UVE{0.0F, 1.0F, 0.0F};
        case EditorStandardViewUVE::Bottom:
            return Math::Vector3UVE{0.0F, -1.0F, 0.0F};
        case EditorStandardViewUVE::Front:
            return Math::Vector3UVE{0.0F, 0.0F, 1.0F};
        case EditorStandardViewUVE::Rear:
            return Math::Vector3UVE{0.0F, 0.0F, -1.0F};
        case EditorStandardViewUVE::Right:
            return Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        case EditorStandardViewUVE::Left:
            return Math::Vector3UVE{-1.0F, 0.0F, 0.0F};
        case EditorStandardViewUVE::User:
            break;
    }
    return Math::Vector3UVE{0.0F, 0.0F, 1.0F};
}

/// Editor-only viewport display state: which projection, which shading, and which overlays are
/// switched on. Pure presentation - none of this is scene data, none of it is serialized into a
/// `.uvescene`, and none of it enters the undo/redo history.
struct EditorViewportSettingsUVE final {
    EditorViewportProjectionModeUVE projection = EditorViewportProjectionModeUVE::Perspective;
    EditorViewportDisplayModeUVE display = EditorViewportDisplayModeUVE::Normal;
    EditorStandardViewUVE standardView = EditorStandardViewUVE::User;

    /// Snapping to an axis-aligned view switches to orthographic on its own, and orbiting away
    /// from one switches back. An axis view in perspective is almost never what "Front" means.
    bool autoOrthographic = true;

    bool showGrid = true;
    bool showNavGizmo = true;
    bool showTransformGizmo = true;

    void CycleDisplayModeUVE() noexcept {
        switch (display) {
            case EditorViewportDisplayModeUVE::Normal:
                display = EditorViewportDisplayModeUVE::Wireframe;
                return;
            case EditorViewportDisplayModeUVE::Wireframe:
                display = EditorViewportDisplayModeUVE::Unshaded;
                return;
            case EditorViewportDisplayModeUVE::Unshaded:
                display = EditorViewportDisplayModeUVE::Normal;
                return;
        }
    }

    void ToggleProjectionUVE() noexcept {
        projection = (projection == EditorViewportProjectionModeUVE::Perspective)
                         ? EditorViewportProjectionModeUVE::Orthographic
                         : EditorViewportProjectionModeUVE::Perspective;
    }
};

} // namespace UVE::Editor
