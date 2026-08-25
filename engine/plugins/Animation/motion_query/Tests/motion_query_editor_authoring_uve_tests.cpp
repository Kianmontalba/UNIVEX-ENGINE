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

TEST(MotionQueryEditorAuthoringUVETest, EditorUtilsUVE_ProvidesPropertyMetadataAndFactoryValidation) {
    const auto& metadata = GetMotionQueryEditorPropertyMetadataUVE();
    ASSERT_EQ(metadata.size(), 10U);
    EXPECT_EQ(metadata.front().id, "display_name");
    EXPECT_EQ(metadata.front().type, MotionQueryEditorPropertyTypeUVE::String);
    EXPECT_TRUE(metadata.front().editable);
    EXPECT_TRUE(metadata.front().required);
    EXPECT_EQ(metadata.front().maximumBytes, kMotionQueryEditorMaximumDisplayNameBytesUVE);
    EXPECT_EQ(metadata[1].id, "database_id");
    EXPECT_FALSE(metadata[1].editable);
    EXPECT_EQ(metadata[2].id, "generation");
    EXPECT_EQ(metadata[3].id, "schema_id");
    EXPECT_TRUE(metadata[3].editable);
    EXPECT_EQ(metadata[4].id, "schema_version");
    EXPECT_EQ(metadata[5].id, "trajectory_offsets");
    EXPECT_EQ(metadata[6].id, "feature_channels");
    EXPECT_EQ(metadata[7].id, "maximum_candidates");
    EXPECT_TRUE(metadata[7].editable);
    EXPECT_EQ(metadata[8].id, "candidate_count");
    EXPECT_EQ(metadata[9].id, "dirty");

    const MotionQueryEditorDatabaseFactoryResultUVE created = CreateMotionQueryEditorDatabaseEntryUVE(
        MakeResourceUVE(9U), "Factory Database", MakeContractUVE("factory-db"));
    ASSERT_TRUE(created.IsCreatedUVE()) << created.validation.message;
    EXPECT_EQ(created.entry.resource, MakeResourceUVE(9U));
    EXPECT_EQ(created.entry.displayName, "Factory Database");
    EXPECT_EQ(created.entry.contract.context.databaseId, "factory-db");

    const MotionQueryEditorDatabaseFactoryResultUVE invalidResource =
        CreateMotionQueryEditorDatabaseEntryUVE(MakeResourceUVE(0U), "Factory Database",
                                                MakeContractUVE("factory-db"));
    EXPECT_FALSE(invalidResource.IsCreatedUVE());
    EXPECT_EQ(invalidResource.validation.code,
              MotionQueryEditorUtilityValidationCodeUVE::InvalidResource);

    MotionQueryEditorDatabaseEntryUVE invalidEntry;
    invalidEntry.resource = MakeResourceUVE(9U);
    invalidEntry.displayName = "Factory Database";
    invalidEntry.contract = MakeContractUVE("factory-db");
    invalidEntry.contract.database.candidates.clear();
    const auto invalidContract = ValidateMotionQueryEditorDatabaseEntryUVE(invalidEntry);
    EXPECT_EQ(invalidContract.code,
              MotionQueryEditorUtilityValidationCodeUVE::InvalidDatabaseContract);
}

