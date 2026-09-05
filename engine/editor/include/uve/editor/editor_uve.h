// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <map>
#include <memory>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_project_file_index_uve.h"
#include "uve/asset/i_project_change_watcher_uve.h"
#include "uve/core/engine_services_uve.h"
#include "uve/core/i_editor_viewport_host_uve.h"
#include "uve/core/i_simulation_control_uve.h"
#include "uve/editor/editor_tool_session_uve.h"
#include "uve/editor/developer_console_uve.h"
#include "uve/editor/editor_ui_assets_uve.h"
#include "uve/editor/inspector_drawer_registry_uve.h"
#include "uve/editor/mesh_thumbnail_renderer_uve.h"
#include "uve/math/ray_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/scene/components/animation_player_component_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/expanded_3d_node_components_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/nodes/scene_node_registry_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"

namespace UVE::Editor::Tests {
struct EditorUVEAccessUVE;
}

namespace UVE::Editor {

class EditorBridgeUVE;

/// EditorStateUVE is the lifecycle state of one editor session. The editor owns session data only;
/// EngineCoreUVE remains the owner of the ECS, renderer, window, and every engine service.
enum class EditorStateUVE {
    Uninitialized,
    Running,
    Shutdown,
};

/// The editor-owned transient simulation state. Edit permits authored document commands; Playing
/// and Paused own an immutable pre-Play document snapshot and reject authoring mutations.
enum class EditorPlayModeStateUVE {
    Edit,
    Playing,
    Paused,
};

/// A screen-space rectangle used by the editor's transparent viewport overlay. Coordinates are in
/// the native desktop window's ImGui/GLFW pixel space, with an origin at the top-left. Value type;
/// safe to copy across editor helper calls.
struct EditorViewportRectUVE final {
    Math::Vector2UVE origin{};
    Math::Vector2UVE size{};
};

/// Editor-only 2D canvas presentation state for authored screen-space content such as loading
/// screens. The design surface is not an ECS entity, is never serialized into a scene, and never
/// changes runtime state. Pan is expressed in desktop pixels relative to the fitted canvas center.
struct Editor2DCanvasStateUVE final {
    static constexpr float kDesignWidth = 1920.0F;
    static constexpr float kDesignHeight = 1080.0F;

    float zoom = 0.36F;
    Math::Vector2UVE pan{};
    bool gridVisible = true;
    bool safeAreaVisible = true;
};

/// Canonical named axes used by EditorUVE's Translate, Rotate, and Scale gizmos. The active
/// coordinate space chooses whether their world or selected-entity-local basis is used.
enum class EditorTransformAxisUVE {
    None,
    X,
    Y,
    Z,
};

/// Source-compatible name retained for downstream editor callers; new code should use the
/// transform-wide name because the same axis contract is shared by Translate, Rotate, and Scale.
using EditorTranslateAxisUVE = EditorTransformAxisUVE;

/// Selects the active transform-gizmo handle family, or Select for the plain pick/pan tool that
/// shows no gizmo at all. Handles use the session-local World or Local coordinate space; negative
/// and proportional/multiplicative scale remain future work. Select is appended last so existing
/// serialized session values for Translate..Universal keep their ordinals.
enum class EditorGizmoModeUVE {
    Translate,
    Rotate,
    Scale,
    Universal,
    Select,
};

/// Selects whether transform handles use canonical world axes or the selected entity's derived
/// world orientation. This editor session preference is never serialized or added to history.
enum class EditorGizmoCoordinateSpaceUVE {
    World,
    Local,
};

/// Selects whether hierarchy reparenting retains authored local TRS or preserves compatible
/// captured world TRS. This editor-session preference is never serialized or added to history.
enum class EditorReparentTransformModeUVE {
    KeepLocal,
    KeepWorld,
};

/// Session-local transform snapping settings. These values are editor-only and are not serialized
/// into scene documents or runtime state.
struct EditorTransformSnappingSettingsUVE final {
    bool enabled = false;
    float translateStep = 1.0F;
    float rotateStepDegrees = 15.0F;
    float scaleStep = 0.1F;
};

/// A read-only oriented box for the selected collider-backed document entity. All points are in
/// derived world space and are intended for editor feedback only; this value is never serialized.
struct EditorSelectionBoundsUVE final {
    std::array<Math::Vector3UVE, 8> worldCorners{};
    Math::Vector3UVE worldCenter{};
};

/// The supported Library workspace archetypes. Each created entity is a document root with a
/// TransformComponentUVE; specialized kinds add only the named gameplay or built-in primitive component.
enum class EditorSceneComponentKindUVE : std::uint8_t {
    Camera,
    Mesh,
    Light,
    Collider,
    RigidBody,
    AudioSource,
    ParticleEmitter,
    Script,
    AnimationPlayer,
    WorldEnvironment,
};

using EditorSceneComponentValueUVE =
    std::variant<Scene::CameraComponentUVE, Scene::MeshComponentUVE, Scene::LightComponentUVE,
                 Scene::ColliderComponentUVE, Scene::RigidBodyComponentUVE, Scene::AudioSourceComponentUVE,
                 Scene::ParticleEmitterComponentUVE, Scene::ScriptComponentUVE,
                 Scene::AnimationPlayerComponentUVE, Scene::WorldEnvironment3DNodeComponentUVE>;

enum class EditorEntityKindUVE {
    Empty,
    Camera,
    DirectionalLight,
    CollisionBox,
    Cube,
    UVSphere,
    Plane,
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
/// mutation path, world-space Translate, Rotate, and Scale gizmos, scene-document save/load, and an editor-private
/// Dear ImGui overlay. No Dear ImGui type appears in this public interface, so the UI backend remains
/// an implementation detail of engine/editor.
///
/// The supplied EngineServicesUVE reference must remain valid from InitUVE() through ShutdownUVE().
/// EditorUVE is main-thread only, matching the scene, render, and window services it composes.
class EditorUVE final {
    friend struct Tests::EditorUVEAccessUVE;
    friend class EditorBridgeUVE;

public:
    explicit EditorUVE(Core::EngineServicesUVE& services,
                       std::filesystem::path activeScenePath = "editor_scene.uvescene",
                       std::size_t historyCapacity = 100U,
                       Core::ISimulationControlUVE* simulationControl = nullptr,
                       Core::IEditorViewportHostUVE* viewportHost = nullptr);
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

