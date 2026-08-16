// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_editor_authoring_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {
UVE::Asset::ResourceHandleUVE MakeResourceUVE(std::uint64_t guid, std::uint64_t generation = 1U) {
    return UVE::Asset::ResourceHandleUVE{UVE::Asset::AssetGuidUVE{guid}, generation};
}

UVE::Core::MotionMatchingCandidateUVE MakeCandidateUVE(const char* id) {
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = id;
    candidate.sourceClipId = "walk";
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return candidate;
}

UVE::Core::MotionQueryDatabaseContractUVE MakeContractUVE(const char* id) {
    UVE::Core::MotionQueryDatabaseContractUVE contract;
    contract.context.databaseId = id;
    contract.context.generation = 1U;
    contract.schema.schemaId = "locomotion-v1";
    contract.settings.maximumCandidates = 4U;
    contract.database.candidates = {MakeCandidateUVE("candidate-0")};
    return contract;
}

MotionQueryEditorDatabaseEntryUVE MakeEntryUVE(std::uint64_t guid, const char* name,
                                               const char* databaseId) {
    MotionQueryEditorDatabaseEntryUVE entry;
    entry.resource = MakeResourceUVE(guid);
    entry.displayName = name;
    entry.contract = MakeContractUVE(databaseId);
    return entry;
}

MotionQueryEditorCommandUVE MakeCommandUVE(MotionQueryEditorCommandKindUVE kind,
                                            std::uint64_t revision) {
    MotionQueryEditorCommandUVE command;
    command.requestId = revision + 100U;
    command.expectedRevision = revision;
    command.kind = kind;
    return command;
}
} // namespace

TEST(MotionQueryEditorAuthoringUVETest, RegisterUVE_ProducesValidCopiedSortedSnapshot) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE first = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    first.database = MakeEntryUVE(2U, "Zulu", "zulu-db");
    ASSERT_TRUE(session.DispatchUVE(first).applied);

    MotionQueryEditorCommandUVE second = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 1U);
    second.database = MakeEntryUVE(1U, "Alpha", "alpha-db");
    const MotionQueryEditorResponseUVE response = session.DispatchUVE(second);
    ASSERT_TRUE(response.applied);
    ASSERT_EQ(response.snapshot.databases.size(), 2U);
    EXPECT_EQ(response.snapshot.databases[0].displayName, "Alpha");
    EXPECT_EQ(response.snapshot.databases[1].displayName, "Zulu");
    EXPECT_TRUE(response.snapshot.databases[0].valid);
    EXPECT_EQ(response.snapshot.revision, 2U);
}

TEST(MotionQueryEditorAuthoringUVETest, DispatchUVE_RejectsStaleRevisionAndDuplicateDatabase) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Main", "main-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);

    MotionQueryEditorCommandUVE stale = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SetDisplayName, 0U);
    stale.resource = MakeResourceUVE(1U);
    stale.text = "Stale";
    EXPECT_EQ(session.DispatchUVE(stale).code, MotionQueryEditorResponseCodeUVE::StaleRevision);

    MotionQueryEditorCommandUVE duplicate = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 1U);
    duplicate.database = MakeEntryUVE(1U, "Duplicate", "duplicate-db");
    EXPECT_EQ(session.DispatchUVE(duplicate).code,
              MotionQueryEditorResponseCodeUVE::DuplicateDatabase);
}

TEST(MotionQueryEditorAuthoringUVETest, DispatchUVE_AppliesNamedMutationsAndPreservesStateOnValidationFailure) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Main", "main-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);

    MotionQueryEditorCommandUVE rename = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SetDisplayName, 1U);
    rename.resource = MakeResourceUVE(1U);
    rename.text = "Main Authoring";
    ASSERT_TRUE(session.DispatchUVE(rename).applied);

    MotionQueryEditorCommandUVE add = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::AddCandidate, 2U);
    add.resource = MakeResourceUVE(1U);
    add.candidate = MakeCandidateUVE("candidate-1");
    ASSERT_TRUE(session.DispatchUVE(add).applied);
    EXPECT_EQ(session.GetSnapshotUVE().databases[0].candidateCount, 2U);
    EXPECT_TRUE(session.GetSnapshotUVE().databases[0].dirty);

    MotionQueryEditorCommandUVE invalidLimit = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SetMaximumCandidates, 3U);
    invalidLimit.resource = MakeResourceUVE(1U);
    invalidLimit.candidateIndex = 1U;
    EXPECT_EQ(session.DispatchUVE(invalidLimit).code,
              MotionQueryEditorResponseCodeUVE::ValidationFailed);
    EXPECT_EQ(session.GetSnapshotUVE().databases[0].maximumCandidates, 4U);
    EXPECT_EQ(session.GetSnapshotUVE().databases[0].candidateCount, 2U);
}

TEST(MotionQueryEditorAuthoringUVETest, SelectAndRemoveUVE_UsesNamedResourceCommands) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Main", "main-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);

    MotionQueryEditorCommandUVE select = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SelectDatabase, 1U);
    select.resource = MakeResourceUVE(1U);
    ASSERT_TRUE(session.DispatchUVE(select).applied);
    ASSERT_TRUE(session.GetSnapshotUVE().selectedResource.has_value());
    EXPECT_TRUE(session.GetSnapshotUVE().databases[0].selected);

    MotionQueryEditorCommandUVE remove = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RemoveDatabase, 2U);
    remove.resource = MakeResourceUVE(1U);
    ASSERT_TRUE(session.DispatchUVE(remove).applied);
    EXPECT_TRUE(session.GetSnapshotUVE().databases.empty());
    EXPECT_FALSE(session.GetSnapshotUVE().selectedResource.has_value());
}
} // namespace UVE::Plugins::Editor
