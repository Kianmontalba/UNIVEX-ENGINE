// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_baseline_registry_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Plugins::Editor {
namespace {
using JsonUVE = nlohmann::json;

[[nodiscard]] bool HasOnlyKeysUVE(const JsonUVE& object,
                                  const std::initializer_list<std::string_view> allowedKeys) {
    if (!object.is_object()) {
        return false;
    }
    for (const auto& item : object.items()) {
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
} // namespace


MotionQueryTraceReplayBaselineResultUVE MotionQueryTraceReplayBaselineRegistryUVE::RegisterUVE(
    const std::string_view name, const MotionQueryTraceReplayFixtureUVE& fixture) {
    if (!IsValidNameUVE(name)) {
        return {MotionQueryTraceReplayBaselineCodeUVE::InvalidName, 0U, generation_,
                "replay baseline name is empty, overlong, or contains a forbidden path/control character"};
    }
    const MotionQueryTraceReplaySerializationResultUVE validation =
        SerializeMotionQueryTraceReplayFixtureUVE(fixture);
    if (!validation.IsAcceptedUVE()) {
        return {MotionQueryTraceReplayBaselineCodeUVE::InvalidFixture, 0U, generation_,
                "replay baseline fixture rejected by the canonical fixture codec: " + validation.message};
    }

    const auto existing = std::lower_bound(
        baselines_.begin(), baselines_.end(), name,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    const std::size_t index = static_cast<std::size_t>(existing - baselines_.begin());
    if (existing != baselines_.end() && existing->name == name) {
        existing->fixture = fixture;
        IncrementGenerationUVE();
        return {MotionQueryTraceReplayBaselineCodeUVE::DuplicateReplacement, index, generation_,
                "replay baseline replaced in place without changing deterministic ordering"};
    }
    if (baselines_.size() >= kMotionQueryMaximumReplayBaselinesUVE) {
        return {MotionQueryTraceReplayBaselineCodeUVE::CapacityExceeded, index, generation_,
                "replay baseline registry reached its bounded capacity"};
    }

    baselines_.insert(existing, StoredBaselineUVE{std::string{name}, fixture});
    IncrementGenerationUVE();
    return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, index, generation_,
            "replay baseline registered"};
}

MotionQueryTraceReplayBaselineResultUVE MotionQueryTraceReplayBaselineRegistryUVE::RemoveUVE(
    const std::string_view name) {
    const auto existing = std::lower_bound(
        baselines_.begin(), baselines_.end(), name,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    if (existing == baselines_.end() || existing->name != name) {
        return {MotionQueryTraceReplayBaselineCodeUVE::NotFound, 0U, generation_,
                "replay baseline name was not found"};
    }
    const std::size_t index = static_cast<std::size_t>(existing - baselines_.begin());
    baselines_.erase(existing);
    IncrementGenerationUVE();
    return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, index, generation_,
            "replay baseline removed"};
}

MotionQueryTraceReplayBaselineResultUVE MotionQueryTraceReplayBaselineRegistryUVE::RenameUVE(
    const std::string_view oldName, const std::string_view newName) {
    if (!IsValidNameUVE(newName)) {
        return {MotionQueryTraceReplayBaselineCodeUVE::InvalidName, 0U, generation_,
                "new replay baseline name is empty, overlong, or contains a forbidden character"};
    }
    if (oldName == newName) {
        return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, 0U, generation_,
                "replay baseline name is already set to the requested value"};
    }

    const auto oldEntry = std::lower_bound(
        baselines_.begin(), baselines_.end(), oldName,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    if (oldEntry == baselines_.end() || oldEntry->name != oldName) {
        return {MotionQueryTraceReplayBaselineCodeUVE::NotFound, 0U, generation_,
                "source replay baseline name was not found"};
    }

    const auto newEntry = std::lower_bound(
        baselines_.begin(), baselines_.end(), newName,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    if (newEntry != baselines_.end() && newEntry->name == newName) {
        return {MotionQueryTraceReplayBaselineCodeUVE::DuplicateReplacement, 0U, generation_,
                "target replay baseline name already exists"};
    }

    MotionQueryTraceReplayFixtureUVE fixture = std::move(oldEntry->fixture);
    baselines_.erase(oldEntry);
    
    // Re-find the insertion point since erase might have invalidated iterators
    const auto insertionPoint = std::lower_bound(
        baselines_.begin(), baselines_.end(), newName,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    const std::size_t index = static_cast<std::size_t>(insertionPoint - baselines_.begin());
    baselines_.insert(insertionPoint, StoredBaselineUVE{std::string{newName}, std::move(fixture)});
    
    IncrementGenerationUVE();
    return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, index, generation_,
            "replay baseline renamed"};
}

MotionQueryTraceReplayBaselineResultUVE MotionQueryTraceReplayBaselineRegistryUVE::ClearUVE() noexcept {
    if (baselines_.empty()) {
        return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, 0U, generation_,
                "replay baseline registry was already empty"};
    }
    baselines_.clear();
    IncrementGenerationUVE();
    return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, 0U, generation_,
            "replay baseline registry cleared"};
}

MotionQueryTraceReplayBaselineSelectionUVE MotionQueryTraceReplayBaselineRegistryUVE::SelectUVE(
    const std::string_view name, const std::optional<std::uint64_t> expectedRegistryGeneration) const {
    if (expectedRegistryGeneration.has_value() && expectedRegistryGeneration.value() != generation_) {
        return {MotionQueryTraceReplayBaselineCodeUVE::StaleGeneration, generation_, std::nullopt,
                "replay baseline selection used a stale registry generation"};
    }
    const auto existing = std::lower_bound(
        baselines_.begin(), baselines_.end(), name,
        [](const StoredBaselineUVE& entry, const std::string_view candidate) {
            return entry.name < candidate;
        });
    if (existing == baselines_.end() || existing->name != name) {
        return {MotionQueryTraceReplayBaselineCodeUVE::NotFound, generation_, std::nullopt,
                "replay baseline name was not found"};
    }
    return {MotionQueryTraceReplayBaselineCodeUVE::Accepted, generation_, existing->fixture,
            "replay baseline selected as a copied fixture"};
}

MotionQueryTraceReplayBaselineSnapshotUVE MotionQueryTraceReplayBaselineRegistryUVE::GetSnapshotUVE() const {
    MotionQueryTraceReplayBaselineSnapshotUVE snapshot;
    snapshot.generation = generation_;
    snapshot.entries.reserve(baselines_.size());
    for (const StoredBaselineUVE& baseline : baselines_) {
        snapshot.entries.push_back(MotionQueryTraceReplayBaselineEntryUVE{
            baseline.name,
            SourceGenerationUVE(baseline.fixture),
            baseline.fixture.events.size(),
            baseline.fixture.truncated});
    }
    return snapshot;
}

MotionQueryTraceReplayBaselineEnvelopeSerializationResultUVE
MotionQueryTraceReplayBaselineRegistryUVE::SerializeEnvelopeUVE() const {
    JsonUVE baselines = JsonUVE::array();
    for (const StoredBaselineUVE& baseline : baselines_) {
        const MotionQueryTraceReplaySerializationResultUVE serializedFixture =
            SerializeMotionQueryTraceReplayFixtureUVE(baseline.fixture);
        if (!serializedFixture.IsAcceptedUVE()) {
            return {serializedFixture.code, {},
                    "replay baseline envelope export rejected a fixture: " + serializedFixture.message};
        }
        try {
            baselines.push_back(JsonUVE{{"name", baseline.name},
                                       {"fixture", JsonUVE::parse(serializedFixture.payload)}});
        } catch (const JsonUVE::exception& exception) {
            return {MotionQueryTraceReplaySerializationCodeUVE::ParseError, {},
                    "replay baseline envelope export could not embed a canonical fixture: " +
                        std::string(exception.what())};
        }
    }
    const JsonUVE document{{"schemaVersion", kMotionQueryReplayBaselineEnvelopeSchemaVersionUVE},
                           {"baselines", std::move(baselines)}};
    const std::string payload = document.dump();
    if (payload.empty()) {
        return {MotionQueryTraceReplaySerializationCodeUVE::EmptyPayload, {},
                "replay baseline envelope export produced an empty payload"};
    }
    if (payload.size() > kMotionQueryMaximumReplayBaselineEnvelopeBytesUVE) {
        return {MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge, {},
                "replay baseline envelope exceeded its bounded payload size"};
    }
    return {MotionQueryTraceReplaySerializationCodeUVE::Accepted, payload,
            "replay baseline envelope exported"};
}

MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE
MotionQueryTraceReplayBaselineRegistryUVE::DeserializeEnvelopeUVE(const std::string_view payload) {
    if (payload.empty()) {
        return {MotionQueryTraceReplaySerializationCodeUVE::EmptyPayload, 0U,
                "replay baseline envelope payload is empty"};
    }
    if (payload.size() > kMotionQueryMaximumReplayBaselineEnvelopeBytesUVE) {
        return {MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge, 0U,
                "replay baseline envelope exceeded its bounded payload size"};
    }

    JsonUVE document;
    try {
        document = JsonUVE::parse(payload.begin(), payload.end());
    } catch (const JsonUVE::exception& exception) {
        return {MotionQueryTraceReplaySerializationCodeUVE::ParseError, 0U,
                "replay baseline envelope JSON could not be parsed: " + std::string(exception.what())};
    }
    if (!HasOnlyKeysUVE(document, {"schemaVersion", "baselines"}) ||
        !document.contains("schemaVersion") || !document.contains("baselines") ||
        !document.at("schemaVersion").is_number_unsigned() ||
        document.at("schemaVersion").get<std::uint32_t>() != kMotionQueryReplayBaselineEnvelopeSchemaVersionUVE) {
        return {MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch, 0U,
                "replay baseline envelope schema is unsupported or malformed"};
    }
    const JsonUVE& serializedBaselines = document.at("baselines");
    if (!serializedBaselines.is_array() || serializedBaselines.size() > kMotionQueryMaximumReplayBaselinesUVE) {
        return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, 0U,
                "replay baseline envelope entries exceed the bounded registry capacity"};
    }

