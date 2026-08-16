// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_baseline_registry_uve.h"

#include <gtest/gtest.h>

#include <string>

namespace UVE::Plugins::Editor {
namespace {

MotionQueryTraceReplayFixtureUVE MakeFixtureUVE(const std::uint64_t sourceGeneration = 0U) {
    MotionQueryTraceReplayFixtureUVE fixture;
    if (sourceGeneration != 0U) {
        fixture.compatibility = MotionQueryTraceReplayCompatibilityUVE{1U, 2U, 3U, sourceGeneration};
    }
    MotionQueryTraceReplayEventUVE event;
    event.sequence = 1U;
    event.frameNumber = 4U;
    event.kind = "accepted";
    event.candidatesConsidered = 2U;
    event.candidatesEvaluated = 1U;
    event.cost = 0.25F;
    event.selectedCandidateIndex = 0U;
    event.qualityTier = 1U;
    event.continuityCode = 1U;
    event.continuityApplied = true;
    event.transitionCode = 2U;
    event.telemetryCode = 0U;
    event.telemetryIndexEntryCount = 8U;
    event.telemetryCandidatesConsidered = 1U;
    event.provenance = "baseline";
    fixture.events.push_back(event);
    return fixture;
}

} // namespace

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, RegisterUVE_SortsEntriesAndSelectsCopiedFixture) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE(17U);

    EXPECT_TRUE(registry.RegisterUVE("zeta", fixture).IsAcceptedUVE());
    EXPECT_TRUE(registry.RegisterUVE("alpha", fixture).IsAcceptedUVE());
    const MotionQueryTraceReplayBaselineSnapshotUVE snapshot = registry.GetSnapshotUVE();
    ASSERT_EQ(snapshot.entries.size(), 2U);
    EXPECT_EQ(snapshot.entries[0].name, "alpha");
    EXPECT_EQ(snapshot.entries[1].name, "zeta");
    EXPECT_EQ(snapshot.entries[0].sourceGeneration, 17U);

    const MotionQueryTraceReplayBaselineSelectionUVE selected = registry.SelectUVE("alpha", snapshot.generation);
    ASSERT_TRUE(selected.IsAcceptedUVE());
    ASSERT_TRUE(selected.fixture.has_value());
    EXPECT_EQ(selected.fixture->compatibility->sourceGeneration, 17U);
    EXPECT_EQ(selected.fixture->events.size(), 1U);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, RegisterUVE_ReplacesInPlaceAndAdvancesGeneration) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    const MotionQueryTraceReplayFixtureUVE first = MakeFixtureUVE(1U);
    MotionQueryTraceReplayFixtureUVE replacement = MakeFixtureUVE(2U);
    replacement.events.front().cost = 0.5F;

    const MotionQueryTraceReplayBaselineResultUVE initial = registry.RegisterUVE("baseline", first);
    ASSERT_TRUE(initial.IsAcceptedUVE());
    const MotionQueryTraceReplayBaselineResultUVE updated = registry.RegisterUVE("baseline", replacement);
    EXPECT_EQ(updated.code, MotionQueryTraceReplayBaselineCodeUVE::DuplicateReplacement);
    EXPECT_GT(updated.registryGeneration, initial.registryGeneration);
    ASSERT_EQ(registry.GetSnapshotUVE().entries.size(), 1U);
    const MotionQueryTraceReplayBaselineSelectionUVE selected = registry.SelectUVE("baseline");
    ASSERT_TRUE(selected.IsAcceptedUVE());
    EXPECT_FLOAT_EQ(selected.fixture->events.front().cost, 0.5F);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, RegisterUVE_RejectsInvalidNameAndFixtureWithoutMutation) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    EXPECT_EQ(registry.RegisterUVE("", MakeFixtureUVE()).code,
              MotionQueryTraceReplayBaselineCodeUVE::InvalidName);
    EXPECT_EQ(registry.RegisterUVE("folder\\baseline", MakeFixtureUVE()).code,
              MotionQueryTraceReplayBaselineCodeUVE::InvalidName);
    MotionQueryTraceReplayFixtureUVE invalidFixture = MakeFixtureUVE();
    invalidFixture.schemaVersion = 999U;
    EXPECT_EQ(registry.RegisterUVE("invalid", invalidFixture).code,
              MotionQueryTraceReplayBaselineCodeUVE::InvalidFixture);
    EXPECT_EQ(registry.GetSnapshotUVE().generation, 0U);
    EXPECT_TRUE(registry.GetSnapshotUVE().entries.empty());
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, RegisterUVE_EnforcesBoundedCapacity) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE();
    for (std::size_t index = 0U; index < kMotionQueryMaximumReplayBaselinesUVE; ++index) {
        ASSERT_TRUE(registry.RegisterUVE("baseline-" + std::to_string(index), fixture).IsAcceptedUVE());
    }
    const MotionQueryTraceReplayBaselineResultUVE overflow = registry.RegisterUVE("overflow", fixture);
    EXPECT_EQ(overflow.code, MotionQueryTraceReplayBaselineCodeUVE::CapacityExceeded);
    EXPECT_EQ(registry.GetSnapshotUVE().entries.size(), kMotionQueryMaximumReplayBaselinesUVE);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, EnvelopeUVE_RoundTripsSortedCopiedFixtures) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("zeta", MakeFixtureUVE(17U)).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE("alpha", MakeFixtureUVE(19U)).IsAcceptedUVE());

    const MotionQueryTraceReplayBaselineEnvelopeSerializationResultUVE exported =
        registry.SerializeEnvelopeUVE();
    ASSERT_TRUE(exported.IsAcceptedUVE());
    EXPECT_NE(exported.payload.find("schemaVersion"), std::string::npos);

    MotionQueryTraceReplayBaselineRegistryUVE imported;
    const MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE restored =
        imported.DeserializeEnvelopeUVE(exported.payload);
    ASSERT_TRUE(restored.IsAcceptedUVE());
    EXPECT_EQ(restored.importedBaselineCount, 2U);
    const MotionQueryTraceReplayBaselineSnapshotUVE snapshot = imported.GetSnapshotUVE();
    ASSERT_EQ(snapshot.entries.size(), 2U);
    EXPECT_EQ(snapshot.entries[0].name, "alpha");
    EXPECT_EQ(snapshot.entries[1].name, "zeta");
    ASSERT_TRUE(imported.SelectUVE("zeta").IsAcceptedUVE());
    EXPECT_EQ(imported.SelectUVE("zeta").fixture->compatibility->sourceGeneration, 17U);
    ASSERT_TRUE(imported.SerializeEnvelopeUVE().IsAcceptedUVE());
    EXPECT_EQ(imported.SerializeEnvelopeUVE().payload, exported.payload);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, EnvelopeUVE_RejectsMalformedPayloadWithoutMutation) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("existing", MakeFixtureUVE()).IsAcceptedUVE());
    const MotionQueryTraceReplayBaselineSnapshotUVE before = registry.GetSnapshotUVE();

    const MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE rejected =
        registry.DeserializeEnvelopeUVE(R"({"schemaVersion":999,"baselines":[]})");
    EXPECT_EQ(rejected.code, MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch);
    EXPECT_EQ(registry.GetSnapshotUVE(), before);

    const MotionQueryTraceReplayBaselineEnvelopeDeserializationResultUVE oversized =
        registry.DeserializeEnvelopeUVE(std::string(kMotionQueryMaximumReplayBaselineEnvelopeBytesUVE + 1U, 'x'));
    EXPECT_EQ(oversized.code, MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge);
    EXPECT_EQ(registry.GetSnapshotUVE(), before);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, SelectUVE_RejectsStaleGenerationAndMissingNames) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("baseline", MakeFixtureUVE()).IsAcceptedUVE());
    const std::uint64_t generation = registry.GetSnapshotUVE().generation;
    ASSERT_TRUE(registry.RegisterUVE("other", MakeFixtureUVE()).IsAcceptedUVE());

    EXPECT_EQ(registry.SelectUVE("baseline", generation).code,
              MotionQueryTraceReplayBaselineCodeUVE::StaleGeneration);
    EXPECT_EQ(registry.SelectUVE("missing").code,
              MotionQueryTraceReplayBaselineCodeUVE::NotFound);
}

TEST(MotionQueryTraceReplayBaselineRegistryUVETest, RemoveUVE_UpdatesGenerationAndClearIsIdempotent) {
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("baseline", MakeFixtureUVE()).IsAcceptedUVE());
    const std::uint64_t registeredGeneration = registry.GetSnapshotUVE().generation;
    const MotionQueryTraceReplayBaselineResultUVE removed = registry.RemoveUVE("baseline");
    EXPECT_TRUE(removed.IsAcceptedUVE());
    EXPECT_GT(removed.registryGeneration, registeredGeneration);
    EXPECT_EQ(registry.RemoveUVE("baseline").code, MotionQueryTraceReplayBaselineCodeUVE::NotFound);
    const std::uint64_t removedGeneration = registry.GetSnapshotUVE().generation;
    EXPECT_TRUE(registry.ClearUVE().IsAcceptedUVE());
    EXPECT_EQ(registry.GetSnapshotUVE().generation, removedGeneration);
}

} // namespace UVE::Plugins::Editor
