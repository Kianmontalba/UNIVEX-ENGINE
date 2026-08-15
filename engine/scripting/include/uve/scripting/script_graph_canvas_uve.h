// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_editor_backend_uve.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptGraphCanvasCommandCodeUVE : std::uint8_t {
    Applied = 0,
    Rejected,
    StaleRevision,
    NoHistory,
};

struct ScriptGraphCanvasPointUVE final {
    float x = 0.0F;
    float y = 0.0F;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasPointUVE&) const noexcept = default;
};

struct ScriptGraphCanvasViewUVE final {
    ScriptGraphCanvasPointUVE pan{};
    float zoom = 1.0F;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasViewUVE&) const noexcept = default;
};

struct ScriptGraphCanvasPinSnapshotUVE final {
    std::string name;
    ScriptPinDirectionUVE direction = ScriptPinDirectionUVE::Input;
    ScriptValueTypeUVE type = ScriptValueTypeUVE::Number;
    ScriptPinRoleUVE role = ScriptPinRoleUVE::Data;
    std::optional<std::string> defaultValue;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasPinSnapshotUVE&) const = default;
};

struct ScriptGraphCanvasNodeSnapshotUVE final {
    std::uint32_t id = 0U;
    std::string typeId;
    std::string displayName;
    std::string category = "Uncategorized";
    std::string iconId = "node.default";
    std::uint32_t displayOrder = 0U;
    std::uint32_t presentationFlags = kScriptNodePresentationFlagNoneUVE;
    ScriptGraphCanvasPointUVE position{};
    bool selected = false;
    std::vector<ScriptGraphCanvasPinSnapshotUVE> pins;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasNodeSnapshotUVE&) const = default;
};

struct ScriptGraphCanvasLinkSnapshotUVE final {
    ScriptLinkUVE link;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasLinkSnapshotUVE&) const = default;
};

struct ScriptGraphCanvasCommandResultUVE final {
    ScriptGraphCanvasCommandCodeUVE code = ScriptGraphCanvasCommandCodeUVE::Rejected;
    std::uint64_t revision = 0U;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return code == ScriptGraphCanvasCommandCodeUVE::Applied;
    }
};

inline constexpr std::size_t kMaximumScriptGraphCanvasEntriesUVE = 128U;
inline constexpr std::size_t kMaximumScriptGraphCanvasSelectionUVE = 128U;
inline constexpr float kMinimumScriptGraphCanvasZoomUVE = 0.1F;
inline constexpr float kMaximumScriptGraphCanvasZoomUVE = 8.0F;

struct ScriptGraphCanvasLayoutEntryUVE final {
    std::uint32_t nodeId = 0U;
    ScriptGraphCanvasPointUVE position{};

    [[nodiscard]] bool operator==(const ScriptGraphCanvasLayoutEntryUVE&) const noexcept = default;
};

struct ScriptGraphCanvasLayoutSnapshotUVE final {
    ScriptGraphCanvasViewUVE view{};
    std::vector<ScriptGraphCanvasLayoutEntryUVE> entries;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasLayoutSnapshotUVE&) const noexcept = default;
};

struct ScriptGraphCanvasSnapshotUVE final {
    std::uint64_t revision = 0U;
    std::uint64_t graphRevision = 0U;
    ScriptGraphCanvasViewUVE view{};
    bool nodesTruncated = false;
    bool linksTruncated = false;
    bool paletteTruncated = false;
    bool diagnosticsTruncated = false;
    std::vector<ScriptGraphCanvasNodeSnapshotUVE> nodes;
    std::vector<ScriptGraphCanvasLinkSnapshotUVE> links;
    std::vector<std::uint32_t> selectedNodeIds;
    std::vector<std::string> paletteNodeTypeIds;
    std::vector<ScriptValidationDiagnosticUVE> diagnostics;

    [[nodiscard]] bool operator==(const ScriptGraphCanvasSnapshotUVE&) const = default;
};

class ScriptGraphCanvasUVE final {
public:
    static constexpr std::size_t kDefaultHistoryCapacityUVE = 128U;

