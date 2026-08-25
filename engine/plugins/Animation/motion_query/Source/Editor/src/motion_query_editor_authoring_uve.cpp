// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_editor_authoring_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Plugins::Editor {
namespace {
[[nodiscard]] const char* CommandNameUVE(const MotionQueryEditorCommandKindUVE kind) noexcept {
    const auto& metadata = GetMotionQueryEditorCommandMetadataUVE();
    const auto iterator = std::find_if(metadata.cbegin(), metadata.cend(), [kind](const auto& entry) {
        return entry.kind == kind;
    });
    return iterator == metadata.cend() ? "unknown command" : iterator->name.c_str();
}

[[nodiscard]] bool IsHandleBeforeUVE(const UVE::Asset::ResourceHandleUVE lhs,
                                     const UVE::Asset::ResourceHandleUVE rhs) noexcept {
    if (lhs.guid.value != rhs.guid.value) {
        return lhs.guid.value < rhs.guid.value;
    }
    return lhs.generation < rhs.generation;
}

[[nodiscard]] bool IsValidResourceHandleUVE(const UVE::Asset::ResourceHandleUVE resource) noexcept {
    return resource.guid.value != 0U && resource.generation != 0U;
}
} // namespace

bool IsValidMotionQueryEditorDisplayNameUVE(const std::string_view displayName) noexcept {
    return !displayName.empty() && displayName.size() <= kMotionQueryEditorMaximumDisplayNameBytesUVE;
}

std::string NormalizeMotionQueryEditorSchemaIdUVE(const std::string_view schemaId) {
    const auto isWhitespace = [](const char value) noexcept {
        return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
               value == '\f' || value == '\v';
    };
    std::size_t first = 0U;
    while (first < schemaId.size() && isWhitespace(schemaId[first])) {
        ++first;
    }
    std::size_t last = schemaId.size();
    while (last > first && isWhitespace(schemaId[last - 1U])) {
        --last;
    }
    return std::string(schemaId.substr(first, last - first));
}

MotionQueryEditorUtilityValidationResultUVE ValidateMotionQueryEditorCandidateIdentifiersUVE(
    const std::vector<UVE::Core::MotionMatchingCandidateUVE>& candidates) noexcept {
    for (std::size_t index = 0U; index < candidates.size(); ++index) {
        const std::string& candidateId = candidates[index].candidateId;
        if (candidateId.empty() ||
            candidateId.size() > UVE::Core::MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE) {
            return MotionQueryEditorUtilityValidationResultUVE{
                MotionQueryEditorUtilityValidationCodeUVE::InvalidCandidateIdentifier, index,
                "motion matching candidate identifier is empty or too long"};
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (candidates[previous].candidateId == candidateId) {
                return MotionQueryEditorUtilityValidationResultUVE{
                    MotionQueryEditorUtilityValidationCodeUVE::DuplicateCandidateIdentifier, index,
                    "motion matching candidate identifiers must be unique"};
            }
        }
    }
    return MotionQueryEditorUtilityValidationResultUVE{
        MotionQueryEditorUtilityValidationCodeUVE::Valid, 0U, "valid"};
}

MotionQueryEditorUtilityValidationResultUVE ValidateMotionQueryEditorDatabaseEntryUVE(
    const MotionQueryEditorDatabaseEntryUVE& entry) noexcept {
    if (!IsValidResourceHandleUVE(entry.resource)) {
        return MotionQueryEditorUtilityValidationResultUVE{
            MotionQueryEditorUtilityValidationCodeUVE::InvalidResource, 0U,
            "motion query editor database resource handle is invalid"};
    }
    if (!IsValidMotionQueryEditorDisplayNameUVE(entry.displayName)) {
        return MotionQueryEditorUtilityValidationResultUVE{
            MotionQueryEditorUtilityValidationCodeUVE::InvalidDisplayName, 0U,
            "motion query editor database display name is empty or too long"};
    }
    const MotionQueryEditorUtilityValidationResultUVE candidateValidation =
        ValidateMotionQueryEditorCandidateIdentifiersUVE(entry.contract.database.candidates);
    if (!candidateValidation.IsValidUVE()) {
        return candidateValidation;
    }
    if (!UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry.contract).IsValidUVE()) {
        return MotionQueryEditorUtilityValidationResultUVE{
            MotionQueryEditorUtilityValidationCodeUVE::InvalidDatabaseContract, 0U,
            "motion query editor database contract is invalid"};
    }
    return MotionQueryEditorUtilityValidationResultUVE{
        MotionQueryEditorUtilityValidationCodeUVE::Valid, 0U, "valid"};
}

