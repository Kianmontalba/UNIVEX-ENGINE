// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/retarget_map_uve.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_set>

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

RetargetMapUVE MakeValidMapUVE() {
    RetargetMapUVE map;
    map.mapId = "humanoid_source_map";
    map.sourceSkeletonId = "source_humanoid";
    map.coordinateSystem = "right_handed_y_up";
    map.units = "meters";
    map.rootBoneId = "root";
    map.isAPose = true;
    map.entries = {
        RetargetMapBoneUVE{"root", {}, "root", RetargetMapBoneCategoryUVE::Core, {0.0F, 0.0F, 0.0F}},
        RetargetMapBoneUVE{"spine", "root", "spine", RetargetMapBoneCategoryUVE::Core, {0.0F, 1.0F, 0.0F}},
        RetargetMapBoneUVE{"spine_helper", "spine", {}, RetargetMapBoneCategoryUVE::CorrectiveHelper,
                           {0.0F, 1.2F, 0.0F}},
    };
    return map;
}

SkeletonDefinitionUVE MakeMatchingSkeletonUVE() {
    return SkeletonDefinitionUVE{
        "source_humanoid",
        {SkeletonJointUVE{"root", {}}, SkeletonJointUVE{"spine", "root"},
         SkeletonJointUVE{"spine_helper", "spine"}}};
}

} // namespace

TEST(RetargetMapUVETest, ValidateRetargetMapUVE_AcceptsVersionedAssetDrivenSourceData) {
    const RetargetMapValidationResultUVE result = ValidateRetargetMapUVE(MakeValidMapUVE());

    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, RetargetMapValidationCodeUVE::Valid);
}

TEST(RetargetMapUVETest, ValidateRetargetMapAgainstSkeletonUVE_OnlyMatchesExistingDefinition) {
    const RetargetMapUVE map = MakeValidMapUVE();
    const SkeletonDefinitionUVE skeleton = MakeMatchingSkeletonUVE();

    EXPECT_TRUE(ValidateRetargetMapAgainstSkeletonUVE(map, skeleton).IsValidUVE());
}

TEST(RetargetMapUVETest, ValidateRetargetMapUVE_RejectsIncompleteMetadataAndCoreRole) {
    RetargetMapUVE missingMetadata = MakeValidMapUVE();
    missingMetadata.units.clear();
    EXPECT_EQ(ValidateRetargetMapUVE(missingMetadata).code,
              RetargetMapValidationCodeUVE::InvalidMetadata);

    RetargetMapUVE missingCoreRole = MakeValidMapUVE();
    missingCoreRole.entries[1].genericName.clear();
    EXPECT_EQ(ValidateRetargetMapUVE(missingCoreRole).code,
              RetargetMapValidationCodeUVE::MissingGenericName);
}

TEST(RetargetMapUVETest, ValidateRetargetMapUVE_RejectsBrokenHierarchy) {
    RetargetMapUVE unknownParent = MakeValidMapUVE();
    unknownParent.entries[2].parentBoneId = "missing";
    EXPECT_EQ(ValidateRetargetMapUVE(unknownParent).code,
              RetargetMapValidationCodeUVE::UnknownParent);

    RetargetMapUVE cyclic = MakeValidMapUVE();
    cyclic.entries[1].parentBoneId = "spine_helper";
    EXPECT_EQ(ValidateRetargetMapUVE(cyclic).code,
              RetargetMapValidationCodeUVE::CyclicHierarchy);

    RetargetMapUVE duplicate = MakeValidMapUVE();
    duplicate.entries.push_back(duplicate.entries[1]);
    EXPECT_EQ(ValidateRetargetMapUVE(duplicate).code,
              RetargetMapValidationCodeUVE::DuplicateBone);
}

TEST(RetargetMapUVETest, ValidateRetargetMapAgainstSkeletonUVE_RejectsDifferentAsset) {
    RetargetMapUVE map = MakeValidMapUVE();
    map.sourceSkeletonId = "other_skeleton";

    EXPECT_EQ(ValidateRetargetMapAgainstSkeletonUVE(map, MakeMatchingSkeletonUVE()).code,
              RetargetMapValidationCodeUVE::SkeletonMismatch);
}

