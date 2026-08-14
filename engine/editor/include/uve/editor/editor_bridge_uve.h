// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "uve/editor/editor_uve.h"

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

/// Immutable bridge-visible state. A revision is incremented whenever any field observable through
/// this snapshot changes, whether native ImGui or the bridge initiated that change.
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
};

/// Main-thread adapter over EditorUVE. It supports coexistence with the native ImGui editor: every
/// public entry synchronizes the bridge-visible snapshot before inspecting expectedRevision, so a
/// C# client can observe native changes and cannot apply a stale mutation silently.
class EditorBridgeUVE final {
public:
    explicit EditorBridgeUVE(EditorUVE& editor) noexcept;

    [[nodiscard]] EditorBridgeSnapshotUVE GetSnapshotUVE();
    [[nodiscard]] EditorBridgeResponseUVE DispatchUVE(const EditorBridgeRequestUVE& request);
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
    [[nodiscard]] EditorBridgeHierarchySnapshotUVE CaptureHierarchyUVE();
    [[nodiscard]] EditorBridgeInspectorSnapshotUVE CaptureInspectorUVE() const;
    [[nodiscard]] EditorBridgeContentBrowserSnapshotUVE CaptureContentBrowserUVE();
    [[nodiscard]] static EditorBridgeContentBrowserEntryUVE ToContentEntryUVE(
        const Asset::ProjectFileEntryUVE& entry);
    [[nodiscard]] static std::optional<EditorUVE::ContentBrowserTypeFocusUVE> ParseContentBrowserFocusUVE(
        const std::string& focus) noexcept;

    EditorUVE* m_editor = nullptr;
    std::optional<ObservedStateUVE> m_lastObservedState;
    std::uint64_t m_revision = 0U;
};

} // namespace UVE::Editor
