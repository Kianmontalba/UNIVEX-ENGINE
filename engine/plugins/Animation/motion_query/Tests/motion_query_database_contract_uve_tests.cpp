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

TEST(MotionQueryDatabaseContractUVETest, FactoryUVE_CreatesValidatedDeterministicDefaultContract) {
    const MotionQueryDatabaseFactoryResultUVE first =
        CreateDefaultMotionQueryDatabaseContractUVE("locomotion", 7U, "locomotion-v1");
    ASSERT_TRUE(first.IsCreatedUVE()) << first.validation.message;
    EXPECT_EQ(first.contract.context.databaseId, "locomotion");
    EXPECT_EQ(first.contract.context.generation, 7U);
    EXPECT_EQ(first.contract.schema.schemaId, "locomotion-v1");
    EXPECT_EQ(first.contract.schema.trajectoryOffsets, (std::vector<double>{0.0, 0.25}));
    EXPECT_EQ(first.contract.schema.featureChannelIds, (std::vector<std::string>{"root_velocity"}));
    EXPECT_EQ(first.contract.settings.maximumCandidates, 4U);
    ASSERT_EQ(first.contract.database.candidates.size(), 1U);
    EXPECT_EQ(first.contract.database.candidates.front().candidateId, "default-candidate-0");

    const MotionQueryDatabaseFactoryResultUVE second =
        CreateDefaultMotionQueryDatabaseContractUVE("locomotion", 7U, "locomotion-v1");
    ASSERT_TRUE(second.IsCreatedUVE());
    EXPECT_EQ(second.contract.context, first.contract.context);
    EXPECT_EQ(second.contract.schema, first.contract.schema);
    EXPECT_EQ(second.contract.settings.maximumCandidates, first.contract.settings.maximumCandidates);
    EXPECT_EQ(ValidateMotionQueryDatabaseContractUVE(first.contract).code,
              MotionQueryDatabaseContractCodeUVE::Valid);
}

TEST(MotionQueryDatabaseContractUVETest, FactoryUVE_CreatesValidatedImportedCandidateSet) {
    MotionQueryDatabaseContractUVE source = MakeContractUVE();
    MotionMatchingCandidateUVE second = MakeCandidateUVE();
    second.candidateId = "run_1";
    second.sourceClipId = "run";
    second.sampleTimeSeconds = 1.5;
    second.feature.rootVelocity = {2.0F, 0.0F, 0.0F};
    source.database.candidates.push_back(second);

    const MotionQueryDatabaseFactoryResultUVE created = CreateMotionQueryDatabaseContractUVE(
        source.context, source.schema, source.settings, source.database);
    ASSERT_TRUE(created.IsCreatedUVE()) << created.validation.message;
    EXPECT_EQ(created.contract.context, source.context);
    EXPECT_EQ(created.contract.schema, source.schema);
    EXPECT_EQ(created.contract.settings, source.settings);
    ASSERT_EQ(created.contract.database.candidates.size(), 2U);
    EXPECT_EQ(created.contract.database.candidates[1].candidateId, "run_1");

    source.database.candidates[1].candidateId.clear();
    const MotionQueryDatabaseFactoryResultUVE invalid = CreateMotionQueryDatabaseContractUVE(
        source.context, source.schema, source.settings, source.database);
    EXPECT_FALSE(invalid.IsCreatedUVE());
    EXPECT_EQ(invalid.validation.code, MotionQueryDatabaseContractCodeUVE::DatabaseValidationFailed);
}

TEST(MotionQueryDatabaseContractUVETest, FactoryUVE_UsesSharedValidationForInvalidInputs) {
    const MotionQueryDatabaseFactoryResultUVE invalidContext =
        CreateDefaultMotionQueryDatabaseContractUVE("", 1U, "locomotion-v1");
    EXPECT_FALSE(invalidContext.IsCreatedUVE());
    EXPECT_EQ(invalidContext.validation.code, MotionQueryDatabaseContractCodeUVE::InvalidContext);

    const MotionQueryDatabaseFactoryResultUVE invalidSchema =
        CreateDefaultMotionQueryDatabaseContractUVE("locomotion", 1U, "");
    EXPECT_FALSE(invalidSchema.IsCreatedUVE());
    EXPECT_EQ(invalidSchema.validation.code, MotionQueryDatabaseContractCodeUVE::InvalidSchema);

    const MotionQueryDatabaseFactoryResultUVE invalidSettings =
        CreateDefaultMotionQueryDatabaseContractUVE("locomotion", 1U, "locomotion-v1", 0U);
    EXPECT_FALSE(invalidSettings.IsCreatedUVE());
    EXPECT_EQ(invalidSettings.validation.code, MotionQueryDatabaseContractCodeUVE::InvalidSettings);
}

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
