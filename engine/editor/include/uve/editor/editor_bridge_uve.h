// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "uve/asset/data_table_registry_uve.h"
#include "uve/asset/data_table_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/scripting/script_debugger_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"
#include "uve/scripting/script_runtime_uve.h"
#include "uve/editor/developer_console_uve.h"
#include "uve/plugins/motion_query_debugging_uve.h"
#include "uve/plugins/motion_query_editor_authoring_uve.h"
#include "uve/plugins/motion_query_live_debug_session_uve.h"

namespace UVE::Editor {

inline constexpr std::uint32_t kEditorBridgeProtocolVersionUVE = 1U;

/// Copy-only identity suitable for protocol DTOs. It intentionally mirrors a generational entity
/// handle without exposing entity-manager memory or behavior across a future managed boundary.
struct EditorBridgeEntityRefUVE final {
    std::uint32_t index = Scene::kInvalidEntityUVE.index;
    std::uint32_t generation = Scene::kInvalidEntityUVE.generation;

    [[nodiscard]] constexpr bool IsValidUVE() const noexcept {
        return Scene::EntityUVE{index, generation} != Scene::kInvalidEntityUVE;
    }
    [[nodiscard]] constexpr bool operator==(const EditorBridgeEntityRefUVE&) const noexcept = default;
};

/// Capabilities are ordered stable protocol facts. A future C# host must only surface controls that
/// are advertised here and must still handle request rejection from an authoritative C++ backend.
enum class EditorBridgeCapabilityUVE : std::uint8_t {
    ReadSnapshot,
    SelectEntity,
    ClearSelection,
    SetSelectedEntityName,
    CreateDocumentEntity,
    Undo,
    Redo,
    ReadHierarchy,
    ReadInspector,
    ReadContentBrowser,
    ToggleEntitySelection,
    SetHierarchyFilter,
    SetContentBrowserDirectory,
    SetContentBrowserFilter,
    SetContentBrowserFocus,
    RefreshContentBrowser,
    SelectContentBrowserEntry,
    ReadViewportSurface,
    ReadVisualScriptCanvas,
    ReadVisualScriptDebugger,
    AddVisualScriptNode,
    RemoveVisualScriptNode,
    MoveVisualScriptNode,
    AddVisualScriptLink,
    RemoveVisualScriptLink,
    SetVisualScriptSelection,
    SetVisualScriptView,
    UndoVisualScript,
    RedoVisualScript,
    ReadDeveloperConsole,
    SubmitDeveloperConsoleCommand,
    ClearDeveloperConsole,
    SetDeveloperConsoleSeverityFilter,
    SetDeveloperConsoleCompletionPrefix,
    MoveDeveloperConsoleHistory,
    SelectDataTablePreview,
    ReadScriptRuntime,
    ReadScriptRuntimeTickDiagnostics,
    SerializeVisualScriptGraph,
    DeserializeVisualScriptGraph,
    AddVisualScriptNodeType,
    SetVisualScriptPinDefault,
    ReadMotionQuery,
    DispatchMotionQueryCommand,
    DispatchMotionQueryDebugCommand,
};

/// The deliberately small v1 request vocabulary. No generic command string is accepted because
/// every mutation must be routed through a named, testable EditorUVE command path.
enum class EditorBridgeRequestKindUVE : std::uint8_t {
    ReadSnapshot,
    SelectEntity,
    ClearSelection,
    SetSelectedEntityName,
    CreateDocumentEntity,
    Undo,
    Redo,
    ToggleEntitySelection,
    SetHierarchyFilter,
    SetContentBrowserDirectory,
    SetContentBrowserFilter,
    SetContentBrowserFocus,
    RefreshContentBrowser,
    SelectContentBrowserEntry,
    ReadViewportSurface,
    ReadVisualScriptCanvas,
    ReadVisualScriptDebugger,
    AddVisualScriptNode,
    RemoveVisualScriptNode,
    MoveVisualScriptNode,
    AddVisualScriptLink,
    RemoveVisualScriptLink,
    SetVisualScriptSelection,
    SetVisualScriptView,
    UndoVisualScript,
    RedoVisualScript,
    ReadDeveloperConsole,
    SubmitDeveloperConsoleCommand,
    ClearDeveloperConsole,
    SetDeveloperConsoleSeverityFilter,
    SetDeveloperConsoleCompletionPrefix,
    MoveDeveloperConsoleHistory,
    SelectDataTablePreview,
    ReadScriptRuntime,
    ReadScriptRuntimeTickDiagnostics,
    SerializeVisualScriptGraph,
    DeserializeVisualScriptGraph,
    AddVisualScriptNodeType,
    SetVisualScriptPinDefault,
    ReadMotionQuery,
    DispatchMotionQueryCommand,
    DispatchMotionQueryDebugCommand,
};

/// Explicitly describes whether this bridge session has a native-owned viewport surface. No raw
/// window, GL context, texture, or input-forwarding handle crosses the bridge boundary.
enum class EditorBridgeViewportSurfaceStateUVE : std::uint8_t {
    Unavailable,
    NativeOwned,
    Detached,
};

struct EditorBridgeViewportSurfaceSnapshotUVE final {
    EditorBridgeViewportSurfaceStateUVE state = EditorBridgeViewportSurfaceStateUVE::Unavailable;
    std::uint64_t generation = 0U;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    bool nativeRendererOwnsSurface = true;
    bool managedAttachAllowed = false;
    std::string reason;