    /// Captures the complete editable document into an in-memory scene envelope and enters the
    /// transient Play sandbox. Returns false without document mutation when capture or Core control fails.
    [[nodiscard]] bool EnterPlayModeUVE();
    [[nodiscard]] bool PausePlayModeUVE();
    [[nodiscard]] bool ResumePlayModeUVE();
    [[nodiscard]] bool StepPlayModeUVE();
    [[nodiscard]] bool StopPlayModeUVE();
    [[nodiscard]] EditorPlayModeStateUVE GetPlayModeStateUVE() const noexcept;

    /// Draws the private editor overlay. EngineCoreUVE invokes this from its post-render callback
    /// before PresentUVE(), after the HDR scene has passed through the standard tone-mapping path.
    void RenderOverlayUVE();

    /// Saves every document root except the editor camera to the active .uvescene path. Dirty state
    /// is cleared only after the scene serializer reports success.
    [[nodiscard]] bool SaveSceneUVE();

    /// Saves the sole selected document subtree as a canonical `.uveprefab` and registers its source
    /// GUID through the existing PrefabSystemUVE. This command never runs during Play or a viewport gesture.
    [[nodiscard]] bool SaveSelectedPrefabUVE(const std::filesystem::path& path);

    /// Refreshes the sole selected prefab instance from its current source revision. Dirty instances
    /// are rejected with merge-required semantics and are never silently overwritten.
    [[nodiscard]] bool RefreshSelectedPrefabUVE();

    /// Explicitly discards persisted local prefab overrides and refreshes from source. This is a
    /// destructive authoring command and is rejected outside Edit mode or without a sole selection.
    [[nodiscard]] bool DiscardSelectedPrefabOverridesAndRefreshUVE();

    /// Replaces the editable document scene with the active .uvescene file. A backup scene is
    /// created before destructive mutation and restored if deserialization fails; the editor camera
    /// remains outside the document root set.
    [[nodiscard]] bool LoadSceneUVE();

    /// Makes entity the sole ordered hierarchy/inspector selection when it is live; invalid or
    /// deleted handles clear the selection instead of exposing stale ECS state.
    void SelectEntityUVE(Scene::EntityUVE entity) noexcept;
    /// Adds a live document entity to the ordered selection or removes it when already selected.
    /// A newly added entity becomes active. Removing the active entity promotes the last remaining
    /// selected entity; removing the final entity clears the active selection.
    void ToggleEntitySelectionUVE(Scene::EntityUVE entity) noexcept;
    void ClearSelectionUVE() noexcept;
    /// Returns the ordered, deduplicated live document selection. The final entry is active whenever
    /// ToggleEntitySelectionUVE() removed the prior active entity.
    [[nodiscard]] const std::vector<Scene::EntityUVE>& GetSelectedEntitiesUVE() const noexcept;
    /// Returns true only when the active entity is the sole selected live document entity.
    [[nodiscard]] bool HasSingleDocumentSelectionUVE() const noexcept;

    /// Applies one validated local transform through ISceneGraphUVE, ensuring derived world
    /// transforms are marked dirty for EngineCoreUVE's next scene-graph update. Returns false for
    /// invalid/deleted/non-transform entities or non-finite transform values.
    [[nodiscard]] bool SetSelectedLocalTransformUVE(const Scene::TransformComponentUVE& transform);

    /// Adds or updates persistent human-readable metadata for the selected live document entity.
    /// Returns false without mutation for invalid editor/selection state, an empty or whitespace-only
    /// name, a name longer than the supported editor-entry limit, or an unchanged value.
    [[nodiscard]] bool SetSelectedEntityNameUVE(std::string name);

    /// Adds or replaces one supported scene component on the selected document entity using the
    /// value variant matching `kind`. Valid changes are one Undo/Redo transaction; invalid, unchanged,
    /// multi-selected, protected-Play, active-gesture, or mismatched kind/value calls fail atomically.
    [[nodiscard]] bool SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE kind,
                                                     const EditorSceneComponentValueUVE& value);

    /// Removes one supported scene component from the selected entity as one Undo/Redo transaction.
    /// Core identity components such as Transform, Name, and Hierarchy are intentionally excluded.
    [[nodiscard]] bool RemoveSelectedSceneComponentUVE(EditorSceneComponentKindUVE kind);

    /// Replaces the selected primitive's complete authored appearance atomically. Primitive kind
    /// and bounded linear-RGB base color are both editable after creation; one changed valid call
    /// becomes one Primitive Appearance Undo/Redo transaction. Invalid, unchanged, multi-selected,
    /// protected-Play, or competing-gesture state returns false without mutation.
    [[nodiscard]] bool SetSelectedPrimitiveMeshUVE(const Scene::PrimitiveMeshComponentUVE& primitive);

    /// Creates a normalized world-space ray from a pointer inside viewportRect. Uses the editor
    /// camera's derived world transform and perspective settings. Returns std::nullopt for invalid
    /// editor state, camera data, viewport geometry, or pointer coordinates outside the rectangle.
    [[nodiscard]] std::optional<Math::RayUVE> MakeViewportRayUVE(const EditorViewportRectUVE& viewportRect,
                                                                   Math::Vector2UVE pointerPosition) const;

    /// Uses the existing deterministic box-collider raycast system to select the closest live
    /// document entity under pointerPosition. A valid regular viewport miss clears selection; a
    /// toggle-selection miss retains it. Entities without ColliderComponentUVE are intentionally
    /// not selectable in this first picking slice.
    [[nodiscard]] bool PickViewportUVE(const EditorViewportRectUVE& viewportRect,
                                        Math::Vector2UVE pointerPosition,
                                        bool toggleSelection = false);

