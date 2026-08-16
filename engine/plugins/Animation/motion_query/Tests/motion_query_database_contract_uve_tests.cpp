// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_database_contract_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

MotionMatchingCandidateUVE MakeCandidateUVE() {
    MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "walk_0";
    candidate.sourceClipId = "walk";
    candidate.sampleTimeSeconds = 0.25;
    candidate.feature.rootVelocity = {1.0F, 0.0F, 0.0F};
    candidate.feature.facingDirection = {0.0F, 0.0F, 1.0F};
    candidate.feature.trajectory = {
        MotionTrajectorySampleUVE{0.0, {0.0F, 0.0F, 0.0F}},
        MotionTrajectorySampleUVE{0.25, {0.25F, 0.0F, 0.0F}},
    };
    return candidate;
}

MotionQueryDatabaseContractUVE MakeContractUVE() {
    MotionQueryDatabaseContractUVE contract;
    contract.context.databaseId = "locomotion";
    contract.context.generation = 1U;
    contract.schema.schemaId = "locomotion_v1";
    contract.schema.trajectoryOffsets = {0.0, 0.25};
    contract.schema.featureChannelIds = {"root_velocity"};
    contract.settings.maximumCandidates = 4U;
    contract.database.candidates.push_back(MakeCandidateUVE());
    return contract;
}

} // namespace

TEST(MotionQueryDatabaseContractUVETest, ValidateUVE_AcceptsBoundedSharedDatabaseContract) {
    MotionQueryDatabaseContractUVE contract = MakeContractUVE();

    const MotionQueryDatabaseContractResultUVE result =
        ValidateMotionQueryDatabaseContractUVE(contract);
    ASSERT_TRUE(result.IsValidUVE()) << result.message;

    const MotionQueryDatabaseContractResultUVE eventResult = AppendMotionQueryDatabaseEventUVE(
        contract, MotionQueryDatabaseEventUVE{MotionQueryDatabaseEventKindUVE::CandidateAdded, 0U,
                                              "walk_0", "candidate added"});
    EXPECT_TRUE(eventResult.IsValidUVE()) << eventResult.message;
    ASSERT_EQ(contract.events.size(), 1U);
    EXPECT_EQ(contract.events.front().sequence, 1U);
    EXPECT_TRUE(ValidateMotionQueryDatabaseContractUVE(contract).IsValidUVE());
}

TEST(MotionQueryDatabaseContractUVETest, ValidateUVE_RejectsSchemaAndSettingsMismatches) {
    MotionQueryDatabaseContractUVE contract = MakeContractUVE();
    contract.schema.trajectoryOffsets = {0.25, 0.0};
    EXPECT_EQ(ValidateMotionQueryDatabaseContractUVE(contract).code,
              MotionQueryDatabaseContractCodeUVE::InvalidSchema);

    contract = MakeContractUVE();
    contract.settings.maximumCandidates = 0U;
    EXPECT_EQ(ValidateMotionQueryDatabaseContractUVE(contract).code,
              MotionQueryDatabaseContractCodeUVE::InvalidSettings);

    contract = MakeContractUVE();
    contract.schema.trajectoryOffsets = {0.0};
    EXPECT_EQ(ValidateMotionQueryDatabaseContractUVE(contract).code,
              MotionQueryDatabaseContractCodeUVE::SchemaMismatch);
}

TEST(MotionQueryDatabaseContractUVETest, EventLifecycle_RejectsInvalidSequenceAndPayload) {
    MotionQueryDatabaseContractUVE contract = MakeContractUVE();
    EXPECT_TRUE(AppendMotionQueryDatabaseEventUVE(
                    contract, MotionQueryDatabaseEventUVE{
                                 MotionQueryDatabaseEventKindUVE::SchemaValidated, 0U, {}, "validated"})
                    .IsValidUVE());

    const MotionQueryDatabaseContractResultUVE wrongSequence = AppendMotionQueryDatabaseEventUVE(
        contract, MotionQueryDatabaseEventUVE{MotionQueryDatabaseEventKindUVE::MatchRequested, 7U,
                                              {}, "requested"});
    EXPECT_EQ(wrongSequence.code, MotionQueryDatabaseContractCodeUVE::InvalidEvent);

    const MotionQueryDatabaseContractResultUVE missingCandidate = AppendMotionQueryDatabaseEventUVE(
        contract, MotionQueryDatabaseEventUVE{MotionQueryDatabaseEventKindUVE::CandidateAdded, 0U,
                                              {}, "missing"});
    EXPECT_EQ(missingCandidate.code, MotionQueryDatabaseContractCodeUVE::InvalidEvent);
}

TEST(MotionQueryDatabaseContractUVETest, ValidateUVE_RejectsDuplicateFeatureChannels) {
    MotionQueryDatabaseContractUVE contract = MakeContractUVE();
    contract.schema.featureChannelIds.push_back("root_velocity");
    EXPECT_EQ(ValidateMotionQueryDatabaseContractUVE(contract).code,
              MotionQueryDatabaseContractCodeUVE::InvalidSchema);
}

} // namespace UVE::Core