    [[nodiscard]] bool operator==(const EditorBridgeViewportSurfaceSnapshotUVE&) const = default;
};

/// One copied entity fact usable by a UI. The label is intentionally presentation data, while all
/// component and hierarchy ownership remains in the native editor and ECS.
struct EditorBridgeEntitySnapshotUVE final {
    EditorBridgeEntityRefUVE entity;
    std::string displayLabel;

    [[nodiscard]] bool operator==(const EditorBridgeEntitySnapshotUVE&) const = default;
};

/// The bridge deliberately bounds presentation records before serializing them. A client must show
/// the explicit truncation fact and never infer that an omitted entry was deleted from the editor.
inline constexpr std::size_t kEditorBridgeMaximumPanelEntriesUVE = 128U;
inline constexpr std::size_t kEditorBridgeMaximumScriptRuntimeTickHistoryUVE = 8U;
inline constexpr std::size_t kEditorBridgeMaximumPresentationTextBytesUVE = 256U;
/// Relative content paths double as native-validated request identities, so they use a separate
/// conservative bound rather than the shorter display-text bound. 128 such rows remain well below
/// the framed protocol's 1 MiB maximum after JSON encoding.
inline constexpr std::size_t kEditorBridgeMaximumContentPathBytesUVE = 4096U;

/// One depth-first copied Scene Outliner row. Parentage stays value-only and C++ remains the sole
/// owner of graph traversal, document membership, selection, and type-tag priority.
struct EditorBridgeHierarchyEntryUVE final {
    EditorBridgeEntityRefUVE entity;
    std::optional<EditorBridgeEntityRefUVE> parent;
    std::string displayLabel;
    std::string typeTag;
    std::size_t depth = 0U;
    std::size_t childCount = 0U;
    bool selected = false;
    bool active = false;

    [[nodiscard]] bool operator==(const EditorBridgeHierarchyEntryUVE&) const = default;
};

struct EditorBridgeHierarchySnapshotUVE final {
    std::string filter;
    bool filterActive = false;
    bool truncated = false;
    std::vector<EditorBridgeHierarchyEntryUVE> entries;

    [[nodiscard]] bool operator==(const EditorBridgeHierarchySnapshotUVE&) const = default;
};

enum class EditorBridgeInspectorModeUVE : std::uint8_t {
    NoSelection,
    MultiSelection,
    SingleSelection,
};

/// Read-only inspector context. Drawer identifiers are copied stable native registry facts, not
/// reflection metadata or a managed authorization to edit arbitrary components.
struct EditorBridgeInspectorSnapshotUVE final {
    EditorBridgeInspectorModeUVE mode = EditorBridgeInspectorModeUVE::NoSelection;
    bool selectedEntitiesTruncated = false;
    std::vector<EditorBridgeEntitySnapshotUVE> selectedEntities;
    std::optional<EditorBridgeEntitySnapshotUVE> activeEntity;
    std::optional<EditorBridgeEntitySnapshotUVE> parent;
    std::vector<EditorBridgeEntitySnapshotUVE> ancestry;
    std::vector<std::string> eligibleDrawerIds;
    bool canEditSelectedName = false;

