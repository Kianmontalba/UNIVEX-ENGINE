// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/motion_query_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Core {

inline constexpr std::uint32_t kMotionQueryDatabaseSchemaVersionUVE = 1U;
inline constexpr std::size_t kMaximumMotionQueryFeatureChannelsUVE = 64U;
inline constexpr std::size_t kMaximumMotionQueryDatabaseEventsUVE = 256U;
inline constexpr std::size_t kMaximumMotionQueryDatabaseEventMessageBytesUVE = 256U;

struct MotionQueryDatabaseContextUVE final {
    std::string databaseId;
    std::uint64_t generation = 0U;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return !databaseId.empty() && generation != 0U;
    }

    [[nodiscard]] bool operator==(const MotionQueryDatabaseContextUVE&) const = default;
};

struct MotionQueryDatabaseSchemaUVE final {
    std::uint32_t version = kMotionQueryDatabaseSchemaVersionUVE;
    std::string schemaId;
    std::vector<double> trajectoryOffsets;
    std::vector<std::string> featureChannelIds;

    [[nodiscard]] bool operator==(const MotionQueryDatabaseSchemaUVE&) const = default;
};

struct MotionQueryDatabaseSettingsUVE final {
    std::size_t maximumCandidates = MotionMatchingDatabaseUVE::kMaximumCandidatesUVE;
    bool requireTrajectorySchema = true;

    [[nodiscard]] bool operator==(const MotionQueryDatabaseSettingsUVE&) const = default;
};

enum class MotionQueryDatabaseEventKindUVE : std::uint8_t {
    SchemaValidated = 0,
    CandidateAdded,
    CandidateRemoved,
    MatchRequested,
    MatchCompleted,
    Invalidated,
};

struct MotionQueryDatabaseEventUVE final {
    MotionQueryDatabaseEventKindUVE kind = MotionQueryDatabaseEventKindUVE::SchemaValidated;
    std::uint64_t sequence = 0U;
    std::string candidateId;
    std::string message;

    [[nodiscard]] bool operator==(const MotionQueryDatabaseEventUVE&) const = default;
};

struct MotionQueryDatabaseContractUVE final {
    MotionQueryDatabaseContextUVE context;
    MotionQueryDatabaseSchemaUVE schema;
    MotionQueryDatabaseSettingsUVE settings;
    MotionMatchingDatabaseUVE database;
    std::vector<MotionQueryDatabaseEventUVE> events;
};

enum class MotionQueryDatabaseContractCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidContext,
    InvalidSchema,
    InvalidSettings,
    DatabaseValidationFailed,
    SchemaMismatch,
    EventCapacityExceeded,
    InvalidEvent,
};

struct MotionQueryDatabaseContractResultUVE final {
    MotionQueryDatabaseContractCodeUVE code = MotionQueryDatabaseContractCodeUVE::InvalidContext;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryDatabaseContractCodeUVE::Valid;
    }
};

[[nodiscard]] MotionQueryDatabaseContractResultUVE ValidateMotionQueryDatabaseContractUVE(
    const MotionQueryDatabaseContractUVE& contract) noexcept;

[[nodiscard]] MotionQueryDatabaseContractResultUVE AppendMotionQueryDatabaseEventUVE(
    MotionQueryDatabaseContractUVE& contract, MotionQueryDatabaseEventUVE event);

struct MotionQueryDatabaseFactoryResultUVE final {
    MotionQueryDatabaseContractUVE contract;
    MotionQueryDatabaseContractResultUVE validation;

    [[nodiscard]] bool IsCreatedUVE() const noexcept {
        return validation.IsValidUVE();
    }
};

[[nodiscard]] MotionQueryDatabaseFactoryResultUVE CreateMotionQueryDatabaseContractUVE(
    MotionQueryDatabaseContextUVE context, MotionQueryDatabaseSchemaUVE schema,
    MotionQueryDatabaseSettingsUVE settings, MotionMatchingDatabaseUVE database);

[[nodiscard]] MotionQueryDatabaseFactoryResultUVE CreateDefaultMotionQueryDatabaseContractUVE(
    std::string_view databaseId, std::uint64_t generation, std::string_view schemaId,
    std::size_t maximumCandidates = 4U);

} // namespace UVE::Core
