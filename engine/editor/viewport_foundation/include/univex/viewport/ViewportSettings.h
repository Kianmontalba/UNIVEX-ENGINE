// univex/viewport/ViewportSettings.h
// -----------------------------------------------------------------------
// The viewport's display state — the set of modes an editor viewport menu
// normally exposes: which projection, which shading, and which overlays
// are switched on.
//
// This is plain state plus keyboard bindings, not a menu widget. Drawing
// an actual dropdown needs a UI toolkit (ImGui or the engine's own), which
// this module deliberately does not pull in; the modes themselves are all
// live and switchable, and the demo reports the current state in its
// window title.
// -----------------------------------------------------------------------
#pragma once

namespace univex::viewport {

enum class ProjectionMode {
    Perspective,
    Orthographic,
};

enum class DisplayMode {
    Normal,     // per-face flat colours
    Wireframe,  // polygon edges only
    Unshaded,   // one flat colour for the whole object
};

// The six axis-aligned views, plus the free user view.
enum class StandardView {
    User,
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
};

[[nodiscard]] constexpr const char* ProjectionModeName(ProjectionMode mode) {
    return mode == ProjectionMode::Perspective ? "Perspective" : "Orthographic";
}

[[nodiscard]] constexpr const char* DisplayModeName(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::Normal:    return "Normal";
        case DisplayMode::Wireframe: return "Wireframe";
        case DisplayMode::Unshaded:  return "Unshaded";
    }
    return "Unknown";
}

[[nodiscard]] constexpr const char* StandardViewName(StandardView view) {
    switch (view) {
        case StandardView::User:   return "User";
        case StandardView::Top:    return "Top";
        case StandardView::Bottom: return "Bottom";
        case StandardView::Front:  return "Front";
        case StandardView::Rear:   return "Rear";
        case StandardView::Left:   return "Left";
        case StandardView::Right:  return "Right";
    }
    return "Unknown";
}

// The world direction the camera should sit along for each standard view
// (the camera looks back at the pivot from here). Y-up, right-handed.
constexpr void StandardViewDirection(StandardView view,
                                                   float& outX, float& outY, float& outZ) {
    switch (view) {
        case StandardView::Top:    outX = 0.f;  outY = 1.f;  outZ = 0.f;  return;
        case StandardView::Bottom: outX = 0.f;  outY = -1.f; outZ = 0.f;  return;
        case StandardView::Front:  outX = 0.f;  outY = 0.f;  outZ = 1.f;  return;
        case StandardView::Rear:   outX = 0.f;  outY = 0.f;  outZ = -1.f; return;
        case StandardView::Right:  outX = 1.f;  outY = 0.f;  outZ = 0.f;  return;
        case StandardView::Left:   outX = -1.f; outY = 0.f;  outZ = 0.f;  return;
        case StandardView::User:   outX = 0.f;  outY = 0.f;  outZ = 1.f;  return;
    }
}

struct ViewportSettings {
    ProjectionMode projection = ProjectionMode::Perspective;
    DisplayMode display = DisplayMode::Normal;
    StandardView standardView = StandardView::User;

    // Snapping to an axis-aligned view switches to orthographic on its own,
    // and orbiting away from one switches back. An axis view in perspective
    // is almost never what someone wants when they press Front.
    bool autoOrthogonal = true;

    bool viewEnvironment = true;      // background gradient
    bool viewGrid = true;             // the infinite ground grid
    bool viewGizmos = true;           // the corner orientation gizmo
    bool viewTransformGizmo = true;   // the move/rotate/scale widget
    bool viewSceneGeometry = true;    // the reference object

    void CycleDisplayMode() {
        switch (display) {
            case DisplayMode::Normal:    display = DisplayMode::Wireframe; break;
            case DisplayMode::Wireframe: display = DisplayMode::Unshaded;  break;
            case DisplayMode::Unshaded:  display = DisplayMode::Normal;    break;
        }
    }

    void ToggleProjection() {
        projection = (projection == ProjectionMode::Perspective) ? ProjectionMode::Orthographic
                                                                 : ProjectionMode::Perspective;
    }
};

} // namespace univex::viewport