MotionQueryEditorDatabaseFactoryResultUVE CreateMotionQueryEditorDatabaseEntryUVE(
    const UVE::Asset::ResourceHandleUVE resource, const std::string_view displayName,
    UVE::Core::MotionQueryDatabaseContractUVE contract) {
    MotionQueryEditorDatabaseFactoryResultUVE result;
    result.entry.resource = resource;
    result.entry.displayName = displayName;
    result.entry.contract = std::move(contract);
    result.validation = ValidateMotionQueryEditorDatabaseEntryUVE(result.entry);
    return result;
}

const std::vector<MotionQueryEditorPropertyMetadataUVE>&
GetMotionQueryEditorPropertyMetadataUVE() noexcept {
    static const std::vector<MotionQueryEditorPropertyMetadataUVE> metadata = {
        {"display_name", "Display Name", MotionQueryEditorPropertyTypeUVE::String, true, true, 0U,
         kMotionQueryEditorMaximumDisplayNameBytesUVE},
        {"database_id", "Database ID", MotionQueryEditorPropertyTypeUVE::String, false, true, 0U,
         UVE::Core::MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE},
        {"generation", "Generation", MotionQueryEditorPropertyTypeUVE::UnsignedInteger, false, true,
         1U, 0U},
        {"schema_id", "Schema ID", MotionQueryEditorPropertyTypeUVE::String, true, true, 0U,
         UVE::Core::MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE},
        {"schema_version", "Schema Version", MotionQueryEditorPropertyTypeUVE::UnsignedInteger, false,
         true, 1U, 0U},
        {"trajectory_offsets", "Trajectory Offsets", MotionQueryEditorPropertyTypeUVE::TrajectoryOffsets,
         false, true, UVE::Core::MotionQueryUVE::kMaximumTrajectorySamplesUVE, 0U},
        {"feature_channels", "Feature Channels", MotionQueryEditorPropertyTypeUVE::FeatureChannels,
         false, true, UVE::Core::kMaximumMotionQueryFeatureChannelsUVE, 0U},
        {"maximum_candidates", "Maximum Candidates", MotionQueryEditorPropertyTypeUVE::UnsignedInteger,
         true, true, UVE::Core::MotionMatchingDatabaseUVE::kMaximumCandidatesUVE, 0U},
        {"candidate_count", "Candidate Count", MotionQueryEditorPropertyTypeUVE::CandidateArray, false,
         false, UVE::Core::MotionMatchingDatabaseUVE::kMaximumCandidatesUVE, 0U},
        {"dirty", "Dirty", MotionQueryEditorPropertyTypeUVE::Boolean, false, false, 1U, 0U},
    };
    return metadata;
}

const std::vector<MotionQueryEditorCommandMetadataUVE>&
GetMotionQueryEditorCommandMetadataUVE() noexcept {
    static const std::vector<MotionQueryEditorCommandMetadataUVE> metadata = {
        {MotionQueryEditorCommandKindUVE::ReadSnapshot, MotionQueryEditorCommandPayloadKindUVE::None,
         "read snapshot", "Read Snapshot", false, false, false, false},
        {MotionQueryEditorCommandKindUVE::RegisterDatabase, MotionQueryEditorCommandPayloadKindUVE::Database,
         "register database", "Register Database", true, false, true, true},
        {MotionQueryEditorCommandKindUVE::RemoveDatabase, MotionQueryEditorCommandPayloadKindUVE::Resource,
         "remove database", "Remove Database", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::SelectDatabase, MotionQueryEditorCommandPayloadKindUVE::Resource,
         "select database", "Select Database", true, true, false, true},
        {MotionQueryEditorCommandKindUVE::SetDisplayName, MotionQueryEditorCommandPayloadKindUVE::Text,
         "set display name", "Set Display Name", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::SetSchemaId, MotionQueryEditorCommandPayloadKindUVE::Text,
         "set schema ID", "Set Schema ID", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::SetMaximumCandidates,
         MotionQueryEditorCommandPayloadKindUVE::CandidateIndex, "set maximum candidates",
         "Set Maximum Candidates", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::AddCandidate, MotionQueryEditorCommandPayloadKindUVE::Candidate,
         "add candidate", "Add Candidate", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::RemoveCandidate,
         MotionQueryEditorCommandPayloadKindUVE::CandidateIndex, "remove candidate",
         "Remove Candidate", true, true, true, true},
        {MotionQueryEditorCommandKindUVE::ValidateDatabase, MotionQueryEditorCommandPayloadKindUVE::None,
         "validate database", "Validate Database", false, true, false, false},
        {MotionQueryEditorCommandKindUVE::CopyDatabase, MotionQueryEditorCommandPayloadKindUVE::Resource,
         "copy database", "Copy Database", true, true, false, true},
        {MotionQueryEditorCommandKindUVE::PasteDatabase, MotionQueryEditorCommandPayloadKindUVE::PasteTarget,
         "paste database", "Paste Database", true, false, true, true},
        {MotionQueryEditorCommandKindUVE::Undo, MotionQueryEditorCommandPayloadKindUVE::None,
         "undo", "Undo", true, false, false, false},
        {MotionQueryEditorCommandKindUVE::Redo, MotionQueryEditorCommandPayloadKindUVE::None,
         "redo", "Redo", true, false, false, false},
    };
    return metadata;
}

