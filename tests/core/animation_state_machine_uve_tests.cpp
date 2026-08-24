// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_state_machine_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

AnimationClipUVE MakeClipUVE(const std::string& id, const float start, const float end) {
    AnimationClipUVE clip;
    clip.clipId = id;
    clip.durationSeconds = 1.0;
    clip.samples = {
        PoseSampleUVE{0.0, TransformPoseUVE{{start, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        PoseSampleUVE{1.0, TransformPoseUVE{{end, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
    };
    return clip;
}

AnimationStateMachineUVE MakeMachineUVE() {
    AnimationStateMachineUVE machine;
    machine.initialStateId = "idle";
    machine.clips = {MakeClipUVE("idleClip", 0.0F, 0.0F), MakeClipUVE("runClip", 10.0F, 20.0F)};
    machine.states = {
        AnimationStateUVE{"idle", "idleClip", "locomotion", "upper", 1.0F},
        AnimationStateUVE{"run", "runClip", "locomotion", "upper", 1.0F},
    };
    machine.transitions = {
        AnimationTransitionUVE{"idle_to_run", "idle", "run", 1.0F, 0.25F, 5, AnimationInterruptionPolicyUVE::SourceOrTarget},
    };
    return machine;
}

AnimationStateMachineUVE MakeInterruptibleMachineUVE(const AnimationInterruptionPolicyUVE policy) {
    AnimationStateMachineUVE machine = MakeMachineUVE();
    machine.clips.push_back(MakeClipUVE("jumpClip", 30.0F, 40.0F));
    machine.states.push_back(AnimationStateUVE{"jump", "jumpClip", "locomotion", "upper", 1.0F});
    machine.transitions[0].priority = 10;
    machine.transitions[0].exitTime = 0.0F;
    machine.transitions[0].interruption = policy;
    machine.transitions.push_back(AnimationTransitionUVE{
        "idle_to_jump", "idle", "jump", 1.0F, 0.0F, 9, AnimationInterruptionPolicyUVE::None});
    machine.transitions.push_back(AnimationTransitionUVE{
        "run_to_jump", "run", "jump", 1.0F, 0.0F, 8, AnimationInterruptionPolicyUVE::None});
    return machine;
}

const AnimationStateMachineEvaluationResultUVE BeginTransitionUVE(
    AnimationStateMachineEvaluatorUVE& evaluator) {
    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);
    EXPECT_TRUE(result.IsSuccessUVE());
    return result;
}

} // namespace

TEST(AnimationStateMachineUVETest, ValidateAnimationStateMachineUVE_AcceptsBoundedStatesAndTransition) {
    const AnimationStateMachineValidationResultUVE result = ValidateAnimationStateMachineUVE(MakeMachineUVE());
    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, AnimationStateMachineValidationCodeUVE::Valid);
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_UsesExitTimeCrossfadeAndCopiedDebugSnapshot) {
    const AnimationStateMachineUVE machine = MakeMachineUVE();
    AnimationStateMachineEvaluatorUVE evaluator(machine);
    const AnimationStateMachineEvaluationResultUVE beforeTransition = evaluator.EvaluateUVE(0.25);
    ASSERT_TRUE(beforeTransition.IsSuccessUVE());
    EXPECT_DOUBLE_EQ(beforeTransition.debug.normalizedTime, 0.25);
    EXPECT_EQ(beforeTransition.debug.currentStateId, "idle");
    EXPECT_TRUE(beforeTransition.debug.transitioning);
    EXPECT_EQ(beforeTransition.debug.activeTransitionId, "idle_to_run");
    EXPECT_EQ(beforeTransition.debug.syncGroupId, "locomotion");
    EXPECT_EQ(beforeTransition.debug.layerMaskId, "upper");

    const AnimationStateMachineEvaluationResultUVE crossfade = evaluator.EvaluateUVE(0.25);
    ASSERT_TRUE(crossfade.IsSuccessUVE());
    EXPECT_NEAR(crossfade.debug.transitionAlpha, 0.5, 1.0e-12);
    EXPECT_NEAR(crossfade.pose.position.x, 7.5F, 1.0e-5F);

    static_cast<void>(evaluator.EvaluateUVE(0.25));
    const AnimationStateMachineEvaluationResultUVE completed = evaluator.EvaluateUVE(0.25);
    ASSERT_TRUE(completed.IsSuccessUVE());
    EXPECT_FALSE(completed.debug.transitioning);
    EXPECT_EQ(completed.debug.currentStateId, "run");
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_SelectsHighestPriorityEligibleTransition) {
    AnimationStateMachineUVE machine = MakeMachineUVE();
    machine.transitions.push_back(AnimationTransitionUVE{
        "idle_to_run_low", "idle", "run", 0.0F, 0.0F, 1, AnimationInterruptionPolicyUVE::None});
    machine.transitions[0].priority = 8;
    machine.transitions[0].exitTime = 0.0F;
    AnimationStateMachineEvaluatorUVE evaluator(machine);

    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.debug.activeTransitionId, "idle_to_run");
    EXPECT_EQ(result.debug.selectedTransitionPriority, 8);
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_NoneInterruptionKeepsActiveTransition) {
    const AnimationStateMachineUVE machine =
        MakeInterruptibleMachineUVE(AnimationInterruptionPolicyUVE::None);
    AnimationStateMachineEvaluatorUVE evaluator(machine);
    ASSERT_EQ(BeginTransitionUVE(evaluator).debug.activeTransitionId, "idle_to_run");

    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.debug.activeTransitionId, "idle_to_run");
    EXPECT_EQ(result.debug.targetStateId, "run");
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_SourceInterruptionSelectsSourceTransition) {
    const AnimationStateMachineUVE machine =
        MakeInterruptibleMachineUVE(AnimationInterruptionPolicyUVE::Source);
    AnimationStateMachineEvaluatorUVE evaluator(machine);
    ASSERT_EQ(BeginTransitionUVE(evaluator).debug.activeTransitionId, "idle_to_run");

    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.debug.activeTransitionId, "idle_to_jump");
    EXPECT_EQ(result.debug.targetStateId, "jump");
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_TargetInterruptionSelectsTargetTransition) {
    const AnimationStateMachineUVE machine =
        MakeInterruptibleMachineUVE(AnimationInterruptionPolicyUVE::Target);
    AnimationStateMachineEvaluatorUVE evaluator(machine);
    ASSERT_EQ(BeginTransitionUVE(evaluator).debug.activeTransitionId, "idle_to_run");

    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.debug.activeTransitionId, "run_to_jump");
    EXPECT_EQ(result.debug.targetStateId, "jump");
}

