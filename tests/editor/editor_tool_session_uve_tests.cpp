// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include "uve/editor/editor_tool_session_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Scene::TransformComponentUVE MakeTransformUVE(const float x) {
    Scene::TransformComponentUVE transform{};
    transform.localPosition.x = x;
    return transform;
}

TEST(EditorToolSessionUVETest, BeginUVE_CapturesBaselineAndReentrantBeginIsStrictNoOp) {
    EditorToolSessionUVE session;
    const Scene::EntityUVE firstEntity{1U, 1U};
    const Scene::EntityUVE secondEntity{2U, 1U};
    const Scene::TransformComponentUVE baseline = MakeTransformUVE(3.0F);

    ASSERT_TRUE(session.BeginUVE(firstEntity, EditorToolSessionModeUVE::Translate, baseline, true));
    ASSERT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Previewing);
    ASSERT_TRUE(session.GetSnapshotUVE().has_value());
    EXPECT_EQ(session.GetSnapshotUVE()->entity, firstEntity);
    EXPECT_EQ(session.GetSnapshotUVE()->mode, EditorToolSessionModeUVE::Translate);
    EXPECT_EQ(session.GetSnapshotUVE()->baselineTransform.localPosition.x, 3.0F);
    EXPECT_EQ(session.GetSnapshotUVE()->lastAppliedTransform.localPosition.x, 3.0F);
    EXPECT_TRUE(session.GetSnapshotUVE()->baselineDirty);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::None);

    EXPECT_FALSE(session.BeginUVE(secondEntity, EditorToolSessionModeUVE::Scale, MakeTransformUVE(9.0F), false));
    ASSERT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Previewing);
    ASSERT_TRUE(session.GetSnapshotUVE().has_value());
    EXPECT_EQ(session.GetSnapshotUVE()->entity, firstEntity);
    EXPECT_EQ(session.GetSnapshotUVE()->mode, EditorToolSessionModeUVE::Translate);
    EXPECT_EQ(session.GetSnapshotUVE()->baselineTransform.localPosition.x, 3.0F);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::None);
}

TEST(EditorToolSessionUVETest, PreviewAndChangedCommitUVE_ReturnOneCopiedTransactionWithoutFurtherMutation) {
    EditorToolSessionUVE session;
    const Scene::EntityUVE entity{4U, 1U};
    const Scene::TransformComponentUVE baseline = MakeTransformUVE(0.0F);
    const Scene::TransformComponentUVE preview = MakeTransformUVE(5.0F);

    ASSERT_TRUE(session.BeginUVE(entity, EditorToolSessionModeUVE::Rotate, baseline, false));
    ASSERT_TRUE(session.RecordPreviewAppliedUVE(preview));
    const std::optional<EditorToolSessionSnapshotUVE> committed = session.CommitUVE(true);

    ASSERT_TRUE(committed.has_value());
    EXPECT_EQ(committed->entity, entity);
    EXPECT_EQ(committed->baselineTransform.localPosition.x, 0.0F);
    EXPECT_EQ(committed->lastAppliedTransform.localPosition.x, 5.0F);
    EXPECT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Idle);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::Committed);
    EXPECT_FALSE(session.GetSnapshotUVE().has_value());
}

TEST(EditorToolSessionUVETest, ZeroDeltaCommitUVE_CompletesWithoutHistoryOutcome) {
    EditorToolSessionUVE session;
    const Scene::EntityUVE entity{5U, 1U};
    const Scene::TransformComponentUVE baseline = MakeTransformUVE(1.0F);

    ASSERT_TRUE(session.BeginUVE(entity, EditorToolSessionModeUVE::Scale, baseline, true));
    ASSERT_TRUE(session.RecordPreviewAppliedUVE(baseline));
    const std::optional<EditorToolSessionSnapshotUVE> completed = session.CommitUVE(false);

    ASSERT_TRUE(completed.has_value());
    EXPECT_EQ(completed->lastAppliedTransform.localPosition.x, baseline.localPosition.x);
    EXPECT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Idle);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::CompletedWithoutChange);
}

TEST(EditorToolSessionUVETest, CancelUVE_ReturnsExactBaselineAndRestoreFailureIsExplicit) {
    EditorToolSessionUVE session;
    const Scene::EntityUVE entity{6U, 1U};
    const Scene::TransformComponentUVE baseline = MakeTransformUVE(-2.0F);
    const Scene::TransformComponentUVE preview = MakeTransformUVE(8.0F);

    ASSERT_TRUE(session.BeginUVE(entity, EditorToolSessionModeUVE::Translate, baseline, false));
    ASSERT_TRUE(session.RecordPreviewAppliedUVE(preview));
    const std::optional<EditorToolSessionSnapshotUVE> cancelled = session.CancelUVE(preview);

    ASSERT_TRUE(cancelled.has_value());
    EXPECT_EQ(cancelled->baselineTransform.localPosition.x, -2.0F);
    EXPECT_EQ(cancelled->lastAppliedTransform.localPosition.x, 8.0F);
    EXPECT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Idle);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::Cancelled);

    session.MarkRestoreFailedUVE();
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::RestoreFailed);
    EXPECT_FALSE(session.CancelUVE(preview).has_value());
}

TEST(EditorToolSessionUVETest, CancelUVE_ExternalTransformConflictPreservesExternalValue) {
    EditorToolSessionUVE session;
    const Scene::EntityUVE entity{7U, 1U};
    const Scene::TransformComponentUVE baseline = MakeTransformUVE(0.0F);
    const Scene::TransformComponentUVE preview = MakeTransformUVE(2.0F);
    const Scene::TransformComponentUVE externallyModified = MakeTransformUVE(9.0F);

    ASSERT_TRUE(session.BeginUVE(entity, EditorToolSessionModeUVE::Translate, baseline, false));
    ASSERT_TRUE(session.RecordPreviewAppliedUVE(preview));

    EXPECT_FALSE(session.CancelUVE(externallyModified).has_value());
    EXPECT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Idle);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::ExternalTransformConflict);
    EXPECT_FALSE(session.GetSnapshotUVE().has_value());
}

TEST(EditorToolSessionUVETest, InvalidOperationsUVE_RejectWithoutCreatingSession) {
    EditorToolSessionUVE session;
    EXPECT_FALSE(session.BeginUVE(Scene::kInvalidEntityUVE, EditorToolSessionModeUVE::Translate,
                                  Scene::TransformComponentUVE{}, false));
    EXPECT_EQ(session.GetPhaseUVE(), EditorToolSessionPhaseUVE::Idle);
    EXPECT_EQ(session.GetLastOutcomeUVE(), EditorToolSessionOutcomeUVE::Rejected);
    EXPECT_FALSE(session.RecordPreviewAppliedUVE(Scene::TransformComponentUVE{}));
    EXPECT_FALSE(session.CommitUVE(true).has_value());
}

} // namespace
} // namespace UVE::Editor::Tests