MotionQueryEditorResponseUVE MotionQueryEditorAuthoringSessionUVE::DispatchUVE(
    const MotionQueryEditorCommandUVE& command) noexcept {
    if (command.protocolVersion != kMotionQueryEditorProtocolVersionUVE) {
        return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidProtocol,
                               "motion query editor protocol version is unsupported");
    }
    if (command.expectedRevision != revision_) {
        return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::StaleRevision,
                               "motion query editor command revision is stale");
    }
    if (command.kind == MotionQueryEditorCommandKindUVE::ReadSnapshot) {
        return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor snapshot read");
    }
    if (command.kind == MotionQueryEditorCommandKindUVE::Undo ||
        command.kind == MotionQueryEditorCommandKindUVE::Redo) {
        if (command.kind == MotionQueryEditorCommandKindUVE::Undo) {
            if (undoHistory_.empty()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::NothingToUndo,
                                       "motion query editor has no undo history");
            }
            StateUVE current = CaptureStateUVE();
            StateUVE target = std::move(undoHistory_.back());
            undoHistory_.pop_back();
            redoHistory_.push_back(std::move(current));
            RestoreStateUVE(std::move(target));
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor undo applied");
        }
        if (redoHistory_.empty()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::NothingToRedo,
                                   "motion query editor has no redo history");
        }
        StateUVE current = CaptureStateUVE();
        StateUVE target = std::move(redoHistory_.back());
        redoHistory_.pop_back();
        undoHistory_.push_back(std::move(current));
        RestoreStateUVE(std::move(target));
        ++revision_;
        return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor redo applied");
    }

    if (command.kind == MotionQueryEditorCommandKindUVE::RegisterDatabase) {
        if (!command.database.has_value()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   "motion query editor database descriptor is missing");
        }
        const MotionQueryEditorUtilityValidationResultUVE entryValidation =
            ValidateMotionQueryEditorDatabaseEntryUVE(*command.database);
        if (!entryValidation.IsValidUVE()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   entryValidation.message);
        }
        if (databases_.size() >= kMotionQueryEditorMaximumDatabasesUVE) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   "motion query editor database capacity is full");
        }
        if (FindDatabaseUVE(command.database->resource) != nullptr) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::DuplicateDatabase,
                                   "motion query editor database resource is already registered");
        }
        StateUVE stateBefore = CaptureStateUVE();
        databases_.push_back(*command.database);
        CommitMutationUVE(std::move(stateBefore));
        return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor database registered");
    }

    if (command.kind == MotionQueryEditorCommandKindUVE::RemoveDatabase) {
        if (!command.resource.has_value() || !IsValidResourceUVE(*command.resource)) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                   "remove database requires a valid resource handle");
        }
        const auto iterator = std::find_if(databases_.begin(), databases_.end(), [&](const auto& entry) {
            return entry.resource == *command.resource;
        });
        if (iterator == databases_.end()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::DatabaseNotFound,
                                   "motion query editor database was not found");
        }
        StateUVE stateBefore = CaptureStateUVE();
        databases_.erase(iterator);
        if (selectedResource_ == command.resource) {
            selectedResource_.reset();
        }
        CommitMutationUVE(std::move(stateBefore));
        return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor database removed");
    }

    if (command.kind == MotionQueryEditorCommandKindUVE::PasteDatabase) {
        if (!clipboard_.has_value()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ClipboardEmpty,
                                   "paste database requires a copied database");
        }
        if (!command.pasteTarget.has_value() ||
            !IsValidResourceUVE(command.pasteTarget->resource) ||
            !IsValidMotionQueryEditorDisplayNameUVE(command.pasteTarget->displayName) ||
            !command.pasteTarget->context.IsValidUVE()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidPasteTarget,
                                   "paste database requires a valid destination identity");
        }
        if (FindDatabaseUVE(command.pasteTarget->resource) != nullptr ||
            std::any_of(databases_.cbegin(), databases_.cend(), [&](const auto& entry) {
                return entry.contract.context.databaseId == command.pasteTarget->context.databaseId;
            })) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::DuplicateDatabase,
                                   "paste database destination identity is already registered");
        }
        if (databases_.size() >= kMotionQueryEditorMaximumDatabasesUVE) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidPasteTarget,
                                   "motion query editor database capacity is full");
        }
        MotionQueryEditorDatabaseEntryUVE pasted = *clipboard_;
        pasted.resource = command.pasteTarget->resource;
        pasted.displayName = command.pasteTarget->displayName;
        pasted.contract.context = command.pasteTarget->context;
        pasted.contract.events.clear();
        pasted.dirty = true;
        const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(pasted.contract);
        if (!validation.IsValidUVE()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidPasteTarget,
                                   validation.message);
        }
        StateUVE stateBefore = CaptureStateUVE();
        databases_.push_back(std::move(pasted));
        CommitMutationUVE(std::move(stateBefore));
        return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor database pasted");
    }

    if (!command.resource.has_value() || !IsValidResourceUVE(*command.resource)) {
        return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                               std::string(CommandNameUVE(command.kind)) +
                                   " requires a valid resource handle");
    }
    MotionQueryEditorDatabaseEntryUVE* entry = FindDatabaseUVE(*command.resource);
    if (entry == nullptr) {
        return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::DatabaseNotFound,
                               "motion query editor database was not found");
    }

    switch (command.kind) {
        case MotionQueryEditorCommandKindUVE::SelectDatabase: {
            StateUVE stateBefore = CaptureStateUVE();
            selectedResource_ = entry->resource;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor database selected");
        }
        case MotionQueryEditorCommandKindUVE::SetDisplayName: {
            if (!command.text.has_value() || !IsValidMotionQueryEditorDisplayNameUVE(*command.text)) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set display name requires bounded text");
            }
            StateUVE stateBefore = CaptureStateUVE();
            entry->displayName = *command.text;
            entry->dirty = true;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor display name updated");
        }
        case MotionQueryEditorCommandKindUVE::SetSchemaId: {
            if (!command.text.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set schema ID requires non-empty text");
            }
            const std::string normalizedSchemaId =
                NormalizeMotionQueryEditorSchemaIdUVE(*command.text);
            if (normalizedSchemaId.empty()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set schema ID requires non-empty text");
            }
            StateUVE stateBefore = CaptureStateUVE();
            const std::string previous = entry->contract.schema.schemaId;
            entry->contract.schema.schemaId = normalizedSchemaId;
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!validation.IsValidUVE()) {
                entry->contract.schema.schemaId = previous;
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       validation.message);
            }
            entry->dirty = true;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor schema ID updated");
        }
        case MotionQueryEditorCommandKindUVE::SetMaximumCandidates: {
            if (!command.candidateIndex.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set maximum candidates requires a bounded value");
            }
            StateUVE stateBefore = CaptureStateUVE();
            const std::size_t previous = entry->contract.settings.maximumCandidates;
            entry->contract.settings.maximumCandidates = *command.candidateIndex;
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!validation.IsValidUVE()) {
                entry->contract.settings.maximumCandidates = previous;
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       validation.message);
            }
            entry->dirty = true;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor candidate limit updated");
        }
        case MotionQueryEditorCommandKindUVE::AddCandidate: {
            if (!command.candidate.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "add candidate requires a candidate value");
            }
            StateUVE stateBefore = CaptureStateUVE();
            const UVE::Core::MotionMatchingDatabaseUVE previous = entry->contract.database;
            const std::size_t previousEventCount = entry->contract.events.size();
            entry->contract.database.candidates.push_back(*command.candidate);
            const MotionQueryEditorUtilityValidationResultUVE candidateValidation =
                ValidateMotionQueryEditorCandidateIdentifiersUVE(entry->contract.database.candidates);
            if (!candidateValidation.IsValidUVE()) {
                entry->contract.database = previous;
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       candidateValidation.message);
            }
            const auto eventResult = UVE::Core::AppendMotionQueryDatabaseEventUVE(
                entry->contract, UVE::Core::MotionQueryDatabaseEventUVE{
                                      UVE::Core::MotionQueryDatabaseEventKindUVE::CandidateAdded, 0U,
                                      command.candidate->candidateId, "candidate added by editor"});
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!eventResult.IsValidUVE() || !validation.IsValidUVE()) {
                entry->contract.database = previous;
                entry->contract.events.resize(previousEventCount);
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       !eventResult.IsValidUVE() ? eventResult.message : validation.message);
            }
            entry->dirty = true;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor candidate added");
        }
        case MotionQueryEditorCommandKindUVE::RemoveCandidate: {
            if (!command.candidateIndex.has_value() ||
                *command.candidateIndex >= entry->contract.database.candidates.size()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::CandidateNotFound,
                                       "remove candidate index is out of range");
            }
            StateUVE stateBefore = CaptureStateUVE();
            const UVE::Core::MotionMatchingDatabaseUVE previous = entry->contract.database;
            const std::size_t previousEventCount = entry->contract.events.size();
            const std::string removedId =
                entry->contract.database.candidates[*command.candidateIndex].candidateId;
            entry->contract.database.candidates.erase(
                entry->contract.database.candidates.begin() +
                static_cast<std::ptrdiff_t>(*command.candidateIndex));
            const auto eventResult = UVE::Core::AppendMotionQueryDatabaseEventUVE(
                entry->contract, UVE::Core::MotionQueryDatabaseEventUVE{
                                      UVE::Core::MotionQueryDatabaseEventKindUVE::CandidateRemoved, 0U,
                                      removedId, "candidate removed by editor"});
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!eventResult.IsValidUVE() || !validation.IsValidUVE()) {
                entry->contract.database = previous;
                entry->contract.events.resize(previousEventCount);
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       !eventResult.IsValidUVE() ? eventResult.message : validation.message);
            }
            entry->dirty = true;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor candidate removed");
        }
        case MotionQueryEditorCommandKindUVE::ValidateDatabase: {
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            return MakeResponseUVE(command, validation.IsValidUVE(),
                                   validation.IsValidUVE() ? MotionQueryEditorResponseCodeUVE::Applied
                                                           : MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                   validation.message);
        }
        case MotionQueryEditorCommandKindUVE::CopyDatabase: {
            StateUVE stateBefore = CaptureStateUVE();
            clipboard_ = *entry;
            CommitMutationUVE(std::move(stateBefore));
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor database copied");
        }
        case MotionQueryEditorCommandKindUVE::ReadSnapshot:
        case MotionQueryEditorCommandKindUVE::RegisterDatabase:
        case MotionQueryEditorCommandKindUVE::RemoveDatabase:
        case MotionQueryEditorCommandKindUVE::PasteDatabase:
        case MotionQueryEditorCommandKindUVE::Undo:
        case MotionQueryEditorCommandKindUVE::Redo:
            break;
    }
    return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                           "motion query editor command is unsupported");
}