    explicit ScriptGraphCanvasUVE(ScriptNodeRegistryUVE& registry,
                                  std::size_t historyCapacity = kDefaultHistoryCapacityUVE);
    ScriptGraphCanvasUVE(const ScriptGraphCanvasUVE&) = delete;
    ScriptGraphCanvasUVE& operator=(const ScriptGraphCanvasUVE&) = delete;

    [[nodiscard]] ScriptGraphCanvasCommandResultUVE AddNodeUVE(
        ScriptNodeUVE node, ScriptGraphCanvasPointUVE position,
        std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE RemoveNodeUVE(
        std::uint32_t nodeId, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE MoveNodeUVE(
        std::uint32_t nodeId, ScriptGraphCanvasPointUVE position,
        std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE AddLinkUVE(
        ScriptLinkUVE link, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE RemoveLinkUVE(
        const ScriptLinkUVE& link, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE SetSelectionUVE(
        std::vector<std::uint32_t> nodeIds, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE SetViewUVE(
        ScriptGraphCanvasViewUVE view, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE UndoUVE(
        std::uint64_t expectedRevision = 0U);
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE RedoUVE(
        std::uint64_t expectedRevision = 0U);

    [[nodiscard]] ScriptGraphCanvasSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] ScriptGraphCanvasLayoutSnapshotUVE GetLayoutSnapshotUVE() const;
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE ApplyLayoutUVE(
        ScriptGraphCanvasLayoutSnapshotUVE layout, std::uint64_t expectedRevision = 0U);
    [[nodiscard]] bool HasNodeUVE(std::uint32_t nodeId) const noexcept;
    [[nodiscard]] std::size_t GetUndoCountUVE() const noexcept;
    [[nodiscard]] std::size_t GetRedoCountUVE() const noexcept;

private:
    using LayoutEntryUVE = ScriptGraphCanvasLayoutEntryUVE;

    struct StateUVE final {
        ScriptGraphUVE graph;
        std::vector<LayoutEntryUVE> layout;
        std::vector<std::uint32_t> selection;
    };

    [[nodiscard]] ScriptGraphCanvasCommandResultUVE MakeResultUVE(
        ScriptGraphCanvasCommandCodeUVE code, std::string message) const;
    [[nodiscard]] bool CheckExpectedRevisionUVE(std::uint64_t expectedRevision) const noexcept;
    [[nodiscard]] bool IsFinitePointUVE(ScriptGraphCanvasPointUVE point) const noexcept;
    [[nodiscard]] bool IsValidViewUVE(ScriptGraphCanvasViewUVE view) const noexcept;
    [[nodiscard]] bool IsSelectionValidUVE(const std::vector<std::uint32_t>& nodeIds) const noexcept;
    [[nodiscard]] const LayoutEntryUVE* FindLayoutUVE(std::uint32_t nodeId) const noexcept;
    [[nodiscard]] LayoutEntryUVE* FindLayoutUVE(std::uint32_t nodeId) noexcept;
    [[nodiscard]] StateUVE CaptureStateUVE() const;
    void RestoreStateUVE(StateUVE state);
    void RecordMutationUVE(StateUVE before);
    void BumpRevisionUVE() noexcept;
    [[nodiscard]] ScriptGraphCanvasCommandResultUVE ValidateAndApplyGraphEditUVE(
        ScriptGraphUVE candidate, std::string operation);

    ScriptNodeRegistryUVE* m_registry = nullptr;
    ScriptGraphEditorBackendUVE m_backend;
    ScriptGraphCanvasViewUVE m_view{};
    std::vector<LayoutEntryUVE> m_layout;
    std::vector<std::uint32_t> m_selection;
    std::uint64_t m_graphRevision = 1U;
    std::size_t m_historyCapacity = kDefaultHistoryCapacityUVE;
    std::uint64_t m_revision = 1U;
    std::deque<StateUVE> m_undo;
    std::deque<StateUVE> m_redo;
};

} // namespace UVE::Scripting
