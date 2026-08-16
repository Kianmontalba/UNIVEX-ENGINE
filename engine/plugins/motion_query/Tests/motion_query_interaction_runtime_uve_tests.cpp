// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_interaction_runtime_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {
namespace {
UVE::Asset::ResourceHandleUVE MakeHandleUVE(std::uint64_t guid, std::uint64_t generation = 1U) {
    return UVE::Asset::ResourceHandleUVE{UVE::Asset::AssetGuidUVE{guid}, generation};
}

MotionQueryInteractionAssetUVE MakeAssetUVE(std::uint64_t guid, const char* id,
                                            UVE::Asset::ResourceHandleUVE required = {}) {
    MotionQueryInteractionAssetUVE asset;
    asset.handle = MakeHandleUVE(guid);
    asset.interactionId = id;
    asset.participants = {
        MotionQueryInteractionParticipantUVE{"initiator", MakeHandleUVE(guid + 100U)},
    };
    if (required.guid.value != 0U) {
        asset.requiredResources = {required};
    }
    return asset;
}
} // namespace

TEST(MotionQueryInteractionRuntimeUVETest, ValidatorUVE_RejectsInvalidAndDuplicateParticipantData) {
    MotionQueryInteractionAssetUVE invalid;
    EXPECT_EQ(ValidateMotionQueryInteractionAssetUVE(invalid).code,
              MotionQueryInteractionValidationCodeUVE::InvalidHandle);

    MotionQueryInteractionAssetUVE duplicate = MakeAssetUVE(1U, "vault");
    duplicate.participants.push_back(duplicate.participants.front());
    EXPECT_EQ(ValidateMotionQueryInteractionAssetUVE(duplicate).code,
              MotionQueryInteractionValidationCodeUVE::DuplicateParticipantRole);
}

TEST(MotionQueryInteractionRuntimeUVETest, LibraryUVE_SelectsByIslandIdAndRequiredResourcesDeterministically) {
    MotionQueryInteractionLibraryUVE library;
    const UVE::Asset::ResourceHandleUVE required = MakeHandleUVE(50U);
    ASSERT_TRUE(library.RegisterAssetUVE(MakeAssetUVE(1U, "vault", required)).IsValidUVE());
    ASSERT_TRUE(library.RegisterAssetUVE(MakeAssetUVE(2U, "dash")).IsValidUVE());
    MotionQueryInteractionIslandUVE island;
    island.handle = MakeHandleUVE(90U);
    island.islandId = "locomotion";
    island.interactionAssets = {MakeHandleUVE(1U), MakeHandleUVE(2U)};
    ASSERT_TRUE(library.RegisterIslandUVE(island).IsValidUVE());

    MotionQueryInteractionSelectionRequestUVE request;
    request.islandId = "locomotion";
    request.interactionId = "vault";
    request.availableResources = {required};
    const MotionQueryInteractionSelectionResultUVE result = library.SelectUVE(request);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    EXPECT_EQ(result.interactionId, "vault");
    EXPECT_EQ(result.interactionAsset, MakeHandleUVE(1U));

    request.availableResources.clear();
    EXPECT_EQ(library.SelectUVE(request).code,
              MotionQueryInteractionSelectionCodeUVE::NoCompatibleInteraction);
}

TEST(MotionQueryInteractionRuntimeUVETest, LibraryUVE_RejectsUnknownIslandAssetsAndDuplicateIds) {
    MotionQueryInteractionLibraryUVE library;
    ASSERT_TRUE(library.RegisterAssetUVE(MakeAssetUVE(1U, "vault")).IsValidUVE());
    EXPECT_EQ(library.RegisterAssetUVE(MakeAssetUVE(2U, "vault")).code,
              MotionQueryInteractionValidationCodeUVE::DuplicateInteractionId);

    MotionQueryInteractionIslandUVE island;
    island.handle = MakeHandleUVE(90U);
    island.islandId = "unknown-ref";
    island.interactionAssets = {MakeHandleUVE(999U)};
    EXPECT_EQ(library.RegisterIslandUVE(island).code,
              MotionQueryInteractionValidationCodeUVE::UnknownInteractionAsset);
}

TEST(MotionQueryInteractionRuntimeUVETest, SubsystemUVE_IsFailureAtomicAndClearable) {
    MotionQueryInteractionLibraryUVE library;
    ASSERT_TRUE(library.RegisterAssetUVE(MakeAssetUVE(1U, "vault")).IsValidUVE());
    MotionQueryInteractionSubsystemUVE subsystem;
    MotionQueryInteractionSelectionRequestUVE validRequest;
    ASSERT_TRUE(subsystem.SelectUVE(library, validRequest).IsAcceptedUVE());
    ASSERT_TRUE(subsystem.HasSelectionUVE());

    MotionQueryInteractionSelectionRequestUVE invalidRequest;
    invalidRequest.availableResources = {MakeHandleUVE(1U), MakeHandleUVE(1U)};
    EXPECT_EQ(subsystem.SelectUVE(library, invalidRequest).code,
              MotionQueryInteractionSelectionCodeUVE::InvalidRequest);
    EXPECT_FALSE(subsystem.HasSelectionUVE());
    subsystem.ClearUVE();
    EXPECT_FALSE(subsystem.HasSelectionUVE());
}
} // namespace UVE::Plugins
