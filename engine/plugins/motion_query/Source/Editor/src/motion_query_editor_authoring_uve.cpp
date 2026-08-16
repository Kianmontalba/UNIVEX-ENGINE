// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_editor_authoring_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Plugins::Editor {
namespace {
[[nodiscard]] const char* CommandNameUVE(const MotionQueryEditorCommandKindUVE kind) noexcept {
    switch (kind) {
        case MotionQueryEditorCommandKindUVE::ReadSnapshot:
            return "read snapshot";
        case MotionQueryEditorCommandKindUVE::RegisterDatabase:
            return "register database";
        case MotionQueryEditorCommandKindUVE::RemoveDatabase:
            return "remove database";
        case MotionQueryEditorCommandKindUVE::SelectDatabase:
            return "select database";
        case MotionQueryEditorCommandKindUVE::SetDisplayName:
            return "set display name";
        case MotionQueryEditorCommandKindUVE::SetSchemaId:
            return "set schema ID";
        case MotionQueryEditorCommandKindUVE::SetMaximumCandidates:
            return "set maximum candidates";
        case MotionQueryEditorCommandKindUVE::AddCandidate:
            return "add candidate";
        case MotionQueryEditorCommandKindUVE::RemoveCandidate:
            return "remove candidate";
        case MotionQueryEditorCommandKindUVE::ValidateDatabase:
            return "validate database";
    }
    return "unknown command";
}

[[nodiscard]] bool IsHandleBeforeUVE(const UVE::Asset::ResourceHandleUVE lhs,
                                     const UVE::Asset::ResourceHandleUVE rhs) noexcept {
    if (lhs.guid.value != rhs.guid.value) {
        return lhs.guid.value < rhs.guid.value;
    }
    return lhs.generation < rhs.generation;
}
} // namespace

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

    if (command.kind == MotionQueryEditorCommandKindUVE::RegisterDatabase) {
        if (!command.database.has_value() || !IsValidResourceUVE(command.database->resource) ||
            !IsValidDisplayNameUVE(command.database->displayName)) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   "motion query editor database descriptor is invalid");
        }
        const UVE::Core::MotionQueryDatabaseContractResultUVE validation =
            UVE::Core::ValidateMotionQueryDatabaseContractUVE(command.database->contract);
        if (!validation.IsValidUVE()) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   validation.message);
        }
        if (databases_.size() >= kMotionQueryEditorMaximumDatabasesUVE) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidDatabase,
                                   "motion query editor database capacity is full");
        }
        if (FindDatabaseUVE(command.database->resource) != nullptr) {
            return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::DuplicateDatabase,
                                   "motion query editor database resource is already registered");
        }
        databases_.push_back(*command.database);
        ++revision_;
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
        databases_.erase(iterator);
        if (selectedResource_ == command.resource) {
            selectedResource_.reset();
        }
        ++revision_;
        return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                               "motion query editor database removed");
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
        case MotionQueryEditorCommandKindUVE::SelectDatabase:
            selectedResource_ = entry->resource;
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor database selected");
        case MotionQueryEditorCommandKindUVE::SetDisplayName:
            if (!command.text.has_value() || !IsValidDisplayNameUVE(*command.text)) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set display name requires bounded text");
            }
            entry->displayName = *command.text;
            entry->dirty = true;
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor display name updated");
        case MotionQueryEditorCommandKindUVE::SetSchemaId: {
            if (!command.text.has_value() || command.text->empty()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set schema ID requires non-empty text");
            }
            const std::string previous = entry->contract.schema.schemaId;
            entry->contract.schema.schemaId = *command.text;
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!validation.IsValidUVE()) {
                entry->contract.schema.schemaId = previous;
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       validation.message);
            }
            entry->dirty = true;
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor schema ID updated");
        }
        case MotionQueryEditorCommandKindUVE::SetMaximumCandidates: {
            if (!command.candidateIndex.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "set maximum candidates requires a bounded value");
            }
            const std::size_t previous = entry->contract.settings.maximumCandidates;
            entry->contract.settings.maximumCandidates = *command.candidateIndex;
            const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(entry->contract);
            if (!validation.IsValidUVE()) {
                entry->contract.settings.maximumCandidates = previous;
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::ValidationFailed,
                                       validation.message);
            }
            entry->dirty = true;
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor candidate limit updated");
        }
        case MotionQueryEditorCommandKindUVE::AddCandidate: {
            if (!command.candidate.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                                       "add candidate requires a candidate value");
            }
            const UVE::Core::MotionMatchingDatabaseUVE previous = entry->contract.database;
            const std::size_t previousEventCount = entry->contract.events.size();
            entry->contract.database.candidates.push_back(*command.candidate);
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
            ++revision_;
            return MakeResponseUVE(command, true, MotionQueryEditorResponseCodeUVE::Applied,
                                   "motion query editor candidate added");
        }
        case MotionQueryEditorCommandKindUVE::RemoveCandidate: {
            if (!command.candidateIndex.has_value() ||
                *command.candidateIndex >= entry->contract.database.candidates.size()) {
                return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::CandidateNotFound,
                                       "remove candidate index is out of range");
            }
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
            ++revision_;
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
        case MotionQueryEditorCommandKindUVE::ReadSnapshot:
        case MotionQueryEditorCommandKindUVE::RegisterDatabase:
        case MotionQueryEditorCommandKindUVE::RemoveDatabase:
            break;
    }
    return MakeResponseUVE(command, false, MotionQueryEditorResponseCodeUVE::InvalidCommand,
                           "motion query editor command is unsupported");
}

void MotionQueryEditorAuthoringSessionUVE::ClearUVE() noexcept {
    databases_.clear();
    selectedResource_.reset();
    revision_ = 0U;
}

MotionQueryEditorSnapshotUVE MotionQueryEditorAuthoringSessionUVE::GetSnapshotUVE() const noexcept {
    MotionQueryEditorSnapshotUVE snapshot;
    snapshot.revision = revision_;
    snapshot.selectedResource = selectedResource_;
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
    return resource.guid.value != 0U && resource.generation != 0U;
}

bool MotionQueryEditorAuthoringSessionUVE::IsValidDisplayNameUVE(const std::string& displayName) noexcept {
    return !displayName.empty() && displayName.size() <= kMotionQueryEditorMaximumDisplayNameBytesUVE;
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