    /// Moves the selected document entity by a finite world-space distance along one unit world
    /// axis. Parent world rotation and scale are converted back to a local position delta before
    /// applying the existing scene-graph transform path. Returns false without mutation if the
    /// entity, parent transform, axis, or distance is invalid.
    [[nodiscard]] bool TranslateSelectedAlongAxisUVE(EditorTransformAxisUVE axis, float worldDistance);

    /// Rotates the selected document entity around one finite world axis by radians. A parented
    /// entity receives the equivalent local quaternion delta through the current parent world
    /// rotation. Returns false without mutation for invalid state, axis, angle, transform, parent,
    /// or active editor gesture.
    [[nodiscard]] bool RotateSelectedAroundWorldAxisUVE(EditorTransformAxisUVE axis, float radians);

    /// Changes one positive authored local-scale component of the selected document entity by a
    /// finite additive delta. Returns false without mutation for invalid state, axis, delta, active
    /// gesture, or a proposed zero/negative/non-finite scale result.
    [[nodiscard]] bool ScaleSelectedAlongAxisUVE(EditorTransformAxisUVE axis, float localScaleDelta);

    /// Adds one finite local-scale offset to every authored local-scale component of the selected
    /// entity. The command rejects as a whole if any proposed component is non-finite or below the
    /// positive scale floor; it never clamps individual components or performs proportional scaling.
    [[nodiscard]] bool ScaleSelectedUniformlyUVE(float localScaleOffset);

    /// Changes the transform handle family. Mode changes are ignored while a gizmo drag or viewport
    /// navigation gesture is active, preserving the transaction currently in progress.
    void SetGizmoModeUVE(EditorGizmoModeUVE mode) noexcept;
    [[nodiscard]] EditorGizmoModeUVE GetGizmoModeUVE() const noexcept;
    [[nodiscard]] bool SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE coordinateSpace);
    [[nodiscard]] EditorGizmoCoordinateSpaceUVE GetGizmoCoordinateSpaceUVE() const noexcept;

    /// Replaces session-local snapping settings only when every increment is finite and strictly
    /// positive and no transform/navigation gesture is active. Returns false without mutation otherwise.
    [[nodiscard]] bool SetTransformSnappingSettingsUVE(const EditorTransformSnappingSettingsUVE& settings);
    [[nodiscard]] const EditorTransformSnappingSettingsUVE& GetTransformSnappingSettingsUVE() const noexcept;

    /// Returns the derived world-space box for the active live collider-backed document entity.
    /// It never mutates selection, scene state, dirty state, or Undo/Redo history; unsafe or
    /// unsupported state returns std::nullopt.
    [[nodiscard]] std::optional<EditorSelectionBoundsUVE> TryGetSelectedBoundsUVE() const;

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

    /// Creates a user-facing node from the centralized SceneNode registry. Runtime ownership remains
    /// in core/physics/render/audio/scripting; this method only creates the authored scene façade.
    [[nodiscard]] Scene::EntityUVE CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE kind);

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
    /// newParent is invalid. Keep Local retains authored local Transform; optional Keep World
    /// preserves only a validated shear-safe compatible world TRS. One successful move is recorded
    /// as an Undo/Redo operation. Returns false without mutation for unsafe state or solve input.
    [[nodiscard]] bool ReparentSelectedEntityUVE(Scene::EntityUVE newParent);
    [[nodiscard]] bool SetReparentTransformModeUVE(EditorReparentTransformModeUVE mode);
    [[nodiscard]] EditorReparentTransformModeUVE GetReparentTransformModeUVE() const noexcept;

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
    [[nodiscard]] bool IsViewportEnvironmentPreviewEnabledUVE() const noexcept;
    void SetViewportEnvironmentPreviewEnabledUVE(bool enabled) noexcept;
    [[nodiscard]] bool IsViewportSunPreviewEnabledUVE() const noexcept;
    void SetViewportSunPreviewEnabledUVE(bool enabled) noexcept;
    /// Returns editor-only 2D canvas state for screen-space authoring. It is not scene data.
    [[nodiscard]] Editor2DCanvasStateUVE Get2DCanvasStateUVE() const noexcept;
    /// Validates and updates the editor-only 2D canvas zoom without changing scene state/history.
    [[nodiscard]] bool Set2DCanvasZoomUVE(float zoom) noexcept;
    /// Restores the editor-only 2D canvas to its centered loading-screen design view.
    void Reset2DCanvasViewUVE() noexcept;
    [[nodiscard]] bool IsSceneDirtyUVE() const noexcept;
    /// Returns whether the editor-only Control Rig tool gate is enabled. This state never creates
    /// runtime entities, changes authored scene data, or serializes into a project.
    [[nodiscard]] bool IsControlRigPluginEnabledUVE() const noexcept;
    /// Updates the editor-only Control Rig tool gate without touching ECS or scene history.
    void SetControlRigPluginEnabledUVE(bool enabled) noexcept;
    /// Returns whether the editor-only Motion Query tool gate is enabled. This state never creates
    /// runtime entities, changes authored scene data, or serializes into a project.
    [[nodiscard]] bool IsMotionQueryPluginEnabledUVE() const noexcept;
    /// Updates the editor-only Motion Query tool gate without touching ECS or scene history.
    void SetMotionQueryPluginEnabledUVE(bool enabled) noexcept;
    /// Read-only transform-tool lifecycle diagnostics. These values expose editor-session evidence
    /// only; they neither alter input routing nor claim any ECS mutation succeeded.
    [[nodiscard]] EditorToolSessionPhaseUVE GetToolSessionPhaseUVE() const noexcept;
    [[nodiscard]] EditorToolSessionOutcomeUVE GetLastToolSessionOutcomeUVE() const noexcept;
    /// Returns the selected registered asset record when the selected project-file entry is a currently
    /// correlated file. Directories and unregistered files return std::nullopt.
    [[nodiscard]] const std::optional<Asset::AssetRecordUVE>& GetSelectedAssetUVE() const noexcept;
    /// Returns the selected cached project-file entry, including directories and unregistered files.
    [[nodiscard]] const std::optional<Asset::ProjectFileEntryUVE>& GetSelectedProjectFileUVE() const noexcept;
    [[nodiscard]] const std::string& GetAssetFilterUVE() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetActiveScenePathUVE() const noexcept;
    void SetActiveScenePathUVE(std::filesystem::path path);
    [[nodiscard]] Scripting::ScriptGraphCanvasUVE& GetVisualScriptCanvasUVE() noexcept;
    [[nodiscard]] const Scripting::ScriptNodeRegistryUVE& GetVisualScriptRegistryUVE() const noexcept;
    [[nodiscard]] std::vector<std::string> GetVisualScriptBranchNamesUVE() const;
    [[nodiscard]] const std::string& GetActiveVisualScriptBranchNameUVE() const noexcept;
    [[nodiscard]] bool CreateVisualScriptBranchUVE(std::string name);
    [[nodiscard]] bool SelectVisualScriptBranchUVE(std::string name);
    [[nodiscard]] bool RenameActiveVisualScriptBranchUVE(std::string name);
    [[nodiscard]] bool SaveVisualScriptWorkspaceUVE();
    [[nodiscard]] bool LoadVisualScriptWorkspaceUVE();

    /// Releases editor-private UI resources and destroys the editor camera while the services are
    /// still alive. Idempotent after the first successful shutdown.
    void ShutdownUVE();

