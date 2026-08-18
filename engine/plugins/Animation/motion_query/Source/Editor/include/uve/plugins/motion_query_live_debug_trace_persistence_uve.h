// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_debugging_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Plugins::Editor {

inline constexpr std::uint32_t kMotionQueryLiveDebugTraceEnvelopeSchemaVersionUVE = 1U;
inline constexpr std::size_t kMotionQueryMaximumLiveDebugTraceEnvelopeBytesUVE = 1U << 20U;

struct MotionQueryLiveDebugTraceEnvelopeUVE final {
    std::uint32_t schemaVersion = kMotionQueryLiveDebugTraceEnvelopeSchemaVersionUVE;
    bool truncated = false;
    std::string filter;
    std::vector<MotionQueryTraceEventUVE> events;

    [[nodiscard]] bool operator==(const MotionQueryLiveDebugTraceEnvelopeUVE&) const = default;
};

enum class MotionQueryLiveDebugTracePersistenceCodeUVE : std::uint8_t {
    Accepted = 0,
    EmptyPayload,
    PayloadTooLarge,
    ParseError,
    SchemaMismatch,
    UnexpectedField,
    InvalidTrace,
};

struct MotionQueryLiveDebugTraceSerializationResultUVE final {
    MotionQueryLiveDebugTracePersistenceCodeUVE code = MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace;
    std::string payload;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryLiveDebugTracePersistenceCodeUVE::Accepted;
    }
};

struct MotionQueryLiveDebugTraceDeserializationResultUVE final {
    MotionQueryLiveDebugTracePersistenceCodeUVE code = MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace;
    std::optional<MotionQueryLiveDebugTraceEnvelopeUVE> envelope;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryLiveDebugTracePersistenceCodeUVE::Accepted && envelope.has_value();
    }
};

[[nodiscard]] MotionQueryLiveDebugTraceSerializationResultUVE SerializeMotionQueryLiveDebugTraceUVE(
    const MotionQueryTraceSnapshotUVE& snapshot, std::string_view filter);

[[nodiscard]] MotionQueryLiveDebugTraceDeserializationResultUVE DeserializeMotionQueryLiveDebugTraceUVE(
    std::string_view payload);

struct MotionQueryLiveDebugTraceKindCountUVE final {
    std::string kind;
    std::size_t count = 0U;

    [[nodiscard]] bool operator==(const MotionQueryLiveDebugTraceKindCountUVE&) const = default;
};

struct MotionQueryLiveDebugTraceAnalysisUVE final {
    std::size_t eventCount = 0U;
    double totalCost = 0.0;
    std::size_t maximumCandidatesEvaluated = 0U;
    std::vector<MotionQueryLiveDebugTraceKindCountUVE> kindCounts;

    [[nodiscard]] bool operator==(const MotionQueryLiveDebugTraceAnalysisUVE&) const = default;
};

enum class MotionQueryLiveDebugTraceAnalysisCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidTrace,
    CapacityExceeded,
};

struct MotionQueryLiveDebugTraceAnalysisResultUVE final {
    MotionQueryLiveDebugTraceAnalysisCodeUVE code = MotionQueryLiveDebugTraceAnalysisCodeUVE::InvalidTrace;
    std::optional<MotionQueryLiveDebugTraceAnalysisUVE> analysis;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryLiveDebugTraceAnalysisCodeUVE::Accepted && analysis.has_value();
    }
};

[[nodiscard]] MotionQueryLiveDebugTraceAnalysisResultUVE AnalyzeMotionQueryLiveDebugTraceUVE(
    const MotionQueryTraceSnapshotUVE& snapshot);

} // namespace UVE::Plugins::Editor