TEST(RetargetMapUVETest, LoadRetargetMapJsonUVE_PreservesOwnerAuthoredDataAndPoseAnalysis) {
    const std::filesystem::path path = std::filesystem::path(UVE_SOURCE_DIR) /
                                       "assets/retarget/UNIVEX_bone_retarget_map.json";
    std::ifstream input(path);
    ASSERT_TRUE(input.is_open());
    std::ostringstream content;
    content << input.rdbuf();

    RetargetMapUVE map;
    const RetargetMapValidationResultUVE result = LoadRetargetMapJsonUVE(content.str(), map);

    ASSERT_TRUE(result.IsValidUVE()) << result.message;
    EXPECT_EQ(map.entries.size(), 162U);
    EXPECT_EQ(map.mapId, "UNIVEX_bone_retarget_map");
    EXPECT_EQ(map.sourceSkeletonId, "UNIVEX_bone_retarget_map");
    EXPECT_EQ(map.coordinateSystem, "Z-up");
    EXPECT_EQ(map.units, "source-authored");
    EXPECT_EQ(map.rootBoneId, "root");
    EXPECT_TRUE(map.isAPose);
    EXPECT_EQ(map.poseAnalysis.detectedPose, RetargetPoseClassificationUVE::APose);
    EXPECT_EQ(map.poseAnalysis.detectedPoseLabel, "A-pose");
    EXPECT_NEAR(map.poseAnalysis.leftArmAngleFromDownDegrees, 35.2F, 0.0001F);
    EXPECT_NEAR(map.poseAnalysis.rightArmAngleFromDownDegrees, 35.2F, 0.0001F);
    EXPECT_EQ(map.poseAnalysis.aPoseMinimumDegrees, 25.0F);
    EXPECT_EQ(map.poseAnalysis.aPoseMaximumDegrees, 55.0F);
    EXPECT_EQ(map.poseAnalysis.relaxedArmsDownMinimumDegrees, 0.0F);
    EXPECT_EQ(map.poseAnalysis.relaxedArmsDownMaximumDegrees, 20.0F);
    EXPECT_EQ(map.poseAnalysis.tPoseMinimumDegrees, 75.0F);
    EXPECT_EQ(map.poseAnalysis.tPoseMaximumDegrees, 95.0F);
    EXPECT_EQ(map.poseAnalysis.method,
              "bind-pose global position of upperarm -> lowerarm (elbow) vector, angle measured from straight-down (-Z) axis");
    EXPECT_EQ(map.poseAnalysis.aPoseThresholdText, "25-55 deg");
    EXPECT_EQ(map.poseAnalysis.relaxedArmsDownThresholdText, "0-20 deg");
    EXPECT_EQ(map.poseAnalysis.tPoseThresholdText, "75-95 deg");

    std::size_t coreCount = 0U;
    std::size_t correctiveCount = 0U;
    std::size_t ikCount = 0U;
    std::size_t utilityCount = 0U;
    std::size_t rootCategoryCount = 0U;
    std::size_t rootCount = 0U;
    std::size_t namedCount = 0U;
    std::unordered_set<std::string> canonicalNames;
    for (const RetargetMapBoneUVE& entry : map.entries) {
        if (!entry.genericName.empty()) {
            ++namedCount;
            EXPECT_TRUE(canonicalNames.insert(entry.genericName).second);
        }
        switch (entry.category) {
        case RetargetMapBoneCategoryUVE::Core:
            ++coreCount;
            break;
        case RetargetMapBoneCategoryUVE::CorrectiveHelper:
            ++correctiveCount;
            break;
        case RetargetMapBoneCategoryUVE::IK:
            ++ikCount;
            break;
        case RetargetMapBoneCategoryUVE::Utility:
            ++utilityCount;
            break;
        case RetargetMapBoneCategoryUVE::Root:
            ++rootCategoryCount;
            break;
        }
        if (entry.boneId == "root") {
            ++rootCount;
            EXPECT_EQ(entry.category, RetargetMapBoneCategoryUVE::Root);
            EXPECT_TRUE(entry.parentBoneId.empty());
            EXPECT_EQ(entry.genericName, "Root");
        }
        if (entry.boneId == "upperarm_l") {
            EXPECT_EQ(entry.genericName, "LeftArm");
            EXPECT_EQ(entry.parentBoneId, "clavicle_l");
            EXPECT_FLOAT_EQ(entry.bindPoseGlobalPosition.x, 19.01F);
        }
    }
    EXPECT_EQ(coreCount, 63U);
    EXPECT_EQ(correctiveCount, 86U);
    EXPECT_EQ(ikCount, 7U);
    EXPECT_EQ(utilityCount, 5U);
    EXPECT_EQ(rootCategoryCount, 1U);
    EXPECT_EQ(rootCount, 1U);
    EXPECT_EQ(namedCount, 71U);
    EXPECT_EQ(map.entries.size() - namedCount, 91U);
    EXPECT_TRUE(ValidateRetargetMapUVE(map).IsValidUVE());
}