void MotionQueryEditorAuthoringSessionUVE::ClearUVE() noexcept {
    databases_.clear();
    selectedResource_.reset();
    clipboard_.reset();
    undoHistory_.clear();
    redoHistory_.clear();
    revision_ = 0U;
}

MotionQueryEditorAuthoringSessionUVE::StateUVE MotionQueryEditorAuthoringSessionUVE::CaptureStateUVE() const {
    return StateUVE{databases_, selectedResource_, clipboard_};
}

void MotionQueryEditorAuthoringSessionUVE::RestoreStateUVE(StateUVE state) noexcept {
    databases_ = std::move(state.databases);
    selectedResource_ = std::move(state.selectedResource);
    clipboard_ = std::move(state.clipboard);
}

void MotionQueryEditorAuthoringSessionUVE::CommitMutationUVE(StateUVE stateBefore) {
    if (undoHistory_.size() >= kMotionQueryEditorMaximumHistoryEntriesUVE) {
        undoHistory_.erase(undoHistory_.begin());
    }
    undoHistory_.push_back(std::move(stateBefore));
    redoHistory_.clear();
    ++revision_;
}

MotionQueryEditorSnapshotUVE MotionQueryEditorAuthoringSessionUVE::GetSnapshotUVE() const noexcept {
    MotionQueryEditorSnapshotUVE snapshot;
    snapshot.revision = revision_;
    snapshot.selectedResource = selectedResource_;
    snapshot.commandMetadata = GetMotionQueryEditorCommandMetadataUVE();
    snapshot.propertyMetadata = GetMotionQueryEditorPropertyMetadataUVE();
    snapshot.clipboardAvailable = clipboard_.has_value();
    snapshot.canUndo = !undoHistory_.empty();
    snapshot.canRedo = !redoHistory_.empty();
    snapshot.databases.reserve(databases_.size());
    for (const MotionQueryEditorDatabaseEntryUVE& entry : databases_) {
        snapshot.databases.push_back(BuildRowUVE(entry, selectedResource_));
    }
    std::sort(snapshot.databases.begin(), snapshot.databases.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.displayName != rhs.displayName) {
            return lhs.displayName < rhs.displayName;
        }
        return IsHandleBeforeUVE(lhs.resource, rhs.resource);
    });
    snapshot.diagnostic = "native Motion Query editor authoring snapshot";
    return snapshot;
}