    [[nodiscard]] bool operator==(const EditorBridgeInspectorSnapshotUVE&) const = default;
};

/// One cached project-content row. This contains no absolute path traversal authority and no file
/// handle; `relativePath` is relative to the native index's configured content root.
struct EditorBridgeContentBrowserEntryUVE final {
    std::string relativePath;
    bool isDirectory = false;
    std::string typeLabel;
    std::optional<std::uint64_t> registeredAssetGuid;

    [[nodiscard]] bool operator==(const EditorBridgeContentBrowserEntryUVE&) const = default;
};

/// Copied current-folder view over the native ProjectFileIndexUVE cache. UI filtering, navigation,
/// refresh, and selection all remain C++ editor session state and never trigger managed filesystem I/O.
struct EditorBridgeContentBrowserSnapshotUVE final {
    std::string contentRoot;
    std::string currentDirectory;
    std::string filter;
    std::string typeFocus;
    std::vector<std::string> breadcrumbs;
    std::uint64_t refreshGeneration = 0U;
    std::size_t visibleEntryCount = 0U;
    std::size_t directEntryCount = 0U;
    bool contentRootExists = false;
    bool initialized = false;
    bool lastRefreshSucceeded = true;
    bool truncated = false;
    std::vector<EditorBridgeContentBrowserEntryUVE> entries;
    std::optional<EditorBridgeContentBrowserEntryUVE> selectedEntry;

    [[nodiscard]] bool operator==(const EditorBridgeContentBrowserSnapshotUVE&) const = default;
};

/// Copied visual-scripting presentation facts. The managed host receives counts and capability state,
/// never native graph objects or runtime ownership. Editing remains a separately named native command path.
struct EditorBridgeVisualScriptDebuggerSnapshotUVE final {
    bool available = false;
    Scripting::ScriptDebuggerStateUVE state = Scripting::ScriptDebuggerStateUVE::Detached;
    std::size_t instructionIndex = 0U;
    std::uint32_t sourceNodeId = 0U;
    std::size_t executedInstructions = 0U;
    std::string pauseReason;
    std::vector<std::uint32_t> breakpointNodeIds;
    std::string reason;

    [[nodiscard]] bool operator==(const EditorBridgeVisualScriptDebuggerSnapshotUVE&) const = default;
};

struct EditorBridgeVisualScriptingSnapshotUVE final {
    bool available = false;
    std::uint64_t graphRevision = 0U;
    std::size_t nodeCount = 0U;
    std::size_t linkCount = 0U;
    bool canEdit = false;
    std::string reason;
    Scripting::ScriptGraphCanvasSnapshotUVE canvas;
    EditorBridgeVisualScriptDebuggerSnapshotUVE debugger;

    [[nodiscard]] bool operator==(const EditorBridgeVisualScriptingSnapshotUVE&) const = default;
};

/// Immutable bridge-visible state. A revision is incremented whenever any field observable through
/// this snapshot changes, whether native ImGui or the bridge initiated that change.
struct EditorBridgeDeveloperConsoleSnapshotUVE final {
    DeveloperConsoleSnapshotUVE console;
    [[nodiscard]] bool operator==(const EditorBridgeDeveloperConsoleSnapshotUVE&) const = default;
};

struct EditorBridgeDataTableCatalogEntryUVE final {
    std::string name;
    std::uint64_t generation = 0U;
    std::size_t columnCount = 0U;
    std::size_t rowCount = 0U;
    bool valid = false;
    [[nodiscard]] bool operator==(const EditorBridgeDataTableCatalogEntryUVE&) const = default;
};

struct EditorBridgeDataTableCatalogSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool entriesTruncated = false;
    std::vector<EditorBridgeDataTableCatalogEntryUVE> entries;
    [[nodiscard]] bool operator==(const EditorBridgeDataTableCatalogSnapshotUVE&) const = default;
};

struct EditorBridgeDataTablePreviewColumnUVE final {
    std::string name;
    Asset::DataTableColumnTypeUVE type = Asset::DataTableColumnTypeUVE::String;
    [[nodiscard]] bool operator==(const EditorBridgeDataTablePreviewColumnUVE&) const = default;
};

struct EditorBridgeDataTablePreviewRowUVE final {
    std::string identifier;
    std::vector<std::string> values;
    [[nodiscard]] bool operator==(const EditorBridgeDataTablePreviewRowUVE&) const = default;
};

