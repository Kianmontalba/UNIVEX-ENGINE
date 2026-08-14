// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <optional>

#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Editor {

/// The externally observable lifecycle state of one editor-owned transform tool transaction.
/// Previewing always represents a single selected document entity; it never owns ECS storage,
/// rendering data, input routing, or Undo/Redo history.
enum class EditorToolSessionPhaseUVE {
    Idle,
    Previewing,
};

/// The most recent terminal outcome recorded by an editor transform tool session. RestoreFailed
/// is deliberately distinct from Cancelled: it means the editor cleared its transient ownership but
/// could not prove the preview transform was restored, so callers must not claim atomic rollback.
enum class EditorToolSessionOutcomeUVE {
    None,
    Committed,
    CompletedWithoutChange,
    Cancelled,
    /// The live transform no longer matched this session's last successful preview, so the
    /// captured baseline was intentionally not restored over an external mutation.
    ExternalTransformConflict,
    RestoreFailed,
    Rejected,
};

/// Immutable tool family captured at session begin. It prevents a mode switch from reinterpreting
/// a pointer gesture that is already previewing a transform.
enum class EditorToolSessionModeUVE {
    Translate,
    Rotate,
    Scale,
};

/// A copied transaction baseline and final preview value. EditorUVE consumes this value to apply
/// scene-graph mutations and create its existing TransformHistoryEntryUVE; the session itself
/// remains independent of ECS, history, and UI ownership.
struct EditorToolSessionSnapshotUVE final {
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    EditorToolSessionModeUVE mode = EditorToolSessionModeUVE::Translate;
    Scene::TransformComponentUVE baselineTransform{};
    Scene::TransformComponentUVE lastAppliedTransform{};
    bool baselineDirty = false;
};

/// EditorToolSessionUVE owns the explicit begin/preview/commit/cancel state machine for one
/// transform-tool gesture. A re-entrant Begin while Previewing is an intentional no-op rejection:
/// the existing session remains untouched until it commits or cancels through its own terminal path.
class EditorToolSessionUVE final {
public:
    [[nodiscard]] bool BeginUVE(Scene::EntityUVE entity, EditorToolSessionModeUVE mode,
                                const Scene::TransformComponentUVE& baselineTransform,
                                bool baselineDirty) noexcept;

    /// Records a transform only after EditorUVE has successfully applied it through SceneGraph.
    /// Preview updates never create history entries or terminal outcomes.
    [[nodiscard]] bool RecordPreviewAppliedUVE(
        const Scene::TransformComponentUVE& appliedTransform) noexcept;

    /// Ends a valid preview for history processing. The returned snapshot is copied before the
    /// phase becomes Idle, so EditorUVE can safely record one history transaction without a second
    /// transform write.
    [[nodiscard]] std::optional<EditorToolSessionSnapshotUVE> CommitUVE(bool changed) noexcept;

    /// Ends the active preview only when `currentTransform` still equals the last transform this
    /// session successfully applied. A mismatch is an external live-transform conflict: the session
    /// clears with ExternalTransformConflict and returns no baseline, preventing a stale restore
    /// from overwriting another editor/runtime mutation.
    [[nodiscard]] std::optional<EditorToolSessionSnapshotUVE> CancelUVE(
        const Scene::TransformComponentUVE& currentTransform) noexcept;

    /// Clears an active session when its target is deleted or cannot be read safely. It does not
    /// attempt a restore and records Cancelled because no live transform was available to compare.
    void DiscardUVE() noexcept;
    void MarkRestoreFailedUVE() noexcept;

    void MarkRejectedUVE() noexcept;

    [[nodiscard]] EditorToolSessionPhaseUVE GetPhaseUVE() const noexcept;
    [[nodiscard]] EditorToolSessionOutcomeUVE GetLastOutcomeUVE() const noexcept;
    [[nodiscard]] const std::optional<EditorToolSessionSnapshotUVE>& GetSnapshotUVE() const noexcept;

private:
    std::optional<EditorToolSessionSnapshotUVE> m_snapshot;
    EditorToolSessionOutcomeUVE m_lastOutcome = EditorToolSessionOutcomeUVE::None;
};

} // namespace UVE::Editor
