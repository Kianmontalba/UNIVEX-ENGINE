//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "uve/core/engine_services_uve.h"
#include "uve/math/ray_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Editor {

/// EditorStateUVE is the lifecycle state of one editor session. The editor owns session data only;
/// EngineCoreUVE remains the owner of the ECS, renderer, window, and every engine service.
enum class EditorStateUVE {
    Uninitialized,
    Running,
    Shutdown,
};

/// A screen-space rectangle used by the editor's transparent viewport overlay. Coordinates are in
/// the native desktop window's ImGui/GLFW pixel space, with an origin at the top-left. Value type;
/// safe to copy across editor helper calls.
struct EditorViewportRectUVE final {
    Math::Vector2UVE origin{};
    Math::Vector2UVE size{};
};

/// World-space axes supported by EditorUVE's first transform-gizmo slice. Rotation, scale, planar
/// handles, snapping, local axes, and multi-selection are intentionally deferred.
enum class EditorTranslateAxisUVE {
    None,
    X,
    Y,
    Z,
};

/// EditorUVE composes the existing engine services into a first editor foundation: an editor-owned
/// camera, deterministic hierarchy and collider-backed viewport selection, a transform inspector
/// mutation path, a translate-only world-space gizmo, scene-document save/load, and an editor-private
/// Dear ImGui overlay. No Dear ImGui type appears in this public interface, so the UI backend remains
/// an implementation detail of engine/editor.
///
/// The supplied EngineServicesUVE reference must remain valid from InitUVE() through ShutdownUVE().
/// EditorUVE is main-thread only, matching the scene, render, and window services it composes.
class EditorUVE final {
public:
    explicit EditorUVE(Core::EngineServicesUVE& services,
                       std::filesystem::path activeScenePath = "editor_scene.uvescene");
    ~EditorUVE();

    EditorUVE(const EditorUVE&) = delete;
    EditorUVE& operator=(const EditorUVE&) = delete;

    /// Creates the non-document editor camera and initializes the private UI backend when a real
    /// native window is available. Safe in headless mode: hierarchy, inspector, picking helpers,
    /// transform editing, and persistence logic remain usable while UI rendering is disabled.
    void InitUVE();

    /// Validates a possibly deleted selection, cancels an invalid gizmo drag, and performs
    /// non-rendering per-frame maintenance.
    void TickUVE();

    /// Draws the private editor overlay. EngineCoreUVE invokes this from its post-render callback
    /// before PresentUVE(), after the HDR scene has passed through the standard tone-mapping path.
    void RenderOverlayUVE();

    /// Saves every document root except the editor camera to the active .uvescene path. Dirty state
    /// is cleared only after the scene serializer reports success.
    [[nodiscard]] bool SaveSceneUVE();

    /// Replaces the editable document scene with the active .uvescene file. A backup scene is
    /// created before destructive mutation and restored if deserialization fails; the editor camera
    /// remains outside the document root set.
    [[nodiscard]] bool LoadSceneUVE();

    /// Makes entity the sole hierarchy/inspector selection when it is live; invalid or deleted
    /// handles clear the selection instead of exposing stale ECS state.
    void SelectEntityUVE(Scene::EntityUVE entity) noexcept;
    void ClearSelectionUVE() noexcept;

    /// Applies one validated local transform through ISceneGraphUVE, ensuring derived world
    /// transforms are marked dirty for EngineCoreUVE's next scene-graph update. Returns false for
    /// invalid/deleted/non-transform entities or non-finite transform values.
    [[nodiscard]] bool SetSelectedLocalTransformUVE(const Scene::TransformComponentUVE& transform);

    /// Creates a normalized world-space ray from a pointer inside viewportRect. Uses the editor
    /// camera's derived world transform and perspective settings. Returns std::nullopt for invalid
    /// editor state, camera data, viewport geometry, or pointer coordinates outside the rectangle.
    [[nodiscard]] std::optional<Math::RayUVE> MakeViewportRayUVE(const EditorViewportRectUVE& viewportRect,
                                                                   Math::Vector2UVE pointerPosition) const;

