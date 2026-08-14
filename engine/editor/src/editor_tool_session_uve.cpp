// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_tool_session_uve.h"

namespace UVE::Editor {
namespace {

[[nodiscard]] bool AreTransformsEqualUVE(const Scene::TransformComponentUVE& lhs,
                                         const Scene::TransformComponentUVE& rhs) noexcept {
    return lhs.localPosition.x == rhs.localPosition.x && lhs.localPosition.y == rhs.localPosition.y &&
           lhs.localPosition.z == rhs.localPosition.z && lhs.localRotation.x == rhs.localRotation.x &&
           lhs.localRotation.y == rhs.localRotation.y && lhs.localRotation.z == rhs.localRotation.z &&
           lhs.localRotation.w == rhs.localRotation.w && lhs.localScale.x == rhs.localScale.x &&
           lhs.localScale.y == rhs.localScale.y && lhs.localScale.z == rhs.localScale.z;
}

} // namespace

bool EditorToolSessionUVE::BeginUVE(const Scene::EntityUVE entity, const EditorToolSessionModeUVE mode,
                                    const Scene::TransformComponentUVE& baselineTransform,
                                    const bool baselineDirty) noexcept {
    if (m_snapshot.has_value()) {
        return false;
    }
    if (entity == Scene::kInvalidEntityUVE) {
        m_lastOutcome = EditorToolSessionOutcomeUVE::Rejected;
        return false;
    }

    m_snapshot = EditorToolSessionSnapshotUVE{
        entity,
        mode,
        baselineTransform,
        baselineTransform,
        baselineDirty,
    };
    m_lastOutcome = EditorToolSessionOutcomeUVE::None;
    return true;
}

bool EditorToolSessionUVE::RecordPreviewAppliedUVE(
    const Scene::TransformComponentUVE& appliedTransform) noexcept {
    if (!m_snapshot.has_value()) {
        m_lastOutcome = EditorToolSessionOutcomeUVE::Rejected;
        return false;
    }

    m_snapshot->lastAppliedTransform = appliedTransform;
    return true;
}

std::optional<EditorToolSessionSnapshotUVE> EditorToolSessionUVE::CommitUVE(const bool changed) noexcept {
    if (!m_snapshot.has_value()) {
        m_lastOutcome = EditorToolSessionOutcomeUVE::Rejected;
        return std::nullopt;
    }

    const EditorToolSessionSnapshotUVE completedSnapshot = *m_snapshot;
    m_snapshot.reset();
    m_lastOutcome = changed ? EditorToolSessionOutcomeUVE::Committed
                            : EditorToolSessionOutcomeUVE::CompletedWithoutChange;
    return completedSnapshot;
}

std::optional<EditorToolSessionSnapshotUVE> EditorToolSessionUVE::CancelUVE(
    const Scene::TransformComponentUVE& currentTransform) noexcept {
    if (!m_snapshot.has_value()) {
        return std::nullopt;
    }

    const EditorToolSessionSnapshotUVE cancelledSnapshot = *m_snapshot;
    m_snapshot.reset();
    if (!AreTransformsEqualUVE(currentTransform, cancelledSnapshot.lastAppliedTransform)) {
        m_lastOutcome = EditorToolSessionOutcomeUVE::ExternalTransformConflict;
        return std::nullopt;
    }

    m_lastOutcome = EditorToolSessionOutcomeUVE::Cancelled;
    return cancelledSnapshot;
}

void EditorToolSessionUVE::DiscardUVE() noexcept {
    if (!m_snapshot.has_value()) {
        return;
    }
    m_snapshot.reset();
    m_lastOutcome = EditorToolSessionOutcomeUVE::Cancelled;
}

void EditorToolSessionUVE::MarkRestoreFailedUVE() noexcept {
    m_lastOutcome = EditorToolSessionOutcomeUVE::RestoreFailed;
}

void EditorToolSessionUVE::MarkRejectedUVE() noexcept {
    m_lastOutcome = EditorToolSessionOutcomeUVE::Rejected;
}

EditorToolSessionPhaseUVE EditorToolSessionUVE::GetPhaseUVE() const noexcept {
    return m_snapshot.has_value() ? EditorToolSessionPhaseUVE::Previewing : EditorToolSessionPhaseUVE::Idle;
}

EditorToolSessionOutcomeUVE EditorToolSessionUVE::GetLastOutcomeUVE() const noexcept {
    return m_lastOutcome;
}

const std::optional<EditorToolSessionSnapshotUVE>& EditorToolSessionUVE::GetSnapshotUVE() const noexcept {
    return m_snapshot;
}

} // namespace UVE::Editor
