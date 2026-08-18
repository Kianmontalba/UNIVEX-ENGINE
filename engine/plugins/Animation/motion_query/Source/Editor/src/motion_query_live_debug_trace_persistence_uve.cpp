#include "uve/plugins/motion_query_live_debug_trace_persistence_uve.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <map>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Plugins::Editor {
namespace {

using JsonUVE = nlohmann::json;

[[nodiscard]] bool HasOnlyKeysUVE(const JsonUVE& document,
                                  const std::initializer_list<std::string_view> allowedKeys) {
    if (!document.is_object()) {
        return false;
    }
    for (const auto& item : document.items()) {
        bool allowed = false;
        for (const std::string_view key : allowedKeys) {
            if (item.key() == key) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] JsonUVE EventToJsonUVE(const MotionQueryTraceEventUVE& event) {
    return JsonUVE{{"sequence", event.sequence},
                   {"frameNumber", event.frameNumber},
                   {"kind", event.kind},
                   {"candidatesConsidered", event.candidatesConsidered},
                   {"candidatesEvaluated", event.candidatesEvaluated},
                   {"cost", event.cost},
                   {"selectedCandidateIndex", event.selectedCandidateIndex.has_value()
                                                    ? JsonUVE(*event.selectedCandidateIndex)
                                                    : JsonUVE(nullptr)},
                   {"qualityTier", event.qualityTier},
                   {"continuityCode", event.continuityCode},
                   {"continuityApplied", event.continuityApplied},
                   {"transitionCode", event.transitionCode},
                   {"transitionHeldPrevious", event.transitionHeldPrevious},
                   {"telemetryCode", event.telemetryCode},
                   {"telemetryIndexEntryCount", event.telemetryIndexEntryCount},
                   {"telemetryCandidatesConsidered", event.telemetryCandidatesConsidered},
                   {"telemetryBudgetSaturated", event.telemetryBudgetSaturated},
                   {"provenance", event.provenance},
                   {"message", event.message},
                   {"comment", event.comment},
                   {"category", event.category},
                   {"pinned", event.pinned}};
}

[[nodiscard]] MotionQueryTraceEventUVE EventFromJsonUVE(const JsonUVE& value) {
    MotionQueryTraceEventUVE event;
    event.sequence = value.at("sequence").get<std::uint64_t>();
    event.frameNumber = value.at("frameNumber").get<std::uint64_t>();
    event.kind = value.at("kind").get<std::string>();
    event.candidatesConsidered = value.at("candidatesConsidered").get<std::size_t>();
    event.candidatesEvaluated = value.at("candidatesEvaluated").get<std::size_t>();
    event.cost = value.at("cost").get<float>();
    if (!value.at("selectedCandidateIndex").is_null()) {
        event.selectedCandidateIndex = value.at("selectedCandidateIndex").get<std::size_t>();
    }
    event.qualityTier = value.at("qualityTier").get<std::uint8_t>();
    event.continuityCode = value.at("continuityCode").get<std::uint8_t>();
    event.continuityApplied = value.at("continuityApplied").get<bool>();
    event.transitionCode = value.at("transitionCode").get<std::uint8_t>();
    event.transitionHeldPrevious = value.at("transitionHeldPrevious").get<bool>();
    event.telemetryCode = value.at("telemetryCode").get<std::uint8_t>();
    event.telemetryIndexEntryCount = value.at("telemetryIndexEntryCount").get<std::size_t>();
    event.telemetryCandidatesConsidered = value.at("telemetryCandidatesConsidered").get<std::size_t>();
    event.telemetryBudgetSaturated = value.at("telemetryBudgetSaturated").get<bool>();
    event.provenance = value.at("provenance").get<std::string>();
    event.message = value.at("message").get<std::string>();
    event.comment = value.at("comment").get<std::string>();
    event.category = value.at("category").get<std::string>();
    event.pinned = value.at("pinned").get<bool>();
    return event;
}

[[nodiscard]] bool IsValidPersistedEventUVE(const MotionQueryTraceEventUVE& event,
                                            const std::optional<MotionQueryTraceEventUVE>& previous) {
    if (event.sequence == 0U || !std::isfinite(event.cost) || event.cost < 0.0F ||
        event.kind.empty() || event.kind.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        event.provenance.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        event.message.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        event.comment.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        event.category.size() > 32U || event.candidatesEvaluated > event.candidatesConsidered) {
        return false;
    }
    if (previous.has_value() &&
        (event.sequence <= previous->sequence || event.frameNumber < previous->frameNumber)) {
        return false;
    }
    return true;
}

[[nodiscard]] MotionQueryLiveDebugTraceSerializationResultUVE MakeSerializationResultUVE(
    const MotionQueryLiveDebugTracePersistenceCodeUVE code, std::string payload, std::string message) {
    return {code, std::move(payload), std::move(message)};
}

} // namespace

MotionQueryLiveDebugTraceSerializationResultUVE SerializeMotionQueryLiveDebugTraceUVE(
    const MotionQueryTraceSnapshotUVE& snapshot, const std::string_view filter) {
    if (filter.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        snapshot.events.size() > kMotionQueryMaximumTraceEventsUVE) {
        return MakeSerializationResultUVE(
            MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace, {},
            "motion query live debug trace exceeds a bounded persistence limit");
    }
    std::optional<MotionQueryTraceEventUVE> previous;
    for (const auto& event : snapshot.events) {
        if (!IsValidPersistedEventUVE(event, previous)) {
            return MakeSerializationResultUVE(
                MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace, {},
                "motion query live debug trace contains an invalid event");
        }
        previous = event;
    }

    JsonUVE events = JsonUVE::array();
    for (const auto& event : snapshot.events) {
        events.push_back(EventToJsonUVE(event));
    }
    const JsonUVE document{{"schemaVersion", kMotionQueryLiveDebugTraceEnvelopeSchemaVersionUVE},
                           {"truncated", snapshot.truncated},
                           {"filter", std::string(filter)},
                           {"events", std::move(events)}};
    std::string payload = document.dump();
    if (payload.size() > kMotionQueryMaximumLiveDebugTraceEnvelopeBytesUVE) {
        return MakeSerializationResultUVE(
            MotionQueryLiveDebugTracePersistenceCodeUVE::PayloadTooLarge, {},
            "motion query live debug trace payload exceeds the bounded serialization limit");
    }
    return MakeSerializationResultUVE(
        MotionQueryLiveDebugTracePersistenceCodeUVE::Accepted, std::move(payload),
        "motion query live debug trace serialized");
}

MotionQueryLiveDebugTraceDeserializationResultUVE DeserializeMotionQueryLiveDebugTraceUVE(
    const std::string_view payload) {
    if (payload.empty()) {
        return {MotionQueryLiveDebugTracePersistenceCodeUVE::EmptyPayload, std::nullopt,
                "motion query live debug trace payload is empty"};
    }
    if (payload.size() > kMotionQueryMaximumLiveDebugTraceEnvelopeBytesUVE) {
        return {MotionQueryLiveDebugTracePersistenceCodeUVE::PayloadTooLarge, std::nullopt,
                "motion query live debug trace payload exceeds the bounded serialization limit"};
    }

    try {
        const JsonUVE document = JsonUVE::parse(payload);
        if (!HasOnlyKeysUVE(document, {"schemaVersion", "truncated", "filter", "events"})) {
            return {MotionQueryLiveDebugTracePersistenceCodeUVE::UnexpectedField, std::nullopt,
                    "motion query live debug trace contains an unexpected field"};
        }
        if (document.at("schemaVersion").get<std::uint32_t>() !=
            kMotionQueryLiveDebugTraceEnvelopeSchemaVersionUVE) {
            return {MotionQueryLiveDebugTracePersistenceCodeUVE::SchemaMismatch, std::nullopt,
                    "motion query live debug trace schema is unsupported"};
        }
        MotionQueryLiveDebugTraceEnvelopeUVE envelope;
        envelope.truncated = document.at("truncated").get<bool>();
        envelope.filter = document.at("filter").get<std::string>();
        const JsonUVE& events = document.at("events");
        if (envelope.filter.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
            !events.is_array() || events.size() > kMotionQueryMaximumTraceEventsUVE) {
            return {MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace, std::nullopt,
                    "motion query live debug trace envelope bounds are invalid"};
        }
        envelope.events.reserve(events.size());
        std::optional<MotionQueryTraceEventUVE> previous;
        for (const JsonUVE& value : events) {
            if (!value.is_object() ||
                !HasOnlyKeysUVE(value, {"sequence", "frameNumber", "kind", "candidatesConsidered",
                                        "candidatesEvaluated", "cost", "selectedCandidateIndex",
                                        "qualityTier", "continuityCode", "continuityApplied",
                                        "transitionCode", "transitionHeldPrevious", "telemetryCode",
                                        "telemetryIndexEntryCount", "telemetryCandidatesConsidered",
                                        "telemetryBudgetSaturated", "provenance", "message", "comment",
                                        "category", "pinned"})) {
                return {MotionQueryLiveDebugTracePersistenceCodeUVE::UnexpectedField, std::nullopt,
                        "motion query live debug trace event contains an unexpected field"};
            }
            MotionQueryTraceEventUVE event = EventFromJsonUVE(value);
            if (!IsValidPersistedEventUVE(event, previous)) {
                return {MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace, std::nullopt,
                        "motion query live debug trace event payload is invalid"};
            }
            envelope.events.push_back(std::move(event));
            previous = envelope.events.back();
        }
        return {MotionQueryLiveDebugTracePersistenceCodeUVE::Accepted, std::move(envelope),
                "motion query live debug trace deserialized"};
    } catch (const nlohmann::json::exception&) {
        return {MotionQueryLiveDebugTracePersistenceCodeUVE::ParseError, std::nullopt,
                "motion query live debug trace payload could not be parsed"};
    }
}

MotionQueryLiveDebugTraceAnalysisResultUVE AnalyzeMotionQueryLiveDebugTraceUVE(
    const MotionQueryTraceSnapshotUVE& snapshot) {
    if (snapshot.events.size() > kMotionQueryMaximumTraceEventsUVE) {
        return {MotionQueryLiveDebugTraceAnalysisCodeUVE::CapacityExceeded, std::nullopt,
                "motion query live debug trace exceeds the bounded analysis event limit"};
    }

    MotionQueryLiveDebugTraceAnalysisUVE analysis;
    analysis.eventCount = snapshot.events.size();
    std::map<std::string, std::size_t> kindCounts;
    std::optional<MotionQueryTraceEventUVE> previous;
    for (const MotionQueryTraceEventUVE& event : snapshot.events) {
        if (!IsValidPersistedEventUVE(event, previous)) {
            return {MotionQueryLiveDebugTraceAnalysisCodeUVE::InvalidTrace, std::nullopt,
                    "motion query live debug trace contains an invalid event"};
        }
        analysis.totalCost += static_cast<double>(event.cost);
        analysis.maximumCandidatesEvaluated =
            std::max(analysis.maximumCandidatesEvaluated, event.candidatesEvaluated);
        ++kindCounts[event.kind];
        previous = event;
    }
    analysis.kindCounts.reserve(kindCounts.size());
    for (const auto& [kind, count] : kindCounts) {
        analysis.kindCounts.push_back({kind, count});
    }
    return {MotionQueryLiveDebugTraceAnalysisCodeUVE::Accepted, std::move(analysis),
            "motion query live debug trace analyzed"};
}

} // namespace UVE::Plugins::Editor
