// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_database_contract_uve.h"

#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

[[nodiscard]] MotionQueryDatabaseContractResultUVE MakeErrorUVE(
    MotionQueryDatabaseContractCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryDatabaseContractResultUVE{code, index, message};
}

[[nodiscard]] bool HasDuplicateStringUVE(const std::vector<std::string>& values,
                                         std::size_t beforeIndex) noexcept {
    for (std::size_t index = 0U; index < beforeIndex; ++index) {
        if (values[index] == values[beforeIndex]) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool HasDuplicateOffsetUVE(const std::vector<double>& values,
                                         std::size_t beforeIndex) noexcept {
    for (std::size_t index = 0U; index < beforeIndex; ++index) {
        if (values[index] == values[beforeIndex]) {
            return true;
        }
    }
    return false;
}

} // namespace

MotionQueryDatabaseContractResultUVE ValidateMotionQueryDatabaseContractUVE(
    const MotionQueryDatabaseContractUVE& contract) noexcept {
    if (!contract.context.IsValidUVE()) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidContext, 0U,
                            "motion query database context is invalid");
    }
    if (contract.schema.version != kMotionQueryDatabaseSchemaVersionUVE ||
        contract.schema.schemaId.empty() ||
        contract.schema.trajectoryOffsets.size() > MotionQueryUVE::kMaximumTrajectorySamplesUVE ||
        contract.schema.featureChannelIds.size() > kMaximumMotionQueryFeatureChannelsUVE) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidSchema, 0U,
                            "motion query database schema is invalid or exceeds its bounds");
    }
    for (std::size_t index = 0U; index < contract.schema.trajectoryOffsets.size(); ++index) {
        const double offsetSeconds = contract.schema.trajectoryOffsets[index];
        if (!std::isfinite(offsetSeconds) || offsetSeconds < 0.0 ||
            (index > 0U && offsetSeconds < contract.schema.trajectoryOffsets[index - 1U]) ||
            HasDuplicateOffsetUVE(contract.schema.trajectoryOffsets, index)) {
            return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidSchema, index,
                                "motion query schema trajectory offsets must be finite, sorted, and unique");
        }
    }
    for (std::size_t index = 0U; index < contract.schema.featureChannelIds.size(); ++index) {
        if (contract.schema.featureChannelIds[index].empty() ||
            contract.schema.featureChannelIds[index].size() > MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE ||
            HasDuplicateStringUVE(contract.schema.featureChannelIds, index)) {
            return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidSchema, index,
                                "motion query schema feature channel IDs must be bounded and unique");
        }
    }
    if (contract.settings.maximumCandidates == 0U ||
        contract.settings.maximumCandidates > MotionMatchingDatabaseUVE::kMaximumCandidatesUVE ||
        contract.database.candidates.size() > contract.settings.maximumCandidates) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidSettings, 0U,
                            "motion query database settings exceed candidate bounds");
    }
    const MotionMatchingDatabaseValidationResultUVE databaseValidation =
        ValidateMotionMatchingDatabaseUVE(contract.database);
    if (!databaseValidation.IsValidUVE()) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::DatabaseValidationFailed,
                            databaseValidation.index, databaseValidation.message.c_str());
    }
    if (contract.settings.requireTrajectorySchema &&
        contract.database.candidates.front().feature.trajectory.size() !=
            contract.schema.trajectoryOffsets.size()) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::SchemaMismatch, 0U,
                            "motion query database trajectory schema does not match the contract schema");
    }
    for (std::size_t index = 0U; index < contract.events.size(); ++index) {
        const MotionQueryDatabaseEventUVE& event = contract.events[index];
        if (event.sequence != static_cast<std::uint64_t>(index + 1U) ||
            event.message.size() > kMaximumMotionQueryDatabaseEventMessageBytesUVE ||
            (event.kind == MotionQueryDatabaseEventKindUVE::CandidateAdded && event.candidateId.empty())) {
            return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidEvent, index,
                                "motion query database event sequence or payload is invalid");
        }
    }
    if (contract.events.size() > kMaximumMotionQueryDatabaseEventsUVE) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::EventCapacityExceeded, 0U,
                            "motion query database event history exceeds its bounded capacity");
    }
    return MotionQueryDatabaseContractResultUVE{MotionQueryDatabaseContractCodeUVE::Valid, 0U,
                                                "valid"};
}

MotionQueryDatabaseContractResultUVE AppendMotionQueryDatabaseEventUVE(
    MotionQueryDatabaseContractUVE& contract, MotionQueryDatabaseEventUVE event) {
    if (contract.events.size() >= kMaximumMotionQueryDatabaseEventsUVE) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::EventCapacityExceeded,
                            contract.events.size(), "motion query database event history is full");
    }
    const std::uint64_t expectedSequence = static_cast<std::uint64_t>(contract.events.size() + 1U);
    if (event.sequence == 0U) {
        event.sequence = expectedSequence;
    }
    if (event.sequence != expectedSequence || event.message.size() > kMaximumMotionQueryDatabaseEventMessageBytesUVE ||
        (event.kind == MotionQueryDatabaseEventKindUVE::CandidateAdded && event.candidateId.empty())) {
        return MakeErrorUVE(MotionQueryDatabaseContractCodeUVE::InvalidEvent,
                            contract.events.size(), "motion query database event payload is invalid");
    }
    contract.events.push_back(std::move(event));
    return MotionQueryDatabaseContractResultUVE{MotionQueryDatabaseContractCodeUVE::Valid, 0U,
                                                "valid"};
}

} // namespace UVE::Core