bool MotionQueryEditorAuthoringSessionUVE::TryGetDatabaseCopyUVE(
    const UVE::Asset::ResourceHandleUVE resource,
    UVE::Core::MotionQueryDatabaseContractUVE& destination) const noexcept {
    const MotionQueryEditorDatabaseEntryUVE* entry = FindDatabaseUVE(resource);
    if (entry == nullptr) {
        return false;
    }
    destination = entry->contract;
    return true;
}

MotionQueryEditorResponseUVE MotionQueryEditorAuthoringSessionUVE::MakeResponseUVE(
    const MotionQueryEditorCommandUVE& command, const bool applied,
    const MotionQueryEditorResponseCodeUVE code, std::string message) const {
    MotionQueryEditorResponseUVE response;
    response.requestId = command.requestId;
    response.applied = applied;
    response.code = code;
    response.message = std::move(message);
    response.snapshot = GetSnapshotUVE();
    return response;
}

MotionQueryEditorDatabaseEntryUVE* MotionQueryEditorAuthoringSessionUVE::FindDatabaseUVE(
    const UVE::Asset::ResourceHandleUVE resource) noexcept {
    const auto iterator = std::find_if(databases_.begin(), databases_.end(), [resource](const auto& entry) {
        return entry.resource == resource;
    });
    return iterator == databases_.end() ? nullptr : &*iterator;
}

