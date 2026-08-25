// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_autorig_uve.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace UVE::Core {
namespace {

SkeletonDefinitionUVE MakeHumanoidSkeletonUVE() {
    return SkeletonDefinitionUVE{
        "humanoid",
        {
            {"root", ""},
            {"spine", "root"},
            {"upper_arm.L", "spine"},
            {"forearm.L", "upper_arm.L"},
            {"hand.L", "forearm.L"},
            {"upper_arm.R", "spine"},
            {"forearm.R", "upper_arm.R"},
            {"hand.R", "forearm.R"},
            {"thigh.L", "root"},
            {"shin.L", "thigh.L"},
            {"foot.L", "shin.L"},
            {"thigh.R", "root"},
            {"shin.R", "thigh.R"},
            {"foot.R", "shin.R"},
            {"head", "spine"},
        },
    };
}

PoseBufferUVE MakeHumanoidPoseUVE(const SkeletonDefinitionUVE& skeleton) {
    PoseBufferUVE pose;
    pose.skeletonId = skeleton.skeletonId;
    pose.localJoints.reserve(skeleton.joints.size());
    for (std::size_t index = 0U; index < skeleton.joints.size(); ++index) {
        const float x = skeleton.joints[index].jointId.find(".L") != std::string::npos ? -1.0F :
                        skeleton.joints[index].jointId.find(".R") != std::string::npos ? 1.0F : 0.0F;
        pose.localJoints.push_back(TransformPoseUVE{{x, static_cast<float>(index), 0.0F}, {}, {1.0F, 1.0F, 1.0F}});
    }
    return pose;
}

ControlRigAutoRigRequestUVE MakeRequestUVE() {
    ControlRigAutoRigRequestUVE request;
    request.rigId = "humanoid_autorig";
    request.skeleton = MakeHumanoidSkeletonUVE();
    request.referencePose = MakeHumanoidPoseUVE(request.skeleton);
    return request;
}

const ControlRigGeneratedControlUVE* FindGeneratedControlUVE(
    const ControlRigAutoRigUVE& rig, const std::string& controlId) {
    const auto iterator = std::find_if(rig.generatedControls.cbegin(), rig.generatedControls.cend(),
                                       [&controlId](const auto& generated) {
                                           return generated.control.controlId == controlId;
                                       });
    return iterator == rig.generatedControls.cend() ? nullptr : &*iterator;
}

} // namespace

TEST(ControlRigAutoRigUVETest, BuildGeneratesDeterministicHumanoidControlsAndConstraints) {
    ControlRigAutoRigUVE rig;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(MakeRequestUVE(), rig));
    EXPECT_TRUE(ValidateControlRigAutoRigUVE(rig).IsValidUVE());

    const ControlRigGeneratedControlUVE* root = FindGeneratedControlUVE(rig, "ctrl_root");
    const ControlRigGeneratedControlUVE* leftHand = FindGeneratedControlUVE(rig, "ctrl_hand_ik_l");
    const ControlRigGeneratedControlUVE* leftPole = FindGeneratedControlUVE(rig, "ctrl_hand_pole_l");
    const ControlRigGeneratedControlUVE* rightFoot = FindGeneratedControlUVE(rig, "ctrl_foot_ik_r");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(leftHand, nullptr);
    ASSERT_NE(leftPole, nullptr);
    ASSERT_NE(rightFoot, nullptr);
    EXPECT_EQ(root->shape.kind, ControlRigControlShapeKindUVE::Box);
    EXPECT_EQ(leftHand->shape.kind, ControlRigControlShapeKindUVE::Circle);
    EXPECT_EQ(leftPole->shape.kind, ControlRigControlShapeKindUVE::Arrow);
    EXPECT_TRUE(leftHand->animatorVisible);
    EXPECT_TRUE(leftPole->animatorSelectable);
    EXPECT_EQ(leftHand->control.parentControlId, "ctrl_root");

    ASSERT_EQ(rig.runtimeRig.constraints.size(), 4U);
    EXPECT_EQ(rig.runtimeRig.constraints[0].kind, ControlRigConstraintKindUVE::TwoBoneIK);
    EXPECT_EQ(rig.runtimeRig.constraints[0].targetControlId, "ctrl_hand_ik_l");
    EXPECT_EQ(rig.runtimeRig.constraints[0].poleControlId, "ctrl_hand_pole_l");
    EXPECT_EQ(rig.runtimeRig.constraints[2].targetControlId, "ctrl_foot_ik_l");
}

TEST(ControlRigAutoRigUVETest, BuildIsDeterministicAndFailureAtomic) {
    const ControlRigAutoRigRequestUVE request = MakeRequestUVE();
    ControlRigAutoRigUVE first;
    ControlRigAutoRigUVE second;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(request, first));
    ASSERT_TRUE(BuildControlRigAutoRigUVE(request, second));
    EXPECT_EQ(first.rigId, second.rigId);
    EXPECT_EQ(first.roleBindings, second.roleBindings);
    EXPECT_EQ(first.generatedControls, second.generatedControls);
    EXPECT_EQ(first.runtimeRig.controls, second.runtimeRig.controls);
    EXPECT_EQ(first.runtimeRig.constraints, second.runtimeRig.constraints);

    ControlRigAutoRigUVE preserved = first;
    ControlRigAutoRigRequestUVE invalid = request;
    invalid.referencePose.skeletonId = "other";
    EXPECT_FALSE(BuildControlRigAutoRigUVE(invalid, preserved));
    EXPECT_EQ(preserved, first);
}