    MotionQueryTraceReplayBaselineRegistryUVE imported;
    std::string previousName;
    for (const JsonUVE& entry : serializedBaselines) {
        if (!HasOnlyKeysUVE(entry, {"name", "fixture"}) || !entry.contains("name") ||
            !entry.contains("fixture") || !entry.at("name").is_string()) {
            return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, 0U,
                    "replay baseline envelope entry is malformed"};
        }
        const std::string name = entry.at("name").get<std::string>();
        if (!IsValidNameUVE(name) || (!previousName.empty() && previousName >= name)) {
            return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, 0U,
                    "replay baseline envelope names must be valid, unique, and strictly sorted"};
        }
        const MotionQueryTraceReplayDeserializationResultUVE fixture =
            DeserializeMotionQueryTraceReplayFixtureUVE(entry.at("fixture").dump());
        if (!fixture.IsAcceptedUVE()) {
            return {fixture.code, 0U,
                    "replay baseline envelope fixture rejected by the canonical codec: " + fixture.message};
        }
        const MotionQueryTraceReplayBaselineResultUVE registered =
            imported.RegisterUVE(name, *fixture.fixture);
        if (!registered.IsAcceptedUVE()) {
            return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, 0U,
                    "replay baseline envelope could not register an imported fixture: " + registered.message};
        }
        previousName = name;
    }

    baselines_ = std::move(imported.baselines_);
    IncrementGenerationUVE();
    return {MotionQueryTraceReplaySerializationCodeUVE::Accepted, serializedBaselines.size(),
            "replay baseline envelope imported atomically"};
}

bool MotionQueryTraceReplayBaselineRegistryUVE::IsValidNameUVE(const std::string_view name) noexcept {
    if (name.empty() || name.size() > kMotionQueryMaximumReplayBaselineNameBytesUVE) {
        return false;
    }
    for (const char rawCharacter : name) {
        const unsigned char character = static_cast<unsigned char>(rawCharacter);
        if (character < 0x20U || character == 0x7FU || character == static_cast<unsigned char>('/') ||
            character == static_cast<unsigned char>('\\')) {
            return false;
        }
    }
    return true;
}

std::uint64_t MotionQueryTraceReplayBaselineRegistryUVE::SourceGenerationUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture) noexcept {
    return fixture.compatibility.has_value() ? fixture.compatibility->sourceGeneration : 0U;
}

void MotionQueryTraceReplayBaselineRegistryUVE::IncrementGenerationUVE() noexcept {
    if (generation_ < std::numeric_limits<std::uint64_t>::max()) {
        ++generation_;
    }
}

} // namespace UVE::Plugins::Editor