TEST(MotionQueryEditorAuthoringUVETest, EditorUtilsUVE_ValidateNamesNormalizeSchemaIdsAndDetectCandidateDuplicates) {
    EXPECT_TRUE(IsValidMotionQueryEditorDisplayNameUVE("Main"));
    EXPECT_FALSE(IsValidMotionQueryEditorDisplayNameUVE(""));
    EXPECT_FALSE(IsValidMotionQueryEditorDisplayNameUVE(
        std::string(kMotionQueryEditorMaximumDisplayNameBytesUVE + 1U, 'x')));
    EXPECT_EQ(NormalizeMotionQueryEditorSchemaIdUVE(" \t locomotion-v1 \r\n"), "locomotion-v1");
    EXPECT_TRUE(NormalizeMotionQueryEditorSchemaIdUVE(" \t ").empty());

    const std::vector<UVE::Core::MotionMatchingCandidateUVE> valid = {
        MakeCandidateUVE("candidate-0"), MakeCandidateUVE("candidate-1")};
    EXPECT_TRUE(ValidateMotionQueryEditorCandidateIdentifiersUVE(valid).IsValidUVE());

    const std::vector<UVE::Core::MotionMatchingCandidateUVE> duplicate = {
        MakeCandidateUVE("candidate-0"), MakeCandidateUVE("candidate-0")};
    const auto duplicateResult = ValidateMotionQueryEditorCandidateIdentifiersUVE(duplicate);
    EXPECT_EQ(duplicateResult.code, MotionQueryEditorUtilityValidationCodeUVE::DuplicateCandidateIdentifier);
    EXPECT_EQ(duplicateResult.index, 1U);

    const std::string oversizedId(
        UVE::Core::MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE + 1U, 'x');
    const std::vector<UVE::Core::MotionMatchingCandidateUVE> oversized = {
        MakeCandidateUVE(oversizedId.c_str())};
    const auto oversizedResult = ValidateMotionQueryEditorCandidateIdentifiersUVE(oversized);
    EXPECT_EQ(oversizedResult.code,
              MotionQueryEditorUtilityValidationCodeUVE::InvalidCandidateIdentifier);
    EXPECT_EQ(oversizedResult.index, 0U);
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

TEST(MotionQueryEditorAuthoringUVETest, CopyAndPasteUVE_CopiesPayloadAndRemapsDestinationIdentity) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Source", "source-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);
    EXPECT_FALSE(session.GetSnapshotUVE().clipboardAvailable);

    MotionQueryEditorCommandUVE copy = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::CopyDatabase, 1U);
    copy.resource = MakeResourceUVE(1U);
    const MotionQueryEditorResponseUVE copyResponse = session.DispatchUVE(copy);
    ASSERT_TRUE(copyResponse.applied);
    EXPECT_TRUE(copyResponse.snapshot.clipboardAvailable);

    MotionQueryEditorCommandUVE paste = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::PasteDatabase, 2U);
    paste.pasteTarget = MotionQueryEditorPasteTargetUVE{
        MakeResourceUVE(2U, 3U), "Pasted", UVE::Core::MotionQueryDatabaseContextUVE{"pasted-db", 7U}};
    const MotionQueryEditorResponseUVE pasteResponse = session.DispatchUVE(paste);
    ASSERT_TRUE(pasteResponse.applied);
    ASSERT_EQ(pasteResponse.snapshot.databases.size(), 2U);
    EXPECT_EQ(pasteResponse.snapshot.databases[0].displayName, "Pasted");
    EXPECT_EQ(pasteResponse.snapshot.databases[0].databaseId, "pasted-db");
    EXPECT_EQ(pasteResponse.snapshot.databases[0].generation, 7U);
    EXPECT_EQ(pasteResponse.snapshot.databases[0].candidateCount, 1U);
    EXPECT_TRUE(pasteResponse.snapshot.databases[0].dirty);

    UVE::Core::MotionQueryDatabaseContractUVE pastedContract;
    ASSERT_TRUE(session.TryGetDatabaseCopyUVE(MakeResourceUVE(2U, 3U), pastedContract));
    EXPECT_EQ(pastedContract.context.databaseId, "pasted-db");
    EXPECT_EQ(pastedContract.context.generation, 7U);
    EXPECT_TRUE(pastedContract.events.empty());
}

TEST(MotionQueryEditorAuthoringUVETest, CopyAndPasteUVE_RejectsMissingOrDuplicateTargetsWithoutMutation) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE pasteWithoutCopy = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::PasteDatabase, 0U);
    pasteWithoutCopy.pasteTarget = MotionQueryEditorPasteTargetUVE{
        MakeResourceUVE(2U), "Pasted", UVE::Core::MotionQueryDatabaseContextUVE{"pasted-db", 1U}};
    EXPECT_EQ(session.DispatchUVE(pasteWithoutCopy).code, MotionQueryEditorResponseCodeUVE::ClipboardEmpty);
    EXPECT_TRUE(session.GetSnapshotUVE().databases.empty());

    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Source", "source-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);
    MotionQueryEditorCommandUVE copy = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::CopyDatabase, 1U);
    copy.resource = MakeResourceUVE(1U);
    ASSERT_TRUE(session.DispatchUVE(copy).applied);

    MotionQueryEditorCommandUVE duplicateTarget = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::PasteDatabase, 2U);
    duplicateTarget.pasteTarget = MotionQueryEditorPasteTargetUVE{
        MakeResourceUVE(1U), "Duplicate", UVE::Core::MotionQueryDatabaseContextUVE{"other-db", 2U}};
    EXPECT_EQ(session.DispatchUVE(duplicateTarget).code, MotionQueryEditorResponseCodeUVE::DuplicateDatabase);
    EXPECT_EQ(session.GetSnapshotUVE().databases.size(), 1U);

    MotionQueryEditorCommandUVE invalidTarget = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::PasteDatabase, 2U);
    invalidTarget.pasteTarget = MotionQueryEditorPasteTargetUVE{
        MakeResourceUVE(3U), "", UVE::Core::MotionQueryDatabaseContextUVE{"invalid-db", 1U}};
    EXPECT_EQ(session.DispatchUVE(invalidTarget).code, MotionQueryEditorResponseCodeUVE::InvalidPasteTarget);
    EXPECT_EQ(session.GetSnapshotUVE().revision, 2U);
    session.ClearUVE();
    EXPECT_FALSE(session.GetSnapshotUVE().clipboardAvailable);
    EXPECT_FALSE(session.GetSnapshotUVE().canUndo);
    EXPECT_FALSE(session.GetSnapshotUVE().canRedo);
}

