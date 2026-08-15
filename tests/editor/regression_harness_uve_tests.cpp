// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/regression_harness_uve.h"

#include <gtest/gtest.h>

namespace UVE::Editor {
namespace {

void PopulateRegistryUVE(Scripting::ScriptNodeRegistryUVE& registry) {
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(Scripting::ScriptNodeTypeDescriptorUVE{
        "Start", "Start", {Scripting::ScriptPinDescriptorUVE{
                                 "Out", Scripting::ScriptPinDirectionUVE::Output,
                                 Scripting::ScriptValueTypeUVE::Execution,
                                 Scripting::ScriptPinRoleUVE::Execution}}}));
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(Scripting::ScriptNodeTypeDescriptorUVE{
        "End", "End", {Scripting::ScriptPinDescriptorUVE{
                               "In", Scripting::ScriptPinDirectionUVE::Input,
                               Scripting::ScriptValueTypeUVE::Execution,
                               Scripting::ScriptPinRoleUVE::Execution}}}));
}

EditorAnimationRegressionCaseUVE MakeRegressionCaseUVE() {
    EditorAnimationRegressionCaseUVE regressionCase;
    EXPECT_TRUE(regressionCase.graphSchema.graph.AddNodeUVE(Scripting::ScriptNodeUVE{1U, "Start"}));
    EXPECT_TRUE(regressionCase.graphSchema.graph.AddNodeUVE(Scripting::ScriptNodeUVE{2U, "End"}));
    EXPECT_TRUE(regressionCase.graphSchema.graph.AddLinkUVE(
        Scripting::ScriptLinkUVE{{1U, "Out"}, {2U, "In"}}));
    regressionCase.graphSchema.layout = {
        Scripting::ScriptGraphLayoutEntryUVE{1U, 0.0F, 0.0F},
        Scripting::ScriptGraphLayoutEntryUVE{2U, 300.0F, 0.0F},
    };
    EditorRegressionOperationUVE moveNode;
    moveNode.kind = EditorRegressionOperationKindUVE::MoveNode;
    moveNode.nodeId = 1U;
    moveNode.position = {20.0F, 30.0F};
    EditorRegressionOperationUVE selection;
    selection.kind = EditorRegressionOperationKindUVE::SetSelection;
    selection.selection = {1U, 2U};
    EditorRegressionOperationUVE view;
    view.kind = EditorRegressionOperationKindUVE::SetView;
    view.view = {{5.0F, 2.0F}, 1.25F};
    EditorRegressionOperationUVE undo;
    undo.kind = EditorRegressionOperationKindUVE::Undo;
    EditorRegressionOperationUVE redo;
    redo.kind = EditorRegressionOperationKindUVE::Redo;
    regressionCase.operations = {moveNode, selection, view, undo, redo};
    regressionCase.animationClip.clipId = "walk";
    regressionCase.animationClip.durationSeconds = 1.0;
    regressionCase.animationClip.samples = {
        Core::PoseSampleUVE{0.0, Core::TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        Core::PoseSampleUVE{1.0, Core::TransformPoseUVE{{10.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
    };
    regressionCase.animationSampleTimes = {0.0, 0.25, 0.5, 0.75, 1.0};
    return regressionCase;
}

} // namespace

TEST(EditorRegressionHarnessUVETest, RunEditorAnimationRegressionUVE_CoversNativeReplayAndCopiedDto) {
    Scripting::ScriptNodeRegistryUVE registry;
    PopulateRegistryUVE(registry);
    const EditorRegressionRunResultUVE result =
        RunEditorAnimationRegressionUVE(MakeRegressionCaseUVE(), registry);

    ASSERT_TRUE(result.IsPassedUVE()) << result.message;
    EXPECT_EQ(result.appliedOperationCount, 5U);
    EXPECT_EQ(result.sampledAnimationCount, 5U);
    EXPECT_FALSE(result.deterministicDigest.empty());
    EXPECT_TRUE(result.visualScriptDto.available);
    EXPECT_TRUE(result.visualScriptDto.canEdit);
    EXPECT_EQ(result.visualScriptDto.nodeCount, 2U);
    EXPECT_EQ(result.visualScriptDto.linkCount, 1U);
    EXPECT_EQ(result.visualScriptDto.canvas.nodes.size(), 2U);
}

TEST(EditorRegressionHarnessUVETest, RunEditorAnimationDeterminismUVE_RepeatsStableTrace) {
    Scripting::ScriptNodeRegistryUVE registry;
    PopulateRegistryUVE(registry);
    const EditorRegressionDeterminismResultUVE result =
        RunEditorAnimationDeterminismUVE(MakeRegressionCaseUVE(), registry, 8U);

    ASSERT_TRUE(result.IsPassedUVE()) << result.message;
    EXPECT_EQ(result.requestedIterations, 8U);
    EXPECT_EQ(result.completedIterations, 8U);
    EXPECT_FALSE(result.deterministicDigest.empty());
}

TEST(EditorRegressionHarnessUVETest, RunEditorAnimationRegressionUVE_RejectsInvalidReplayInputs) {
    Scripting::ScriptNodeRegistryUVE registry;
    PopulateRegistryUVE(registry);
    EditorAnimationRegressionCaseUVE invalid = MakeRegressionCaseUVE();
    invalid.animationClip.durationSeconds = 0.0;
    EXPECT_EQ(RunEditorAnimationRegressionUVE(invalid, registry).code,
              EditorRegressionCodeUVE::InvalidCase);

    invalid = MakeRegressionCaseUVE();
    EditorRegressionOperationUVE invalidAdd;
    invalidAdd.kind = EditorRegressionOperationKindUVE::AddNode;
    invalidAdd.nodeId = 99U;
    invalidAdd.nodeTypeId = "Unknown";
    invalidAdd.position = {0.0F, 0.0F};
    invalid.operations.push_back(invalidAdd);
    EXPECT_EQ(RunEditorAnimationRegressionUVE(invalid, registry).code,
              EditorRegressionCodeUVE::GraphEditRejected);
}

TEST(EditorRegressionHarnessUVETest, RunEditorAnimationRegressionUVE_RejectsBoundedStressOverflow) {
    Scripting::ScriptNodeRegistryUVE registry;
    PopulateRegistryUVE(registry);
    EditorAnimationRegressionCaseUVE overflow = MakeRegressionCaseUVE();
    overflow.operations.resize(EditorAnimationRegressionCaseUVE::kMaximumOperationsUVE + 1U);

    const EditorRegressionRunResultUVE result =
        RunEditorAnimationRegressionUVE(overflow, registry);
    EXPECT_EQ(result.code, EditorRegressionCodeUVE::CapacityExceeded);
}

} // namespace UVE::Editor