TEST(AnimationStateMachineUVETest, EvaluateUVE_SourceOrTargetUsesPriorityAcrossBothStates) {
    const AnimationStateMachineUVE machine =
        MakeInterruptibleMachineUVE(AnimationInterruptionPolicyUVE::SourceOrTarget);
    AnimationStateMachineEvaluatorUVE evaluator(machine);
    ASSERT_EQ(BeginTransitionUVE(evaluator).debug.activeTransitionId, "idle_to_run");

    const AnimationStateMachineEvaluationResultUVE result = evaluator.EvaluateUVE(0.01);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.debug.activeTransitionId, "idle_to_jump");
    EXPECT_EQ(result.debug.targetStateId, "jump");
    EXPECT_EQ(result.debug.selectedTransitionPriority, 9);
}

TEST(AnimationStateMachineUVETest, ValidateAnimationStateMachineUVE_RejectsUnknownClipAndInvalidExitTime) {
    AnimationStateMachineUVE unknownClip = MakeMachineUVE();
    unknownClip.states[1].clipId = "missing";
    EXPECT_EQ(ValidateAnimationStateMachineUVE(unknownClip).code,
              AnimationStateMachineValidationCodeUVE::UnknownClip);

    AnimationStateMachineUVE invalidExit = MakeMachineUVE();
    invalidExit.transitions[0].exitTime = 1.5F;
    EXPECT_EQ(ValidateAnimationStateMachineUVE(invalidExit).code,
              AnimationStateMachineValidationCodeUVE::InvalidExitTime);
}

} // namespace UVE::Core