struct EditorBridgeScriptRuntimeInstanceEntryUVE final {
    std::uint32_t entityIndex = Scene::kInvalidEntityUVE.index;
    std::uint32_t entityGeneration = Scene::kInvalidEntityUVE.generation;
    std::uint64_t generation = 0U;
    std::uint32_t programVersion = 0U;
    std::size_t instructionCount = 0U;
    std::size_t stateValueCount = 0U;
    bool enabled = false;

    [[nodiscard]] bool operator==(const EditorBridgeScriptRuntimeInstanceEntryUVE&) const = default;
};

struct EditorBridgeScriptRuntimeSnapshotUVE final {
    bool available = false;
    std::size_t instanceCount = 0U;
    bool entriesTruncated = false;
    std::string reason;
    std::vector<EditorBridgeScriptRuntimeInstanceEntryUVE> entries;

    [[nodiscard]] bool operator==(const EditorBridgeScriptRuntimeSnapshotUVE&) const = default;
};

/// Copied counters from one explicitly requested native ScriptRuntime diagnostic tick. The managed
/// host may request this DTO but never executes VM work or receives a runtime pointer.
struct EditorBridgeScriptRuntimeTickSummaryUVE final {
    bool available = false;
    std::string reason = "No ScriptRuntime diagnostic tick has been requested.";
    std::size_t enabledInstanceCount = 0U;
    std::size_t completedCount = 0U;
    std::size_t instructionBudgetExceededCount = 0U;
    std::size_t invalidInstructionCount = 0U;
    std::size_t diagnosticCount = 0U;

    [[nodiscard]] bool operator==(const EditorBridgeScriptRuntimeTickSummaryUVE&) const = default;
};

struct EditorBridgeScriptRuntimeTickHistoryEntryUVE final {
    std::uint64_t sequence = 0U;
    EditorBridgeScriptRuntimeTickSummaryUVE summary;

    [[nodiscard]] bool operator==(const EditorBridgeScriptRuntimeTickHistoryEntryUVE&) const = default;
};

struct EditorBridgeDataTablePreviewSnapshotUVE final {
    bool available = false;
    std::uint64_t generation = 0U;
    std::string name;
    std::size_t totalColumnCount = 0U;
    std::size_t totalRowCount = 0U;
    bool columnsTruncated = false;
    bool rowsTruncated = false;
    bool valuesTruncated = false;
    std::string reason;
    std::vector<EditorBridgeDataTablePreviewColumnUVE> columns;
    std::vector<EditorBridgeDataTablePreviewRowUVE> rows;
    [[nodiscard]] bool operator==(const EditorBridgeDataTablePreviewSnapshotUVE&) const = default;
};

struct EditorBridgeMotionQueryAuthoringSnapshotUVE final {
    std::uint64_t revision = 0U;
    std::optional<Asset::ResourceHandleUVE> selectedResource;
    std::vector<Plugins::Editor::MotionQueryEditorDatabaseRowUVE> databases;
    std::string diagnostic;

    [[nodiscard]] bool operator==(const EditorBridgeMotionQueryAuthoringSnapshotUVE&) const = default;
};

struct EditorBridgeMotionQueryDebuggerSnapshotUVE final {
    bool attached = false;
    std::uint64_t generation = 0U;
    std::optional<Asset::ResourceHandleUVE> database;
    std::optional<std::size_t> selectedCandidateIndex;
    std::size_t candidateCount = 0U;
    std::size_t candidatesEvaluated = 0U;
    float selectedCost = 0.0F;
    std::string selectedCandidateId;
    std::string selectedSourceClipId;
    std::uint8_t qualityTier = 0U;
    std::uint8_t continuityCode = 0U;
    bool continuityApplied = false;
    std::uint8_t transitionCode = 0U;
    bool transitionHeldPrevious = false;
    std::uint8_t telemetryCode = 0U;
    std::size_t telemetryIndexEntryCount = 0U;
    std::size_t telemetryCandidatesConsidered = 0U;
    bool telemetryBudgetSaturated = false;
    std::string provenance;
    std::string message;

    [[nodiscard]] bool operator==(const EditorBridgeMotionQueryDebuggerSnapshotUVE&) const = default;
};

struct EditorBridgeMotionQueryTraceSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool truncated = false;
    std::vector<Plugins::Editor::MotionQueryTraceEventUVE> events;

    [[nodiscard]] bool operator==(const EditorBridgeMotionQueryTraceSnapshotUVE&) const = default;
};

