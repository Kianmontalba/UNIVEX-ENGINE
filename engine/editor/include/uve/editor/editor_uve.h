//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "uve/asset/i_asset_database_uve.h"
#include "uve/core/engine_services_uve.h"
#include "uve/math/ray_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"

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

/// The supported first-pass Scene menu archetypes. Each created entity is a document root with a
/// TransformComponentUVE; specialized kinds add only the named gameplay component.
enum class EditorEntityKindUVE {
    Empty,
    Camera,
    DirectionalLight,
    CollisionBox,
};

/// The active pointer gesture affecting the editor-owned viewport camera. This state never touches
/// document entities, persistence, or command history.
enum class EditorViewportNavigationModeUVE {
    None,
    Orbit,
    Pan,
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
                       std::filesystem::path activeScenePath = "editor_scene.uvescene",
                       std::size_t historyCapacity = 100U);
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

    /// Adds or updates persistent human-readable metadata for the selected live document entity.
    /// Returns false without mutation for invalid editor/selection state, an empty or whitespace-only
    /// name, a name longer than the supported editor-entry limit, or an unchanged value.
    [[nodiscard]] bool SetSelectedEntityNameUVE(std::string name);

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

    /// Moves the editor-only focus point to the selected live document entity's derived world position
    /// and reapplies the current orbit camera state. It never changes selection, dirty state, or history.
    [[nodiscard]] bool FocusSelectedEntityUVE();
    /// Applies finite yaw and pitch deltas in radians to the editor-only orbit camera. Pitch remains
    /// clamped to a safe range around the horizon; document state and history remain unchanged.
    [[nodiscard]] bool OrbitViewportUVE(float yawDeltaRadians, float pitchDeltaRadians);
    /// Pans the editor-only focus point from a finite pixel delta inside a valid viewport rectangle.
    [[nodiscard]] bool PanViewportUVE(Math::Vector2UVE pixelDelta, const EditorViewportRectUVE& viewportRect);
    /// Applies a finite mouse-wheel dolly delta to the editor-only camera distance with safe clamps.
    [[nodiscard]] bool ZoomViewportUVE(float wheelDelta);

    /// Creates one root-level document entity with a TransformComponentUVE and the specialized
    /// component implied by `kind`, selects it, and marks the document dirty. Returns the invalid
    /// entity handle without mutation when the editor is not running or `kind` is unsupported.
    [[nodiscard]] Scene::EntityUVE CreateDocumentEntityUVE(EditorEntityKindUVE kind);

    /// Duplicates the selected live document entity and all descendants under the selected root's
    /// current parent, assigns the duplicate root a deterministic available name when it has name
    /// metadata, selects the fresh root, marks the scene dirty, and records one Undo/Redo entry.
    /// Returns the invalid handle without mutation for invalid state, a stale editor camera, an
    /// active viewport gesture, unsupported snapshot component data, or failed restoration.
    [[nodiscard]] Scene::EntityUVE DuplicateSelectedEntityUVE();

    /// Deletes the selected live document entity and every descendant after capturing a reversible
    /// in-memory snapshot. The still-live document parent becomes selected when present; otherwise
    /// selection is cleared. Returns false without mutation for the same invalid/safety states as
    /// DuplicateSelectedEntityUVE().
    [[nodiscard]] bool DeleteSelectedEntityUVE();

    /// Reparents the selected document subtree under newParent, or makes it a document root when
    /// newParent is invalid. The selected entity retains its authored local Transform and one
    /// successful move is recorded as an Undo/Redo operation. Returns false without mutation for
    /// invalid, stale, self-parenting, cyclic, unchanged, or active-gesture state.
    [[nodiscard]] bool ReparentSelectedEntityUVE(Scene::EntityUVE newParent);

    /// Replays the most recent supported editor mutation in reverse. It is safe and returns false
    /// when the editor is not running, history is empty, or a target became stale externally.
    [[nodiscard]] bool UndoUVE();
    /// Reapplies the most recently undone supported editor mutation. It follows UndoUVE's lifecycle
    /// and stale-target safety rules and never records another history entry while replaying.
    [[nodiscard]] bool RedoUVE();
    [[nodiscard]] bool CanUndoUVE() const noexcept;
    [[nodiscard]] bool CanRedoUVE() const noexcept;

    [[nodiscard]] std::vector<Scene::EntityUVE> GetDocumentRootsUVE();
    [[nodiscard]] EditorStateUVE GetStateUVE() const noexcept;
    [[nodiscard]] Scene::EntityUVE GetSelectedEntityUVE() const noexcept;
    [[nodiscard]] Scene::EntityUVE GetViewportCameraUVE() const noexcept;
    [[nodiscard]] Math::Vector3UVE GetViewportFocusPointUVE() const noexcept;
    [[nodiscard]] float GetViewportDistanceUVE() const noexcept;
    [[nodiscard]] EditorViewportNavigationModeUVE GetViewportNavigationModeUVE() const noexcept;
    [[nodiscard]] bool IsSceneDirtyUVE() const noexcept;
    [[nodiscard]] const std::optional<Asset::AssetRecordUVE>& GetSelectedAssetUVE() const noexcept;
    [[nodiscard]] const std::string& GetAssetFilterUVE() const noexcept;
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
        bool initialDirty = false;
    };

    struct TransformHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::TransformComponentUVE before{};
        Scene::TransformComponentUVE after{};
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct NameHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        std::optional<std::string> beforeName;
        std::optional<std::string> afterName;
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct CreationHistoryEntryUVE final {
        EditorEntityKindUVE kind = EditorEntityKindUVE::Empty;
        std::string name;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A duplicated subtree is restored from a scene-envelope snapshot instead of relying on stale
    /// ECS handles. `activeEntity` is invalid after Undo and becomes a fresh root after Redo.
    struct DuplicationHistoryEntryUVE final {
        Scene::SceneSnapshotUVE snapshot;
        Scene::EntityUVE originalParent = Scene::kInvalidEntityUVE;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        std::optional<std::string> duplicateRootName;
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A deleted subtree is restored under its original parent with fresh handles on Undo.
    /// `activeEntity` begins as the deleted root's stale handle and changes to the restored root.
    struct DeletionHistoryEntryUVE final {
        Scene::SceneSnapshotUVE snapshot;
        Scene::EntityUVE originalParent = Scene::kInvalidEntityUVE;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A hierarchy move keeps its entity handle and local Transform, while its parent changes.
    /// Replay restores either stored parent and never records nested history.
    struct ReparentHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::EntityUVE parentBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE parentAfter = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE selectionAfter = Scene::kInvalidEntityUVE;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    using HistoryEntryUVE =
        std::variant<TransformHistoryEntryUVE, NameHistoryEntryUVE, CreationHistoryEntryUVE,
                     DuplicationHistoryEntryUVE, DeletionHistoryEntryUVE, ReparentHistoryEntryUVE>;

    [[nodiscard]] bool IsDocumentEntityUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] bool HasSceneGraphNodeUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] bool IsTransformFiniteUVE(const Scene::TransformComponentUVE& transform) const noexcept;
    [[nodiscard]] bool IsEntityNameValidUVE(std::string_view name) const noexcept;
    [[nodiscard]] std::string GetEntityDisplayLabelUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] std::string GetDefaultEntityNameUVE(EditorEntityKindUVE kind) const;
    [[nodiscard]] std::string MakeUniqueDocumentEntityNameUVE(std::string_view baseName) const;
    [[nodiscard]] bool IsViewportRectValidUVE(const EditorViewportRectUVE& viewportRect) const noexcept;
    [[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept;
    [[nodiscard]] Math::Vector3UVE GetAxisVectorUVE(EditorTranslateAxisUVE axis) const noexcept;
    [[nodiscard]] bool ProjectWorldPointUVE(const EditorViewportRectUVE& viewportRect,
                                             const Math::Vector3UVE& worldPoint,
                                             Math::Vector2UVE& outScreenPoint) const;
    [[nodiscard]] bool ComputeLocalDeltaForWorldDeltaUVE(Scene::EntityUVE entity,
                                                           const Math::Vector3UVE& worldDelta,
                                                           Math::Vector3UVE& outLocalDelta) const;
    [[nodiscard]] bool ApplyViewportCameraUVE();
    [[nodiscard]] bool IsViewportNavigationFiniteUVE() const noexcept;
    void CancelViewportNavigationUVE() noexcept;
    [[nodiscard]] bool BeginGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                          Math::Vector2UVE pointerPosition);
    void UpdateGizmoDragUVE(Math::Vector2UVE pointerPosition);
    void CommitGizmoDragUVE();
    void CancelGizmoDragUVE() noexcept;
    void DrawTranslateGizmoUVE(const EditorViewportRectUVE& viewportRect);
    [[nodiscard]] bool ApplyLocalTransformUVE(Scene::EntityUVE entity,
                                               const Scene::TransformComponentUVE& transform);
    [[nodiscard]] bool ApplyEntityNameStateUVE(Scene::EntityUVE entity,
                                                const std::optional<std::string>& name);
    [[nodiscard]] bool IsDocumentSubtreeUVE(Scene::EntityUVE root) const;
    [[nodiscard]] bool DoesSubtreeContainEntityUVE(Scene::EntityUVE root,
                                                    Scene::EntityUVE candidate) const;
    [[nodiscard]] std::optional<Scene::SceneSnapshotUVE> CaptureSubtreeUVE(Scene::EntityUVE root);
    [[nodiscard]] Scene::EntityUVE RestoreSubtreeUnderParentUVE(const Scene::SceneSnapshotUVE& snapshot,
                                                                 Scene::EntityUVE parent);
    [[nodiscard]] bool TryGetDocumentParentUVE(Scene::EntityUVE entity, Scene::EntityUVE& outParent) const;
    [[nodiscard]] bool IsLifecycleCommandAllowedUVE() const noexcept;
    [[nodiscard]] bool ReparentDocumentEntityUVE(Scene::EntityUVE entity, Scene::EntityUVE newParent);
    [[nodiscard]] Scene::EntityUVE CreateDocumentEntityInternalUVE(
        EditorEntityKindUVE kind, const std::optional<std::string>& explicitName);
    void RecordHistoryUVE(HistoryEntryUVE entry);
    void ClearHistoryUVE() noexcept;
    void RestoreSelectionUVE(Scene::EntityUVE selection) noexcept;
    [[nodiscard]] bool UndoHistoryEntryUVE(HistoryEntryUVE& entry);
    [[nodiscard]] bool RedoHistoryEntryUVE(HistoryEntryUVE& entry);
    void DestroyDocumentSubtreeUVE(Scene::EntityUVE root);
    void ClearDocumentSceneUVE();
    void DrawMenuBarUVE();
    void DrawHierarchyPanelUVE();
    void DrawHierarchyNodeUVE(Scene::EntityUVE entity);
    void AcceptHierarchyDropTargetUVE(Scene::EntityUVE targetParent);
    void DrawInspectorPanelUVE();
    void DrawViewportPanelUVE();
    void DrawAssetsPanelUVE();

    Core::EngineServicesUVE* m_services = nullptr;
    EditorStateUVE m_state = EditorStateUVE::Uninitialized;
    Scene::EntityUVE m_viewportCamera = Scene::kInvalidEntityUVE;
    Scene::EntityUVE m_selectedEntity = Scene::kInvalidEntityUVE;
    std::filesystem::path m_activeScenePath;
    std::size_t m_historyCapacity = 100U;
    GizmoDragUVE m_gizmoDrag{};
    Math::Vector3UVE m_viewportFocusPoint{0.0F, 1.5F, 0.0F};
    float m_viewportYawRadians = 0.0F;
    float m_viewportPitchRadians = 0.0F;
    float m_viewportDistance = 6.0F;
    EditorViewportNavigationModeUVE m_viewportNavigationMode = EditorViewportNavigationModeUVE::None;
    std::deque<HistoryEntryUVE> m_undoHistory;
    std::deque<HistoryEntryUVE> m_redoHistory;
    std::string m_assetFilter;
    std::optional<Asset::AssetRecordUVE> m_selectedAsset;
    bool m_sceneDirty = false;
    bool m_uiInitialized = false;
};

} // namespace UVE::Editor
