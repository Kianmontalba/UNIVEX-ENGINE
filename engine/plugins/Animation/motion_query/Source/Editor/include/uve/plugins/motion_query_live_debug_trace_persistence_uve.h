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

} // namespace UVE::Plugins::Editor