TEST(RetargetMapUVETest, LoadRetargetMapFileUVE_ComputesSuppliedAPoseFromBindPositions) {
    const std::filesystem::path path = std::filesystem::path(UVE_SOURCE_DIR) /
                                       "assets/retarget/UNIVEX_bone_retarget_map.json";
    RetargetMapUVE map;

    const RetargetMapValidationResultUVE result = LoadRetargetMapFileUVE(path, map);
    ASSERT_TRUE(result.IsValidUVE()) << result.message;
    const RetargetMapPoseAnalysisUVE computed = AnalyzeRetargetMapPoseUVE(map);

    EXPECT_EQ(computed.detectedPose, RetargetPoseClassificationUVE::APose);
    EXPECT_NEAR(computed.leftArmAngleFromDownDegrees, 35.2F, 0.1F);
    EXPECT_NEAR(computed.rightArmAngleFromDownDegrees, 35.2F, 0.1F);
    EXPECT_EQ(computed.detectedPoseLabel, map.poseAnalysis.detectedPoseLabel);
    EXPECT_EQ(computed.method, map.poseAnalysis.method);
    EXPECT_EQ(computed.aPoseThresholdText, map.poseAnalysis.aPoseThresholdText);
    EXPECT_EQ(computed.aPoseMinimumDegrees, map.poseAnalysis.aPoseMinimumDegrees);
    EXPECT_EQ(computed.tPoseMaximumDegrees, map.poseAnalysis.tPoseMaximumDegrees);
}

TEST(RetargetMapUVETest, AnalyzeRetargetMapPoseUVE_UsesSuppliedAxisAndInclusiveToleranceBands) {
    RetargetMapUVE map;
    map.poseAnalysis.aPoseMinimumDegrees = 25.0F;
    map.poseAnalysis.aPoseMaximumDegrees = 55.0F;
    map.poseAnalysis.relaxedArmsDownMinimumDegrees = 0.0F;
    map.poseAnalysis.relaxedArmsDownMaximumDegrees = 20.0F;
    map.poseAnalysis.tPoseMinimumDegrees = 75.0F;
    map.poseAnalysis.tPoseMaximumDegrees = 95.0F;
    map.entries = {
        RetargetMapBoneUVE{"upperarm_l", {}, {}, RetargetMapBoneCategoryUVE::Core, {0.0F, 0.0F, 0.0F}},
        RetargetMapBoneUVE{"lowerarm_l", {}, {}, RetargetMapBoneCategoryUVE::Core, {25.0F, 0.0F, -53.58899F}},
        RetargetMapBoneUVE{"upperarm_r", {}, {}, RetargetMapBoneCategoryUVE::Core, {0.0F, 0.0F, 0.0F}},
        RetargetMapBoneUVE{"lowerarm_r", {}, {}, RetargetMapBoneCategoryUVE::Core, {-25.0F, 0.0F, -53.58899F}},
    };
    EXPECT_EQ(AnalyzeRetargetMapPoseUVE(map).detectedPose, RetargetPoseClassificationUVE::APose);

    map.entries[1].bindPoseGlobalPosition = {0.0F, 0.0F, -1.0F};
    map.entries[3].bindPoseGlobalPosition = {0.0F, 0.0F, -1.0F};
    EXPECT_EQ(AnalyzeRetargetMapPoseUVE(map).detectedPose, RetargetPoseClassificationUVE::RelaxedArmsDown);

    map.entries[1].bindPoseGlobalPosition = {75.0F, 0.0F, -1.0F};
    map.entries[3].bindPoseGlobalPosition = {-75.0F, 0.0F, -1.0F};
    EXPECT_EQ(AnalyzeRetargetMapPoseUVE(map).detectedPose, RetargetPoseClassificationUVE::TPOSE);

    map.entries.pop_back();
    EXPECT_EQ(AnalyzeRetargetMapPoseUVE(map).detectedPose, RetargetPoseClassificationUVE::Unknown);
    EXPECT_TRUE(AnalyzeRetargetMapPoseUVE(map).detectedPoseLabel.empty());
}

TEST(RetargetMapUVETest, LoadRetargetMapJsonUVE_RejectsMalformedRequiredData) {
    RetargetMapUVE map;
    EXPECT_EQ(LoadRetargetMapJsonUVE("[]", map).code,
              RetargetMapValidationCodeUVE::InvalidMetadata);
    EXPECT_EQ(LoadRetargetMapJsonUVE(
                  R"({"root":{"category":"unsupported","parent":null,"bind_pose_global_position":[0,0,0],"generic_name":"root"}})", map)
                  .code,
              RetargetMapValidationCodeUVE::InvalidCategory);
    EXPECT_EQ(LoadRetargetMapJsonUVE(
                  R"({"root":{"category":"core","parent":null,"bind_pose_global_position":[0,0],"generic_name":"Root"}})", map)
                  .code,
              RetargetMapValidationCodeUVE::InvalidEntry);
}

TEST(RetargetMapUVETest, ValidateRetargetMapUVE_RejectsNonFiniteBindPosition) {
    RetargetMapUVE map = MakeValidMapUVE();
    map.entries[1].bindPoseGlobalPosition.x = std::numeric_limits<float>::quiet_NaN();

    EXPECT_EQ(ValidateRetargetMapUVE(map).code,
              RetargetMapValidationCodeUVE::InvalidEntry);
}

} // namespace UVE::Core
