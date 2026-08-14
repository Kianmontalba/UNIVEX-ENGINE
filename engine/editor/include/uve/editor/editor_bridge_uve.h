// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "uve/editor/editor_uve.h"

namespace UVE::Editor {

inline constexpr std::uint32_t kEditorBridgeProtocolVersionUVE = 1U;

/// Copy-only identity suitable for protocol DTOs. It intentionally mirrors a generational entity
/// handle without exposing entity-manager memory or behavior across a future managed boundary.
struct EditorBridgeEntityRefUVE final {
    std::uint32_t index = Scene::kInvalidEntityUVE.index;
    std::uint32_t generation = Scene::kInvalidEntityUVE.generation;

    [[nodiscard]] constexpr bool IsValidUVE() const noexcept { return index != Scene::kInvalidEntityUVE.index; }
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
};

/// One copied entity fact usable by a UI. The label is intentionally presentation data, while all
/// component and hierarchy ownership remains in the native editor and ECS.
struct EditorBridgeEntitySnapshotUVE final {
    EditorBridgeEntityRefUVE entity;
    std::string displayLabel;

    [[nodiscard]] bool operator==(const EditorBridgeEntitySnapshotUVE&) const = default;
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
    std::optional<EditorBridgeEntityRefUVE> activeEntity;
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
        std::optional<EditorBridgeEntityRefUVE> activeEntity;

        [[nodiscard]] bool operator==(const ObservedStateUVE&) const = default;
    };

    [[nodiscard]] ObservedStateUVE CaptureObservedStateUVE() const;
    void SynchronizeRevisionUVE();
    [[nodiscard]] EditorBridgeSnapshotUVE BuildSnapshotUVE() const;
    [[nodiscard]] EditorBridgeResponseUVE MakeResponseUVE(const EditorBridgeRequestUVE& request, bool applied,
                                                           std::string code, std::string message) const;
    [[nodiscard]] static Scene::EntityUVE ToEntityUVE(EditorBridgeEntityRefUVE entity) noexcept;
    [[nodiscard]] static EditorBridgeEntityRefUVE ToBridgeEntityUVE(Scene::EntityUVE entity) noexcept;
    [[nodiscard]] bool IsSupportedEntityKindUVE(EditorEntityKindUVE kind) const noexcept;

    EditorUVE* m_editor = nullptr;
    std::optional<ObservedStateUVE> m_lastObservedState;
    std::uint64_t m_revision = 0U;
};

} // namespace UVE::Editor
