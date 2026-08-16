// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_baseline_registry_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Plugins::Editor {

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