private:
    enum class EditorLayoutPresetUVE : std::uint8_t {
        Default,
        FocusViewport,
        ContentReview,
    };

    struct EditorSelectionPathUVE final {
        std::size_t rootIndex = 0U;
        std::vector<std::size_t> childIndices;
    };

    struct EditorSelectionSnapshotUVE final {
        std::vector<Scene::EntityUVE> entities;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
    };

    struct EditorSelectionPathsUVE final {
        std::vector<EditorSelectionPathUVE> entityPaths;
        std::optional<EditorSelectionPathUVE> activePath;
    };

    struct ScriptBranchUVE final {
        std::string name;
        std::unique_ptr<Scripting::ScriptGraphCanvasUVE> canvas;
    };

    struct PlayModeSessionUVE final {
        Scene::SceneSnapshotUVE documentSnapshot;
        bool capturedEmptyDocument = false;
        bool dirtyBefore = false;
        EditorSelectionPathsUVE selectionBefore;
    };

    enum class EditorTranslatePlaneUVE {
        None,
        XY,
        XZ,
        YZ,
    };

    enum class GizmoHandleKindUVE {
        Axis,
        Plane,
        UniformScaleOffset,
        ScreenPlaneMove,
        Trackball,
    };

    /// Editor-only workspace labels. They do not alter document data, simulation state, or history.
    enum class EditorWorkspaceUVE {
        Library,
        Asset,
        Scripting,
        Debug,
        Plugin,
    };

    /// Selects the visible content inside the fixed right-side editor panel.
    enum class EditorRightPanelTabUVE {
        Inspector,
        Import,
        Signals,
    };

    /// Selects the editor viewport surface. The 2D tab is an editor-owned screen-space canvas;
    /// the 3D tab renders the real scene viewport.
    enum class EditorViewportTabUVE {
        Scene,
        TwoD,
    };

    /// Selects one docked lower-workspace panel. FileSystem is the safe default and keeps the
    /// former Assets database view visible without introducing an AI tooling implementation.
    enum class EditorBottomDockUVE {
        Debugger,
        Animator,
        AIToolbar,
        FileSystem,
    };

    /// Session-only Content Browser focus. This filters copied ProjectFileIndexUVE entries and
    /// never requests I/O, loads an asset, or changes the AssetDatabaseUVE registry.
    enum class ContentBrowserTypeFocusUVE {
        All,
        Folders,
        Scene,
        Prefab,
        Bundle,
        Mesh,
        Texture,
        Shader,
        Material,
        Save,
        MotionQuery,
        Registered,
        OtherFiles,
    };

    /// A file's primary presentation type. Registry correlation is deliberately a separate badge:
    /// one registered `.uvemodel` row therefore remains Mesh + Registered, never an ambiguous tag.
    enum class ContentBrowserItemTypeUVE {
        Folder,
        Scene,
        Prefab,
        Bundle,
        Mesh,
        Texture,
        Shader,
        Material,
        Save,
        MotionQuery,
        File,
    };

    struct GizmoDragUVE final {
        EditorGizmoModeUVE mode = EditorGizmoModeUVE::Translate;
        GizmoHandleKindUVE handleKind = GizmoHandleKindUVE::Axis;
        EditorTransformAxisUVE axis = EditorTransformAxisUVE::None;
        EditorTranslatePlaneUVE plane = EditorTranslatePlaneUVE::None;
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Math::Vector2UVE initialPointer{};
        Math::Vector2UVE screenCenter{};
        Math::Vector2UVE screenAxisDirection{};
        Math::Vector2UVE screenPlaneAxisA{};
        Math::Vector2UVE screenPlaneAxisB{};
        Math::Vector3UVE worldAxisA{};
        Math::Vector3UVE worldAxisB{};
        Math::Vector3UVE initialTrackballVector{};
        Math::QuaternionUVE viewWorldRotation{};
        EditorViewportRectUVE viewportRect{};
        float pixelsPerWorldUnit = 0.0F;
        float trackballRadiusPixels = 0.0F;
        float initialRingParameterRadians = 0.0F;
    };

    struct TransformHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::TransformComponentUVE before{};
        Scene::TransformComponentUVE after{};
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct NameHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        std::optional<std::string> beforeName;
        std::optional<std::string> afterName;
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct PrimitiveAppearanceHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::PrimitiveMeshComponentUVE before{};
        Scene::PrimitiveMeshComponentUVE after{};
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct SceneComponentHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        EditorSceneComponentKindUVE kind = EditorSceneComponentKindUVE::Camera;
        std::optional<EditorSceneComponentValueUVE> before;
        std::optional<EditorSceneComponentValueUVE> after;
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    struct CreationHistoryEntryUVE final {
        EditorEntityKindUVE kind = EditorEntityKindUVE::Empty;
        std::string name;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A centralized scene node is restored from one complete authored snapshot so compound node
    /// creation (for example CharacterBody3D plus Collider and kinematic RigidBody) is one history unit.
    struct SceneNodeCreationHistoryEntryUVE final {
        Scene::SceneSnapshotUVE snapshot;
        Scene::Nodes::SceneNodeKindUVE kind = Scene::Nodes::SceneNodeKindUVE::Empty;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
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
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A deleted subtree is restored under its original parent with fresh handles on Undo.
    /// `activeEntity` begins as the deleted root's stale handle and changes to the restored root.
    struct DeletionHistoryEntryUVE final {
        Scene::SceneSnapshotUVE snapshot;
        Scene::EntityUVE originalParent = Scene::kInvalidEntityUVE;
        Scene::EntityUVE activeEntity = Scene::kInvalidEntityUVE;
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    /// A hierarchy move restores its parent and authored local Transform atomically on replay.
    struct ReparentHistoryEntryUVE final {
        Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
        Scene::EntityUVE parentBefore = Scene::kInvalidEntityUVE;
        Scene::EntityUVE parentAfter = Scene::kInvalidEntityUVE;
        Scene::TransformComponentUVE localTransformBefore{};
        Scene::TransformComponentUVE localTransformAfter{};
        EditorSelectionSnapshotUVE selectionBefore;
        EditorSelectionSnapshotUVE selectionAfter;
        bool dirtyBefore = false;
        bool dirtyAfter = false;
    };

    using HistoryEntryUVE =
        std::variant<TransformHistoryEntryUVE, NameHistoryEntryUVE, PrimitiveAppearanceHistoryEntryUVE,
                     SceneComponentHistoryEntryUVE, CreationHistoryEntryUVE, SceneNodeCreationHistoryEntryUVE,
                     DuplicationHistoryEntryUVE,
                     DeletionHistoryEntryUVE,
                     ReparentHistoryEntryUVE>;

    [[nodiscard]] bool IsDocumentEntityUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] bool HasSceneGraphNodeUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] bool IsTransformFiniteUVE(const Scene::TransformComponentUVE& transform) const noexcept;
    [[nodiscard]] bool IsEntityNameValidUVE(std::string_view name) const noexcept;
    [[nodiscard]] std::string GetEntityDisplayLabelUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] std::string GetDefaultEntityNameUVE(EditorEntityKindUVE kind) const;
    [[nodiscard]] std::string MakeUniqueDocumentEntityNameUVE(std::string_view baseName) const;
    [[nodiscard]] bool IsViewportRectValidUVE(const EditorViewportRectUVE& viewportRect) const noexcept;
    [[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept;
    [[nodiscard]] bool IsQuaternionFiniteUVE(const Math::QuaternionUVE& quaternion) const noexcept;
    [[nodiscard]] bool AreTransformSnappingSettingsValidUVE(
        const EditorTransformSnappingSettingsUVE& settings) const noexcept;
    [[nodiscard]] float SnapScalarUVE(float value, float increment) const noexcept;
    [[nodiscard]] Math::Vector3UVE GetAxisVectorUVE(EditorTransformAxisUVE axis) const noexcept;
    [[nodiscard]] bool GetGizmoAxisWorldVectorUVE(Scene::EntityUVE entity, EditorTransformAxisUVE axis,
                                                    Math::Vector3UVE& outAxis) const;
    [[nodiscard]] bool GetPlaneAxesUVE(EditorTranslatePlaneUVE plane, Math::Vector3UVE& outAxisA,
                                        Math::Vector3UVE& outAxisB) const noexcept;
    [[nodiscard]] bool ProjectWorldPointUVE(const EditorViewportRectUVE& viewportRect,
                                             const Math::Vector3UVE& worldPoint,
                                             Math::Vector2UVE& outScreenPoint) const;
    [[nodiscard]] bool ComputeLocalDeltaForWorldDeltaUVE(Scene::EntityUVE entity,
                                                           const Math::Vector3UVE& worldDelta,
                                                           Math::Vector3UVE& outLocalDelta) const;
    [[nodiscard]] bool ComputeLocalRotationForWorldAxisUVE(Scene::EntityUVE entity,
                                                            const Math::QuaternionUVE& initialLocalRotation,
                                                            const Math::Vector3UVE& worldAxis, float radians,
                                                            Math::QuaternionUVE& outLocalRotation) const;
    [[nodiscard]] bool ApplyViewportCameraUVE();
    [[nodiscard]] bool ApplyViewportPresetUVE(float yawRadians, float pitchRadians) noexcept;
    void UpdateViewportPresetAnimationUVE();
    [[nodiscard]] bool HandleViewportNavigationGizmoClickUVE(const EditorViewportRectUVE& viewportRect,
                                                              Math::Vector2UVE pointerPosition);
    [[nodiscard]] bool IsViewportNavigationFiniteUVE() const noexcept;
    void CancelViewportNavigationUVE() noexcept;
    [[nodiscard]] bool BeginGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                          Math::Vector2UVE pointerPosition);
    [[nodiscard]] bool BeginGizmoDragForModeUVE(const EditorViewportRectUVE& viewportRect,
                                                 Math::Vector2UVE pointerPosition,
                                                 EditorGizmoModeUVE mode);
    [[nodiscard]] bool BeginRotateGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                                Math::Vector2UVE pointerPosition);
    [[nodiscard]] bool MapTrackballPointerUVE(Math::Vector2UVE center, float radius,
                                               Math::Vector2UVE pointerPosition,
                                               Math::Vector3UVE& outVector) const noexcept;
    [[nodiscard]] bool FindClosestRingParameterUVE(const EditorViewportRectUVE& viewportRect,
                                                    Scene::EntityUVE entity, EditorTransformAxisUVE axis,
                                                    Math::Vector2UVE pointerPosition,
                                                    float& outParameterRadians,
                                                    float& outDistanceSquared) const;
    void UpdateGizmoDragUVE(Math::Vector2UVE pointerPosition);
    void CommitGizmoDragUVE();
    void CancelGizmoDragUVE() noexcept;
    void DrawSelectionBoundsUVE(const EditorViewportRectUVE& viewportRect);
    /// Draws all transform handle families around one selection while preserving the existing
    /// mode-specific drag/commit semantics and editor-only ownership.
    void DrawUnifiedTransformGizmoUVE(const EditorViewportRectUVE& viewportRect);
    [[nodiscard]] bool ApplyLocalTransformUVE(Scene::EntityUVE entity,
                                               const Scene::TransformComponentUVE& transform);
    [[nodiscard]] bool ApplyEntityNameStateUVE(Scene::EntityUVE entity,
                                                const std::optional<std::string>& name);
    [[nodiscard]] bool ApplyPrimitiveMeshStateUVE(Scene::EntityUVE entity,
                                                    const Scene::PrimitiveMeshComponentUVE& primitive);
    [[nodiscard]] bool IsSceneComponentValueValidUVE(EditorSceneComponentKindUVE kind,
                                                      const EditorSceneComponentValueUVE& value) const noexcept;
    [[nodiscard]] bool AreSceneComponentValuesEqualUVE(const EditorSceneComponentValueUVE& lhs,
                                                        const EditorSceneComponentValueUVE& rhs) const noexcept;
    [[nodiscard]] bool ApplySceneComponentStateUVE(
        Scene::EntityUVE entity, EditorSceneComponentKindUVE kind,
        const std::optional<EditorSceneComponentValueUVE>& value);
    [[nodiscard]] bool IsDocumentSubtreeUVE(Scene::EntityUVE root) const;
    [[nodiscard]] bool DoesSubtreeContainEntityUVE(Scene::EntityUVE root,
                                                    Scene::EntityUVE candidate) const;
    [[nodiscard]] std::optional<Scene::SceneSnapshotUVE> CaptureSubtreeUVE(Scene::EntityUVE root);
    [[nodiscard]] Scene::EntityUVE RestoreSubtreeUnderParentUVE(const Scene::SceneSnapshotUVE& snapshot,
                                                                 Scene::EntityUVE parent);
    [[nodiscard]] bool TryGetDocumentParentUVE(Scene::EntityUVE entity, Scene::EntityUVE& outParent) const;
    /// Returns one compact editor-only tag using the fixed primitive-first priority documented for
    /// the Scene Outliner. Empty means the entity is a plain document entity.
    [[nodiscard]] std::string GetOutlinerTypeTagUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] std::vector<Scene::EntityUVE> GetDocumentAncestryUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] std::vector<Scene::EntityUVE> GetEligibleReparentParentsUVE(Scene::EntityUVE entity);
    [[nodiscard]] std::string GetHierarchyCandidateLabelUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] bool IsLifecycleCommandAllowedUVE() const noexcept;
    [[nodiscard]] bool IsAuthoringCommandAllowedUVE() const noexcept;
    [[nodiscard]] EditorSelectionSnapshotUVE CaptureSelectionSnapshotUVE() const;
    void RestoreSelectionUVE(EditorSelectionSnapshotUVE selection) noexcept;
    void PruneSelectionUVE() noexcept;
    [[nodiscard]] bool IsEntitySelectedUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] std::optional<EditorSelectionBoundsUVE> TryGetEntityBoundsUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] EditorSelectionPathsUVE CaptureSelectionPathsUVE(
        const std::vector<Scene::EntityUVE>& roots) const;
    [[nodiscard]] EditorSelectionSnapshotUVE ResolveSelectionPathsUVE(
        const EditorSelectionPathsUVE& paths, const std::vector<Scene::EntityUVE>& roots) const;
    [[nodiscard]] Scene::EntityUVE ResolveSelectionPathUVE(
        const EditorSelectionPathUVE& path, const std::vector<Scene::EntityUVE>& roots) const;
    [[nodiscard]] bool FindSelectionPathUVE(Scene::EntityUVE current, Scene::EntityUVE target,
                                             std::vector<std::size_t>& inOutChildIndices) const;
    [[nodiscard]] bool ReparentDocumentEntityUVE(Scene::EntityUVE entity, Scene::EntityUVE newParent);
    [[nodiscard]] bool ComputeKeepWorldLocalTransformUVE(Scene::EntityUVE entity, Scene::EntityUVE newParent,
                                                          Scene::TransformComponentUVE& outTransform) const;
    [[nodiscard]] bool IsReparentModeChangeAllowedUVE() const noexcept;
    [[nodiscard]] bool IsHierarchyFilterActiveUVE() const noexcept;
    [[nodiscard]] bool IsHierarchyEntityVisibleUVE(Scene::EntityUVE entity) const;
    void RebuildHierarchyFilterCacheUVE();
    void InvalidateHierarchyFilterCacheUVE() noexcept;
    void CancelHierarchyRenameUVE() noexcept;
    [[nodiscard]] Scene::EntityUVE CreateDocumentEntityInternalUVE(
        EditorEntityKindUVE kind, const std::optional<std::string>& explicitName);
    void RecordHistoryUVE(HistoryEntryUVE entry);
    void ClearHistoryUVE() noexcept;
    [[nodiscard]] bool UndoHistoryEntryUVE(HistoryEntryUVE& entry);
    [[nodiscard]] bool RedoHistoryEntryUVE(HistoryEntryUVE& entry);
    void DestroyDocumentSubtreeUVE(Scene::EntityUVE root);
    void ClearDocumentSceneUVE();
    void LoadSessionSettingsUVE();
    [[nodiscard]] bool SaveSessionSettingsUVE();
    void ApplyLayoutPresetUVE(EditorLayoutPresetUVE preset) noexcept;
    void DrawMenuBarUVE();
    void DrawPluginWindowUVE();
    void DrawBottomDockUVE();
    void DrawBottomDockContentUVE();
    void DrawHierarchyPanelUVE();
    void DrawHierarchyNodeUVE(Scene::EntityUVE entity);
    void AcceptHierarchyDropTargetUVE(Scene::EntityUVE targetParent);
    void DrawInspectorPanelUVE();
    void DrawInspectorContentUVE();
    void RegisterBuiltInInspectorDrawersUVE();
    void DrawNameInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawHierarchyInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawTransformInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawPrimitiveMeshInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawWorldEnvironmentInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawSceneComponentInspectorDrawerUVE(Scene::EntityUVE entity, EditorSceneComponentKindUVE kind);
    void DrawSceneComponentAddPanelUVE();
    void DrawPrefabInspectorDrawerUVE(Scene::EntityUVE entity);
    void DrawImportQueueMonitorUVE();
    void DrawViewportToolCanvasUVE();
    void DrawViewportPanelUVE();
    void Draw2DCanvasUVE(const EditorViewportRectUVE& viewportRect);
    void DrawScriptingWorkspaceUVE();
    void CompileVisualScriptUVE();
    [[nodiscard]] static ContentBrowserItemTypeUVE ClassifyContentBrowserEntryUVE(
        const Asset::ProjectFileEntryUVE& entry);
    [[nodiscard]] static const char* GetContentBrowserItemTypeLabelUVE(ContentBrowserItemTypeUVE type) noexcept;
    [[nodiscard]] static const char* GetContentBrowserFocusLabelUVE(ContentBrowserTypeFocusUVE focus) noexcept;
    [[nodiscard]] bool DoesContentBrowserEntryMatchFocusUVE(const Asset::ProjectFileEntryUVE& entry) const;
    [[nodiscard]] bool IsContentBrowserDirectoryInSnapshotUVE(const Asset::ProjectFileSnapshotUVE& snapshot,
                                                               const std::filesystem::path& directory) const;
    void ReconcileContentBrowserDirectoryUVE(const Asset::ProjectFileSnapshotUVE& snapshot) noexcept;
    [[nodiscard]] bool IsProjectPathFavoritedUVE(const std::filesystem::path& relativePath) const;
    void ToggleProjectPathFavoriteUVE(const std::filesystem::path& relativePath);
    /// Returns a GL texture id showing relativePath's own decoded image content, loading and
    /// uploading it on first request and caching the result thereafter. Returns 0 if the file
    /// cannot be loaded as a texture asset (not a texture, corrupt, or an unsupported pixel
    /// format) - callers should fall back to the generic per-type icon in that case.
    [[nodiscard]] std::uintptr_t GetTextureThumbnailUVE(const std::filesystem::path& relativePath);
    void ClearTextureThumbnailCacheUVE() noexcept;
    /// Returns a GL texture id previewing relativePath's own mesh geometry (fixed camera angle,
    /// no material), rendering and caching it on first request. Returns 0 if the file cannot be
    /// loaded as a mesh asset or has no vertices/indices - callers should fall back to the
    /// generic per-type icon in that case.
    [[nodiscard]] std::uintptr_t GetMeshThumbnailUVE(const std::filesystem::path& relativePath);
    void ClearMeshThumbnailCacheUVE() noexcept;
    void DrawAssetsPanelUVE();
    /// Refreshes the read-only project index after the engine-owned watcher observes a new
    /// filesystem baseline. It never schedules imports or mutates project files.
    void RefreshProjectFileIndexUVE();
    void DrawFolderContentsPanelUVE();
    void DrawFilesystemContextPopupUVE();
    [[nodiscard]] Scripting::ScriptGraphCanvasUVE& ActiveVisualScriptCanvasUVE() noexcept;
    [[nodiscard]] const Scripting::ScriptGraphCanvasUVE& ActiveVisualScriptCanvasUVE() const noexcept;

    Core::EngineServicesUVE* m_services = nullptr;
    Core::ISimulationControlUVE* m_simulationControl = nullptr;
    Core::IEditorViewportHostUVE* m_viewportHost = nullptr;
    EditorStateUVE m_state = EditorStateUVE::Uninitialized;
    EditorPlayModeStateUVE m_playModeState = EditorPlayModeStateUVE::Edit;
    std::optional<PlayModeSessionUVE> m_playModeSession;
    Scene::EntityUVE m_viewportCamera = Scene::kInvalidEntityUVE;
    std::vector<Scene::EntityUVE> m_selectedEntities;
    Scene::EntityUVE m_selectedEntity = Scene::kInvalidEntityUVE;
    std::filesystem::path m_activeScenePath;
    std::size_t m_historyCapacity = 100U;
    EditorGizmoModeUVE m_gizmoMode = EditorGizmoModeUVE::Translate;
    EditorGizmoCoordinateSpaceUVE m_gizmoCoordinateSpace = EditorGizmoCoordinateSpaceUVE::World;
    EditorReparentTransformModeUVE m_reparentTransformMode = EditorReparentTransformModeUVE::KeepLocal;
    EditorTransformSnappingSettingsUVE m_transformSnappingSettings{};
    EditorToolSessionUVE m_toolSession;
    GizmoDragUVE m_gizmoDrag{};
    Math::Vector3UVE m_viewportFocusPoint{0.0F, 1.5F, 0.0F};
    float m_viewportYawRadians = 0.0F;
    float m_viewportPitchRadians = 0.0F;
    float m_viewportDistance = 6.0F;
    bool m_viewportEnvironmentPreviewEnabled = true;
    bool m_viewportSunPreviewEnabled = true;
    float m_viewportPresetTargetYawRadians = 0.0F;
    float m_viewportPresetTargetPitchRadians = 0.0F;
    bool m_viewportPresetAnimating = false;
    EditorViewportNavigationModeUVE m_viewportNavigationMode = EditorViewportNavigationModeUVE::None;
    Editor2DCanvasStateUVE m_2dCanvasState{};
    Math::Vector2UVE m_2dCanvasPanStartPointer{};
    Math::Vector2UVE m_2dCanvasPanStart{};
    bool m_2dCanvasPanning = false;
    std::deque<HistoryEntryUVE> m_undoHistory;
    std::deque<HistoryEntryUVE> m_redoHistory;
    EditorWorkspaceUVE m_activeWorkspace = EditorWorkspaceUVE::Library;
    EditorViewportTabUVE m_viewportTab = EditorViewportTabUVE::Scene;
    /// Transient Plugin window/tool gates. These are editor-session state only and never become ECS
    /// components, serialized scene data, or runtime/plugin activation side effects.
    bool m_pluginWindowVisible = false;
    bool m_controlRigPluginEnabled = false;
    bool m_motionQueryPluginEnabled = false;
    EditorRightPanelTabUVE m_activeRightPanelTab = EditorRightPanelTabUVE::Inspector;
    InspectorDrawerRegistryUVE m_inspectorDrawerRegistry;
    DeveloperConsoleUVE m_developerConsole;
    Scripting::ScriptNodeRegistryUVE m_visualScriptRegistry;
    std::vector<ScriptBranchUVE> m_visualScriptBranches;
    std::size_t m_activeVisualScriptBranch = 0U;
    std::string m_scriptBranchDialogBuffer;
    bool m_scriptBranchDialogRenaming = false;
    EditorBottomDockUVE m_activeBottomDock = EditorBottomDockUVE::FileSystem;
    /// Empty is the ProjectFileIndexUVE content root. This value is session-only and must name a
    /// directory in the latest successful copied snapshot before it is used as a browser location.
    std::filesystem::path m_contentBrowserDirectory;
    /// User-curated shortcuts into the project tree, as project-relative generic paths; both
    /// directories and files may be favorited. Persisted across sessions (see
    /// Save/LoadSessionSettingsUVE). An entry no longer present in the latest snapshot is simply
    /// not shown, never pruned from storage here, so a not-yet-scanned favorite is not lost.
    std::vector<std::filesystem::path> m_favoriteProjectPaths;
    /// True while the Filesystem panel shows the flattened Favorites list instead of the direct
    /// children of m_contentBrowserDirectory.
    bool m_contentBrowserShowingFavorites = false;
    /// Content-derived thumbnail textures for Content Browser entries (currently texture assets
    /// only), keyed by project-relative generic path. A cached 0 means a prior load attempt
    /// failed (not a texture, corrupt, or unsupported format) and callers should fall back to the
    /// generic per-type icon rather than retrying every frame. Cleared whenever the project file
    /// index is successfully refreshed, since on-disk content may have changed.
    std::map<std::string, std::uintptr_t> m_textureThumbnailCache;
    /// Content-derived thumbnail textures for Content Browser mesh entries, rendered on demand by
    /// m_meshThumbnailRenderer. Same caching/invalidation contract as m_textureThumbnailCache.
    std::map<std::string, std::uintptr_t> m_meshThumbnailCache;
    MeshThumbnailRendererUVE m_meshThumbnailRenderer;
    ContentBrowserTypeFocusUVE m_contentBrowserTypeFocus = ContentBrowserTypeFocusUVE::All;
    std::string m_assetFilter;
    std::string m_inspectorFilter;
    std::string m_consoleFilter;
    std::string m_consoleCommand;
    std::string m_hierarchyFilter;
    std::string m_cachedHierarchyFilter;
    std::vector<Scene::EntityUVE> m_cachedHierarchyVisibleEntities;
    Scene::EntityUVE m_hierarchyRenameEntity = Scene::kInvalidEntityUVE;
    std::string m_hierarchyRenameBuffer;
    bool m_hierarchyFilterCacheDirty = true;
    bool m_hierarchyRenameFocusRequested = false;
    std::optional<Asset::AssetRecordUVE> m_selectedAsset;
    std::optional<Asset::ProjectFileEntryUVE> m_selectedProjectFile;
    std::optional<Asset::ProjectFileEntryUVE> m_filesystemContextEntry;
    std::string m_filesystemContextFilter;
    bool m_filesystemContextVisible = false;
    std::filesystem::path m_filesystemLongPressPath;
    float m_filesystemLongPressSeconds = 0.0F;
    bool m_projectFileSnapshotInitialized = false;
    bool m_projectFileLastRefreshSucceeded = true;
    std::uint64_t m_projectFileLastObservedChangeSequence = 0U;
    bool m_projectFileRefreshAttemptedForRescan = false;
    bool m_scenePanelVisible = true;
    bool m_inspectorPanelVisible = true;
    bool m_bottomDockVisible = true;
    bool m_sceneDirty = false;
    bool m_uiInitialized = false;
    EditorUiAssetsUVE m_uiAssets;
    std::uint32_t m_scriptCanvasDragNodeId = 0U;
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasDragStartPosition{};
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasDragStartPointer{};
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasDragPreviewPosition{};
    std::uint64_t m_scriptCanvasDragRevision = 0U;
    bool m_scriptCanvasDragging = false;
    std::uint32_t m_scriptCanvasLinkSourceNodeId = 0U;
    std::string m_scriptCanvasLinkSourcePin;
    std::uint32_t m_scriptCanvasDefaultEditNodeId = 0U;
    std::string m_scriptCanvasDefaultEditPin;
    std::string m_scriptCanvasDefaultEditBuffer;
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasContextMenuPosition{};
    std::string m_scriptCanvasContextFilter;
    bool m_scriptCanvasLongPressPending = false;
    float m_scriptCanvasLongPressSeconds = 0.0F;
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasLongPressStartPointer{};
    bool m_scriptCompileAttempted = false;
    bool m_scriptCompileSucceeded = false;
    std::uint64_t m_scriptLastCompiledGraphRevision = 0U;
    std::size_t m_scriptCompileInstructionCount = 0U;
    std::string m_scriptCompileMessage;
    bool m_scriptCanvasPanning = false;
    Scripting::ScriptGraphCanvasPointUVE m_scriptCanvasPanStart{};
    Scripting::ScriptGraphCanvasViewUVE m_scriptCanvasPanViewStart{};
};

} // namespace UVE::Editor
