// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_tree_uve.h"

#include <limits>

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

AnimationTreeUVE MakeBlendTreeUVE() {
    AnimationTreeUVE tree;
    tree.clips = {MakeClipUVE("a", 0.0F, 10.0F), MakeClipUVE("b", 10.0F, 20.0F)};
    tree.nodes = {
        AnimationTreeNodeUVE{1U, AnimationTreeNodeKindUVE::ClipPlayer, "A", "a", {}, 0U, 0U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{2U, AnimationTreeNodeKindUVE::ClipPlayer, "B", "b", {}, 0U, 0U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{3U, AnimationTreeNodeKindUVE::Blend, "Blend", {}, {}, 1U, 2U, 0.25F, 1.0F, true},
        AnimationTreeNodeUVE{4U, AnimationTreeNodeKindUVE::OutputPose, "Output", {}, {}, 3U, 0U, 0.5F, 1.0F, true},
    };
    return tree;
}

} // namespace

TEST(AnimationTreeUVETest, ValidateAnimationTreeUVE_AcceptsClipBlendOutputGraph) {
    const AnimationTreeValidationResultUVE result = ValidateAnimationTreeUVE(MakeBlendTreeUVE());
    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, AnimationTreeValidationCodeUVE::Valid);
}

TEST(AnimationTreeUVETest, EvaluateAnimationTreeUVE_BlendsValidatedClipPosesDeterministically) {
    const AnimationTreeEvaluationResultUVE result = EvaluateAnimationTreeUVE(MakeBlendTreeUVE(), 0.5);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.pose.position, (Math::Vector3UVE{7.5F, 0.0F, 0.0F}));
    EXPECT_EQ(result.evaluatedNodeCount, 4U);
}

TEST(AnimationTreeUVETest, EvaluateAnimationTreeUVE_SelectsTransitionAndPropagatesTimeScale) {
    AnimationTreeUVE tree = MakeBlendTreeUVE();
    tree.nodes = {
        AnimationTreeNodeUVE{1U, AnimationTreeNodeKindUVE::ClipPlayer, "A", "a", {}, 0U, 0U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{2U, AnimationTreeNodeKindUVE::ClipPlayer, "B", "b", {}, 0U, 0U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{3U, AnimationTreeNodeKindUVE::Transition, "Transition", {}, "useB", 1U, 2U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{4U, AnimationTreeNodeKindUVE::TimeScale, "Slow", {}, {}, 3U, 0U, 0.5F, 0.5F, true},
        AnimationTreeNodeUVE{5U, AnimationTreeNodeKindUVE::OutputPose, "Output", {}, {}, 4U, 0U, 0.5F, 1.0F, true},
    };

    const AnimationTreeEvaluationResultUVE result = EvaluateAnimationTreeUVE(
        tree, 0.8, {AnimationTreeParameterUVE{"useB", 1.0F}});

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.pose.position, (Math::Vector3UVE{14.0F, 0.0F, 0.0F}));
    EXPECT_EQ(result.evaluatedNodeCount, 4U);
}

TEST(AnimationTreeUVETest, EvaluateAnimationTreeUVE_RejectsNonFiniteScaledTimeBeforeRecursion) {
    AnimationTreeUVE tree;
    tree.clips = {MakeClipUVE("a", 0.0F, 1.0F)};
    tree.nodes = {
        AnimationTreeNodeUVE{1U, AnimationTreeNodeKindUVE::ClipPlayer, "A", "a", {}, 0U, 0U, 0.5F, 1.0F, true},
        AnimationTreeNodeUVE{2U, AnimationTreeNodeKindUVE::TimeScale, "Scale", {}, {}, 1U, 0U, 0.5F,
                              std::numeric_limits<float>::max(), true},
        AnimationTreeNodeUVE{3U, AnimationTreeNodeKindUVE::OutputPose, "Output", {}, {}, 2U, 0U, 0.5F, 1.0F, true},
    };

    const AnimationTreeEvaluationResultUVE result =
        EvaluateAnimationTreeUVE(tree, std::numeric_limits<double>::max());
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.evaluatedNodeCount, 0U);
}

TEST(AnimationTreeUVETest, ValidateAnimationTreeUVE_RejectsUnknownClipAndCycle) {
    AnimationTreeUVE unknownClip = MakeBlendTreeUVE();
    unknownClip.nodes[0].clipId = "missing";
    EXPECT_EQ(ValidateAnimationTreeUVE(unknownClip).code, AnimationTreeValidationCodeUVE::UnknownClip);

    AnimationTreeUVE cycle = MakeBlendTreeUVE();
    cycle.nodes[2].inputA = 4U;
    EXPECT_EQ(ValidateAnimationTreeUVE(cycle).code, AnimationTreeValidationCodeUVE::CycleDetected);
}

} // namespace UVE::Core