const MotionQueryEditorDatabaseEntryUVE* MotionQueryEditorAuthoringSessionUVE::FindDatabaseUVE(
    const UVE::Asset::ResourceHandleUVE resource) const noexcept {
    const auto iterator = std::find_if(databases_.cbegin(), databases_.cend(), [resource](const auto& entry) {
        return entry.resource == resource;
    });
    return iterator == databases_.cend() ? nullptr : &*iterator;
}

bool MotionQueryEditorAuthoringSessionUVE::IsValidResourceUVE(
    const UVE::Asset::ResourceHandleUVE resource) noexcept {
    return IsValidResourceHandleUVE(resource);
}

MotionQueryEditorDatabaseRowUVE MotionQueryEditorAuthoringSessionUVE::BuildRowUVE(
    const MotionQueryEditorDatabaseEntryUVE& entry,
    const std::optional<UVE::Asset::ResourceHandleUVE> selectedResource) {
    MotionQueryEditorDatabaseRowUVE row;
    row.resource = entry.resource;
    row.displayName = entry.displayName;
    row.databaseId = entry.contract.context.databaseId;
    row.generation = entry.contract.context.generation;
    row.schemaVersion = entry.contract.schema.version;
    row.schemaId = entry.contract.schema.schemaId;
    row.candidateCount = entry.contract.database.candidates.size();
    row.maximumCandidates = entry.contract.settings.maximumCandidates;
    row.valid = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry.contract).IsValidUVE();
    row.selected = selectedResource.has_value() && selectedResource == entry.resource;
    row.dirty = entry.dirty;
    return row;
}

} // namespace UVE::Plugins::Editor