struct EditorBridgeMotionQuerySnapshotUVE final {
    EditorBridgeMotionQueryAuthoringSnapshotUVE authoring;
    EditorBridgeMotionQueryDebuggerSnapshotUVE debugger;
    EditorBridgeMotionQueryTraceSnapshotUVE trace;
    bool liveDebugActive = false;
    std::uint64_t liveDebugGeneration = 0U;
    std::optional<Asset::ResourceHandleUVE> liveDebugDatabase;
    std::string liveDebugFilter;
    std::size_t liveDebugTotalTraceEventCount = 0U;
    std::size_t liveDebugVisibleTraceEventCount = 0U;
    bool liveDebugTraceTruncated = false;
    std::string liveDebugDiagnostic;

    [[nodiscard]] bool operator==(const EditorBridgeMotionQuerySnapshotUVE&) const = default;
};

struct EditorBridgeSnapshotUVE final {
    std::uint32_t protocolVersion = kEditorBridgeProtocolVersionUVE;
    std::uint64_t revision = 0U;
    EditorStateUVE editorState = EditorStateUVE::Uninitialized;
    EditorPlayModeStateUVE playModeState = EditorPlayModeStateUVE::Edit;
    bool sceneDirty = false;
    bool canUndo = false;
    bool canRedo = false;
    std::filesystem::path activeScenePath;
    std::vector<EditorBridgeEntitySnapshotUVE> selectedEntities;
    bool selectedEntitiesTruncated = false;
    std::optional<EditorBridgeEntityRefUVE> activeEntity;
    EditorBridgeHierarchySnapshotUVE hierarchy;
    EditorBridgeInspectorSnapshotUVE inspector;
    EditorBridgeContentBrowserSnapshotUVE contentBrowser;
    EditorBridgeViewportSurfaceSnapshotUVE viewportSurface;
    EditorBridgeVisualScriptingSnapshotUVE visualScripting;
    EditorBridgeDeveloperConsoleSnapshotUVE developerConsole;
    EditorBridgeScriptRuntimeSnapshotUVE scriptRuntime;
    EditorBridgeScriptRuntimeTickSummaryUVE scriptRuntimeTickSummary;
    bool scriptRuntimeTickHistoryTruncated = false;
    std::vector<EditorBridgeScriptRuntimeTickHistoryEntryUVE> scriptRuntimeTickHistory;
    EditorBridgeDataTableCatalogSnapshotUVE dataTableCatalog;
    EditorBridgeDataTablePreviewSnapshotUVE dataTablePreview;
    EditorBridgeMotionQuerySnapshotUVE motionQuery;
    std::vector<EditorBridgeCapabilityUVE> capabilities;
};

/// A value-only request. `expectedRevision` protects a future C# UI from applying a mutation based
/// on a stale copied snapshot after native ImGui or another bridge request changed visible state.
struct EditorBridgeRequestUVE final {
    std::uint32_t protocolVersion = kEditorBridgeProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    std::uint64_t expectedRevision = 0U;
    EditorBridgeRequestKindUVE kind = EditorBridgeRequestKindUVE::ReadSnapshot;
    std::optional<EditorBridgeEntityRefUVE> entity;
    std::optional<std::string> entityName;
    std::optional<EditorEntityKindUVE> entityKind;
    std::optional<std::string> hierarchyFilter;
    std::optional<std::string> contentDirectory;
    std::optional<std::string> contentFilter;
    std::optional<std::string> contentFocus;
    std::optional<std::string> contentEntryPath;
    std::optional<std::uint32_t> visualScriptNodeId;
    std::optional<Scripting::ScriptNodeUVE> visualScriptNode;
    std::optional<std::string> visualScriptNodeTypeId;
    std::optional<Scripting::ScriptGraphCanvasPointUVE> visualScriptPosition;
    std::optional<Scripting::ScriptLinkUVE> visualScriptLink;
    std::optional<std::vector<std::uint32_t>> visualScriptSelection;
    std::optional<Scripting::ScriptGraphCanvasViewUVE> visualScriptView;
    std::optional<std::string> visualScriptGraphSchema;
    std::optional<std::string> dataTableName;
    std::optional<std::string> developerConsoleCommand;
    std::optional<DeveloperConsoleSeverityFilterUVE> developerConsoleSeverityFilter;
    std::optional<std::string> developerConsoleCompletionPrefix;
    std::optional<std::int32_t> developerConsoleHistoryDelta;
    std::optional<std::string> visualScriptPinName;
    std::optional<std::string> visualScriptDefaultValue;
    std::optional<Plugins::Editor::MotionQueryEditorCommandUVE> motionQueryCommand;
    std::optional<Plugins::Editor::MotionQueryLiveDebugCommandUVE> motionQueryDebugCommand;

