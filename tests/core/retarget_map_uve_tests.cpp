// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/retarget_map_uve.h"

#include <limits>

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

TEST(RetargetMapUVETest, ValidateRetargetMapUVE_RejectsNonFiniteBindPosition) {
    RetargetMapUVE map = MakeValidMapUVE();
    map.entries[1].bindPoseGlobalPosition.x = std::numeric_limits<float>::quiet_NaN();

    EXPECT_EQ(ValidateRetargetMapUVE(map).code,
              RetargetMapValidationCodeUVE::InvalidEntry);
}

} // namespace UVE::Core