TEST(MotionQueryEditorAuthoringUVETest, UndoRedoUVE_RestoresSnapshotsAndHonorsRevisionGuards) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE emptyUndo = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::Undo, 0U);
    EXPECT_EQ(session.DispatchUVE(emptyUndo).code, MotionQueryEditorResponseCodeUVE::NothingToUndo);

    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Main", "main-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);

    MotionQueryEditorCommandUVE rename = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SetDisplayName, 1U);
    rename.resource = MakeResourceUVE(1U);
    rename.text = "Renamed";
    ASSERT_TRUE(session.DispatchUVE(rename).applied);
    ASSERT_EQ(session.GetSnapshotUVE().revision, 2U);
    EXPECT_TRUE(session.GetSnapshotUVE().canUndo);
    EXPECT_FALSE(session.GetSnapshotUVE().canRedo);

    MotionQueryEditorCommandUVE staleUndo = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::Undo, 1U);
    EXPECT_EQ(session.DispatchUVE(staleUndo).code, MotionQueryEditorResponseCodeUVE::StaleRevision);

    MotionQueryEditorCommandUVE undo = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::Undo, 2U);
    const MotionQueryEditorResponseUVE undoResponse = session.DispatchUVE(undo);
    ASSERT_TRUE(undoResponse.applied);
    ASSERT_EQ(undoResponse.snapshot.revision, 3U);
    ASSERT_EQ(undoResponse.snapshot.databases.size(), 1U);
    EXPECT_EQ(undoResponse.snapshot.databases[0].displayName, "Main");
    EXPECT_TRUE(undoResponse.snapshot.canRedo);

    MotionQueryEditorCommandUVE redo = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::Redo, 3U);
    const MotionQueryEditorResponseUVE redoResponse = session.DispatchUVE(redo);
    ASSERT_TRUE(redoResponse.applied);
    ASSERT_EQ(redoResponse.snapshot.revision, 4U);
    ASSERT_EQ(redoResponse.snapshot.databases[0].displayName, "Renamed");
    EXPECT_TRUE(redoResponse.snapshot.canUndo);
    EXPECT_FALSE(redoResponse.snapshot.canRedo);

    MotionQueryEditorCommandUVE emptyRedo = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::Redo, 4U);
    EXPECT_EQ(session.DispatchUVE(emptyRedo).code, MotionQueryEditorResponseCodeUVE::NothingToRedo);

    MotionQueryEditorCommandUVE branch = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::SetSchemaId, 4U);
    branch.resource = MakeResourceUVE(1U);
    branch.text = " \tbranch-schema\r\n";
    ASSERT_TRUE(session.DispatchUVE(branch).applied);
    EXPECT_FALSE(session.GetSnapshotUVE().canRedo);
    UVE::Core::MotionQueryDatabaseContractUVE normalizedContract;
    ASSERT_TRUE(session.TryGetDatabaseCopyUVE(MakeResourceUVE(1U), normalizedContract));
    EXPECT_EQ(normalizedContract.schema.schemaId, "branch-schema");
}

TEST(MotionQueryEditorAuthoringUVETest, UndoRedoUVE_EnforcesBoundedHistoryCapacity) {
    MotionQueryEditorAuthoringSessionUVE session;
    MotionQueryEditorCommandUVE registerCommand = MakeCommandUVE(
        MotionQueryEditorCommandKindUVE::RegisterDatabase, 0U);
    registerCommand.database = MakeEntryUVE(1U, "Main", "main-db");
    ASSERT_TRUE(session.DispatchUVE(registerCommand).applied);

    std::uint64_t revision = 1U;
    for (std::size_t index = 0U; index < kMotionQueryEditorMaximumHistoryEntriesUVE + 8U; ++index) {
        MotionQueryEditorCommandUVE rename = MakeCommandUVE(
            MotionQueryEditorCommandKindUVE::SetDisplayName, revision);
        rename.resource = MakeResourceUVE(1U);
        rename.text = "Name-" + std::to_string(index);
        ASSERT_TRUE(session.DispatchUVE(rename).applied);
        ++revision;
    }

    std::size_t undoCount = 0U;
    while (session.GetSnapshotUVE().canUndo) {
        MotionQueryEditorCommandUVE undo = MakeCommandUVE(
            MotionQueryEditorCommandKindUVE::Undo, revision);
        ASSERT_TRUE(session.DispatchUVE(undo).applied);
        ++revision;
        ++undoCount;
    }
    EXPECT_EQ(undoCount, kMotionQueryEditorMaximumHistoryEntriesUVE);
    EXPECT_TRUE(session.GetSnapshotUVE().canRedo);
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
