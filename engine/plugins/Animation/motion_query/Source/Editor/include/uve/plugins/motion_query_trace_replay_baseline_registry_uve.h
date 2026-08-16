// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Plugins::Editor {

inline constexpr std::size_t kMotionQueryMaximumReplayBaselinesUVE = 32U;
inline constexpr std::size_t kMotionQueryMaximumReplayBaselineNameBytesUVE = 128U;
inline constexpr std::uint32_t kMotionQueryReplayBaselineEnvelopeSchemaVersionUVE = 1U;
inline constexpr std::size_t kMotionQueryMaximumReplayBaselineEnvelopeBytesUVE =
    kMotionQueryMaximumTraceReplayPayloadBytesUVE;

enum class MotionQueryTraceReplayBaselineCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidName,
    InvalidFixture,
    DuplicateReplacement,
    CapacityExceeded,
    NotFound,
    StaleGeneration,
};

struct MotionQueryTraceReplayBaselineResultUVE final {
    MotionQueryTraceReplayBaselineCodeUVE code =
        MotionQueryTraceReplayBaselineCodeUVE::InvalidFixture;
    std::size_t index = 0U;
    std::uint64_t registryGeneration = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplayBaselineCodeUVE::Accepted ||
               code == MotionQueryTraceReplayBaselineCodeUVE::DuplicateReplacement;
    }
};

struct MotionQueryTraceReplayBaselineEntryUVE final {
    std::string name;
    std::uint64_t sourceGeneration = 0U;
    std::size_t eventCount = 0U;
    bool truncated = false;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplayBaselineEntryUVE&) const = default;
};

struct MotionQueryTraceReplayBaselineSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool truncated = false;
    std::vector<MotionQueryTraceReplayBaselineEntryUVE> entries;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplayBaselineSnapshotUVE&) const = default;
};

struct MotionQueryTraceReplayBaselineEnvelopeSerializationResultUVE final {
    MotionQueryTraceReplaySerializationCodeUVE code =
        MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture;
    std::string payload;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplaySerializationCodeUVE::Accepted;
    }
};

struct MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE final {
    MotionQueryTraceReplaySerializationCodeUVE code =
        MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture;
    std::size_t importedBaselineCount = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplaySerializationCodeUVE::Accepted;
    }
};

struct MotionQueryTraceReplayBaselineSelectionUVE final {
    MotionQueryTraceReplayBaselineCodeUVE code = MotionQueryTraceReplayBaselineCodeUVE::NotFound;
    std::uint64_t registryGeneration = 0U;
    std::optional<MotionQueryTraceReplayFixtureUVE> fixture;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplayBaselineCodeUVE::Accepted && fixture.has_value();
    }
};

class MotionQueryTraceReplayBaselineRegistryUVE final {
public:
    [[nodiscard]] MotionQueryTraceReplayBaselineResultUVE RegisterUVE(
        std::string_view name, const MotionQueryTraceReplayFixtureUVE& fixture);
    [[nodiscard]] MotionQueryTraceReplayBaselineResultUVE RemoveUVE(std::string_view name);
    [[nodiscard]] MotionQueryTraceReplayBaselineResultUVE RenameUVE(std::string_view oldName, std::string_view newName);
    [[nodiscard]] MotionQueryTraceReplayBaselineResultUVE ClearUVE() noexcept;

    [[nodiscard]] MotionQueryTraceReplayBaselineSelectionUVE SelectUVE(
        std::string_view name, std::optional<std::uint64_t> expectedRegistryGeneration = std::nullopt) const;
    [[nodiscard]] MotionQueryTraceReplayBaselineSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] MotionQueryTraceReplayBaselineEnvelopeSerializationResultUVE SerializeEnvelopeUVE() const;
    [[nodiscard]] MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE DeserializeEnvelopeUVE(
        std::string_view payload);

private:
    struct StoredBaselineUVE final {
        std::string name;
        MotionQueryTraceReplayFixtureUVE fixture;
    };

    [[nodiscard]] static bool IsValidNameUVE(std::string_view name) noexcept;
    [[nodiscard]] static std::uint64_t SourceGenerationUVE(
        const MotionQueryTraceReplayFixtureUVE& fixture) noexcept;
    void IncrementGenerationUVE() noexcept;

    std::vector<StoredBaselineUVE> baselines_;
    std::uint64_t generation_ = 0U;
};

} // namespace UVE::Plugins::Editor