TEST(ControlRigAutoRigUVETest, ResetAndMirrorPreserveControlSetAndSwapSides) {
    ControlRigAutoRigUVE rig;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(MakeRequestUVE(), rig));
    ControlRigAutoRigUVE modified = rig;
    for (auto& generated : modified.generatedControls) {
        generated.control.pose.position.x += 7.0F;
    }

    ASSERT_TRUE(ResetControlRigAutoRigUVE(modified));
    EXPECT_EQ(modified.generatedControls, rig.generatedControls);

    std::vector<ControlRigControlUVE> mirrored;
    ASSERT_TRUE(MirrorControlRigControlsUVE(rig, rig.runtimeRig.controls, mirrored));
    ASSERT_EQ(mirrored.size(), rig.runtimeRig.controls.size());
    const auto left = std::find_if(mirrored.cbegin(), mirrored.cend(), [](const auto& control) {
        return control.controlId == "ctrl_hand_ik_l";
    });
    ASSERT_NE(left, mirrored.cend());
    EXPECT_NEAR(left->pose.position.x, -FindGeneratedControlUVE(rig, "ctrl_hand_ik_r")->control.pose.position.x,
                1.0e-5F);
}

TEST(ControlRigAutoRigUVETest, BakeCopiesGeneratedDeformControlsIntoSkeletonMatchedPose) {
    ControlRigAutoRigUVE rig;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(MakeRequestUVE(), rig));
    ControlRigUVE evaluated = rig.runtimeRig;
    const auto bone = std::find_if(evaluated.controls.begin(), evaluated.controls.end(), [](const auto& control) {
        return control.controlId == "bone.hand.L";
    });
    ASSERT_NE(bone, evaluated.controls.end());
    bone->pose.position = {42.0F, 3.0F, 1.0F};

    const ControlRigBakeResultUVE result = BakeControlRigPoseToSkeletonUVE(rig, evaluated);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.pose.skeletonId, "humanoid");
    EXPECT_EQ(result.mappedJointCount, rig.skeleton.joints.size());
    const std::size_t handIndex = 4U;
    EXPECT_EQ(result.pose.localJoints[handIndex].position, (Math::Vector3UVE{42.0F, 3.0F, 1.0F}));
}

TEST(ControlRigAutoRigUVETest, ExplicitJointMapSupportsCustomSkeletonNames) {
    ControlRigAutoRigRequestUVE request = MakeRequestUVE();
    const auto rename = [&request](const std::string& from, const std::string& to) {
        for (auto& joint : request.skeleton.joints) {
            if (joint.jointId == from) {
                joint.jointId = to;
            }
            if (joint.parentJointId == from) {
                joint.parentJointId = to;
            }
        }
    };
    rename("root", "pelvis");
    rename("spine", "chest");
    rename("upper_arm.L", "arm_left_upper");
    rename("forearm.L", "arm_left_lower");
    rename("hand.L", "hand_left");
    rename("upper_arm.R", "arm_right_upper");
    rename("forearm.R", "arm_right_lower");
    rename("hand.R", "hand_right");
    rename("thigh.L", "leg_left_upper");
    rename("shin.L", "leg_left_lower");
    rename("foot.L", "foot_left");
    rename("thigh.R", "leg_right_upper");
    rename("shin.R", "leg_right_lower");
    rename("foot.R", "foot_right");
    rename("head", "skull");
    request.referencePose.skeletonId = request.skeleton.skeletonId;
    request.jointMap.rootJointId = "pelvis";
    request.jointMap.spineJointId = "chest";
    request.jointMap.headJointId = "skull";
    request.jointMap.leftUpperArmJointId = "arm_left_upper";
    request.jointMap.leftForearmJointId = "arm_left_lower";
    request.jointMap.leftHandJointId = "hand_left";
    request.jointMap.rightUpperArmJointId = "arm_right_upper";
    request.jointMap.rightForearmJointId = "arm_right_lower";
    request.jointMap.rightHandJointId = "hand_right";
    request.jointMap.leftThighJointId = "leg_left_upper";
    request.jointMap.leftShinJointId = "leg_left_lower";
    request.jointMap.leftFootJointId = "foot_left";
    request.jointMap.rightThighJointId = "leg_right_upper";
    request.jointMap.rightShinJointId = "leg_right_lower";
    request.jointMap.rightFootJointId = "foot_right";

    ControlRigAutoRigUVE rig;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(request, rig));
    ASSERT_NE(FindGeneratedControlUVE(rig, "ctrl_hand_ik_l"), nullptr);
    EXPECT_EQ(rig.runtimeRig.constraints.front().sourceControlId, "bone.arm_left_upper");
}

TEST(ControlRigAutoRigUVETest, ValidationRejectsInvalidShapeAndUnknownDrivenJoint) {
    ControlRigAutoRigUVE rig;
    ASSERT_TRUE(BuildControlRigAutoRigUVE(MakeRequestUVE(), rig));
    rig.generatedControls[0].shape.size = 0.0F;
    EXPECT_EQ(ValidateControlRigAutoRigUVE(rig).code, ControlRigAutoRigValidationCodeUVE::InvalidShape);

    ASSERT_TRUE(BuildControlRigAutoRigUVE(MakeRequestUVE(), rig));
    rig.generatedControls[0].drivenJointId = "missing";
    EXPECT_EQ(ValidateControlRigAutoRigUVE(rig).code,
              ControlRigAutoRigValidationCodeUVE::UnknownDrivenJoint);
}

} // namespace UVE::Core