    EditorBridgeRequestUVE() = default;

    /// Retains source compatibility with the original named-command request shape. New panel
    /// payloads stay opt-in fields so existing callers cannot accidentally reinterpret a request.
    EditorBridgeRequestUVE(std::uint32_t inProtocolVersion, std::uint64_t inRequestId,
                           std::uint64_t inExpectedRevision, EditorBridgeRequestKindUVE inKind,
                           std::optional<EditorBridgeEntityRefUVE> inEntity,
                           std::optional<std::string> inEntityName,
                           std::optional<EditorEntityKindUVE> inEntityKind)
        : protocolVersion(inProtocolVersion),
          requestId(inRequestId),
          expectedRevision(inExpectedRevision),
          kind(inKind),
          entity(std::move(inEntity)),
          entityName(std::move(inEntityName)),
          entityKind(inEntityKind) {}
};

/// A deterministic response. `code` is a stable machine-readable protocol identifier and `message`
/// is safe human-facing context. The copied snapshot is always freshly synchronized before return.
struct EditorBridgeResponseUVE final {
    std::uint32_t protocolVersion = kEditorBridgeProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    bool applied = false;
    std::string code;
    std::string message;
    EditorBridgeSnapshotUVE snapshot;
    std::optional<EditorBridgeEntityRefUVE> createdEntity;
    std::optional<Scripting::ScriptGraphSchemaUVE> visualScriptGraphSchema;
};

/// Main-thread adapter over EditorUVE. It supports coexistence with the native ImGui editor: every
/// public entry synchronizes the bridge-visible snapshot before inspecting expectedRevision, so a
/// C# client can observe native changes and cannot apply a stale mutation silently.
class EditorBridgeUVE final {
public:
    /// The optional registry is non-owning and must outlive this bridge. When supplied, it is the
    /// authoritative source for catalog facts and selected preview snapshots; the legacy injection
    /// seams remain available only for bridge sessions without a registry dependency.
    explicit EditorBridgeUVE(EditorUVE& editor,
                             const Asset::DataTableRegistryUVE* dataTableRegistry = nullptr,
                             const Scripting::ScriptDebuggerUVE* scriptDebugger = nullptr,
                             const Scripting::ScriptRuntimeUVE* scriptRuntime = nullptr) noexcept;

    [[nodiscard]] EditorBridgeSnapshotUVE GetSnapshotUVE();
    [[nodiscard]] EditorBridgeResponseUVE DispatchUVE(const EditorBridgeRequestUVE& request);
    /// Selects a registry-owned table for read-only preview. An empty name clears the selection.
    /// Unknown or overlong names are rejected without changing the current selection.
    [[nodiscard]] bool SetPreviewTableUVE(std::string_view name);
    void SetDataTableCatalogSnapshotUVE(Asset::DataTableCatalogSnapshotUVE snapshot);
    void SetDataTablePreviewSnapshotUVE(Asset::DataTableSnapshotUVE snapshot);

    [[nodiscard]] static const std::vector<EditorBridgeCapabilityUVE>& GetCapabilitiesUVE() noexcept;

private:
    struct ObservedStateUVE final {
        EditorStateUVE editorState = EditorStateUVE::Uninitialized;
        EditorPlayModeStateUVE playModeState = EditorPlayModeStateUVE::Edit;
        bool sceneDirty = false;
        bool canUndo = false;
        bool canRedo = false;
        std::filesystem::path activeScenePath;
        std::vector<EditorBridgeEntitySnapshotUVE> selectedEntities;
        bool selectedEntitiesTruncated = false;
        std::optional<EditorBridgeEntityRefUVE> activeEntity;
        EditorBridgeHierarchySnapshotUVE hierarchy;
        EditorBridgeInspectorSnapshotUVE inspector;
        EditorBridgeContentBrowserSnapshotUVE contentBrowser;
        EditorBridgeViewportSurfaceSnapshotUVE viewportSurface;
        EditorBridgeVisualScriptingSnapshotUVE visualScripting;
        EditorBridgeDeveloperConsoleSnapshotUVE developerConsole;
        EditorBridgeScriptRuntimeSnapshotUVE scriptRuntime;
        EditorBridgeDataTableCatalogSnapshotUVE dataTableCatalog;
        EditorBridgeDataTablePreviewSnapshotUVE dataTablePreview;