    /// Uses the existing deterministic box-collider raycast system to select the closest live
    /// document entity under pointerPosition. A valid viewport miss clears selection. Entities
    /// without ColliderComponentUVE are intentionally not selectable in this first picking slice.
    [[nodiscard]] bool PickViewportUVE(const EditorViewportRectUVE& viewportRect,
                                        Math::Vector2UVE pointerPosition);

    /// Moves the selected document entity by a finite world-space distance along one unit world
    /// axis. Parent world rotation and scale are converted back to a local position delta before
    /// applying the existing scene-graph transform path. Returns false without mutation if the
    /// entity, parent transform, axis, or distance is invalid.
    [[nodiscard]] bool TranslateSelectedAlongAxisUVE(EditorTranslateAxisUVE axis, float worldDistance);

    [[nodiscard]] std::vector<Scene::EntityUVE> GetDocumentRootsUVE();
    [[nodiscard]] EditorStateUVE GetStateUVE() const noexcept;
    [[nodiscard]] Scene::EntityUVE GetSelectedEntityUVE() const noexcept;
    [[nodiscard]] Scene::EntityUVE GetViewportCameraUVE() const noexcept;
    [[nodiscard]] bool IsSceneDirtyUVE() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetActiveScenePathUVE() const noexcept;
    void SetActiveScenePathUVE(std::filesystem::path path);

    /// Releases editor-private UI resources and destroys the editor camera while the services are
    /// still alive. Idempotent after the first successful shutdown.
    void ShutdownUVE();

private:
    struct GizmoDragUVE final {
        EditorTranslateAxisUVE axis = EditorTranslateAxisUVE::None;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::TransformComponentUVE initialLocalTransform{};
        Math::Vector2UVE initialPointer{};
        Math::Vector2UVE screenAxisDirection{};
        float pixelsPerWorldUnit = 0.0F;
    };

    [[nodiscard]] bool IsDocumentEntityUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] bool IsTransformFiniteUVE(const Scene::TransformComponentUVE& transform) const noexcept;
    [[nodiscard]] bool IsViewportRectValidUVE(const EditorViewportRectUVE& viewportRect) const noexcept;
    [[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept;
    [[nodiscard]] Math::Vector3UVE GetAxisVectorUVE(EditorTranslateAxisUVE axis) const noexcept;
    [[nodiscard]] bool ProjectWorldPointUVE(const EditorViewportRectUVE& viewportRect,
                                             const Math::Vector3UVE& worldPoint,
                                             Math::Vector2UVE& outScreenPoint) const;
    [[nodiscard]] bool ComputeLocalDeltaForWorldDeltaUVE(Scene::EntityUVE entity,
                                                           const Math::Vector3UVE& worldDelta,
                                                           Math::Vector3UVE& outLocalDelta) const;
    [[nodiscard]] bool BeginGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                          Math::Vector2UVE pointerPosition);
    void UpdateGizmoDragUVE(Math::Vector2UVE pointerPosition);
    void CancelGizmoDragUVE() noexcept;
    void DrawTranslateGizmoUVE(const EditorViewportRectUVE& viewportRect);
    void DestroyDocumentSubtreeUVE(Scene::EntityUVE root);
    void ClearDocumentSceneUVE();
    void DrawHierarchyPanelUVE();
    void DrawHierarchyNodeUVE(Scene::EntityUVE entity);
    void DrawInspectorPanelUVE();
    void DrawViewportPanelUVE();

    Core::EngineServicesUVE* m_services = nullptr;
    EditorStateUVE m_state = EditorStateUVE::Uninitialized;
    Scene::EntityUVE m_viewportCamera = Scene::kInvalidEntityUVE;
    Scene::EntityUVE m_selectedEntity = Scene::kInvalidEntityUVE;
    std::filesystem::path m_activeScenePath;
    GizmoDragUVE m_gizmoDrag{};
    bool m_sceneDirty = false;
    bool m_uiInitialized = false;
};

} // namespace UVE::Editor