        [[nodiscard]] bool operator==(const ObservedStateUVE&) const = default;
    };

    [[nodiscard]] ObservedStateUVE CaptureObservedStateUVE();
    void SynchronizeRevisionUVE();
    [[nodiscard]] EditorBridgeSnapshotUVE BuildSnapshotUVE() const;
    [[nodiscard]] EditorBridgeResponseUVE MakeResponseUVE(const EditorBridgeRequestUVE& request, bool applied,
                                                           std::string code, std::string message) const;
    [[nodiscard]] static Scene::EntityUVE ToEntityUVE(EditorBridgeEntityRefUVE entity) noexcept;
    [[nodiscard]] static EditorBridgeEntityRefUVE ToBridgeEntityUVE(Scene::EntityUVE entity) noexcept;
    [[nodiscard]] bool IsSupportedEntityKindUVE(EditorEntityKindUVE kind) const noexcept;
    [[nodiscard]] static std::string BoundPresentationTextUVE(std::string value);
    [[nodiscard]] static std::string BoundContentPathUVE(std::string value);
    [[nodiscard]] EditorBridgeVisualScriptingSnapshotUVE CaptureVisualScriptingUVE() const;
    [[nodiscard]] EditorBridgeScriptRuntimeSnapshotUVE CaptureScriptRuntimeUVE() const;
    [[nodiscard]] EditorBridgeDeveloperConsoleSnapshotUVE CaptureDeveloperConsoleUVE() const;
    [[nodiscard]] EditorBridgeDataTableCatalogSnapshotUVE CaptureDataTableCatalogUVE() const;
    [[nodiscard]] EditorBridgeDataTablePreviewSnapshotUVE CaptureDataTablePreviewUVE() const;
    [[nodiscard]] EditorBridgeMotionQuerySnapshotUVE CaptureMotionQueryUVE() const;
    [[nodiscard]] EditorBridgeHierarchySnapshotUVE CaptureHierarchyUVE();
    [[nodiscard]] EditorBridgeInspectorSnapshotUVE CaptureInspectorUVE() const;
    [[nodiscard]] EditorBridgeContentBrowserSnapshotUVE CaptureContentBrowserUVE();
    [[nodiscard]] static EditorBridgeContentBrowserEntryUVE ToContentEntryUVE(
        const Asset::ProjectFileEntryUVE& entry);
    [[nodiscard]] static std::optional<EditorUVE::ContentBrowserTypeFocusUVE> ParseContentBrowserFocusUVE(
        const std::string& focus) noexcept;

    EditorUVE* m_editor = nullptr;
    Scripting::ScriptNodeRegistryUVE m_visualScriptRegistry;
    Scripting::ScriptGraphCanvasUVE m_visualScriptCanvas;
    DeveloperConsoleUVE m_developerConsole;
    const Asset::DataTableRegistryUVE* m_dataTableRegistry = nullptr;
    const Scripting::ScriptDebuggerUVE* m_scriptDebugger = nullptr;
    const Scripting::ScriptRuntimeUVE* m_scriptRuntime = nullptr;
    std::optional<std::string> m_dataTablePreviewName;
    Asset::DataTableCatalogSnapshotUVE m_dataTableCatalogSnapshot;
    Asset::DataTableSnapshotUVE m_dataTablePreviewSnapshot;
    std::optional<ObservedStateUVE> m_lastObservedState;
    EditorBridgeScriptRuntimeTickSummaryUVE m_lastScriptRuntimeTickSummary;
    std::deque<EditorBridgeScriptRuntimeTickHistoryEntryUVE> m_scriptRuntimeTickHistory;
    bool m_scriptRuntimeTickHistoryTruncated = false;
    std::uint64_t m_nextScriptRuntimeTickSequence = 1U;
    Plugins::Editor::MotionQueryEditorAuthoringSessionUVE m_motionQueryAuthoring;
    Plugins::Editor::MotionQueryLiveDebugSessionUVE m_motionQueryLiveDebugSession;
    std::uint64_t m_revision = 0U;
};

} // namespace UVE::Editor
