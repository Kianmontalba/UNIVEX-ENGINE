// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {

MotionQueryTraceSnapshotUVE MakeSnapshotUVE() {
    MotionQueryTraceSnapshotUVE snapshot;
    MotionQueryTraceEventUVE event;
    event.sequence = 1U;
    event.timestampNanoseconds = 9000000000U;
    event.frameNumber = 42U;
    event.kind = "accepted";
    event.database = UVE::Asset::ResourceHandleUVE{UVE::Asset::AssetGuidUVE{77U}, 3U};
    event.candidatesConsidered = 8U;
    event.candidatesEvaluated = 4U;
    event.cost = 0.375F;
    event.selectedCandidateIndex = 2U;
    event.qualityTier = 1U;
    event.continuityCode = 1U;
    event.continuityApplied = true;
    event.transitionCode = 2U;
    event.transitionHeldPrevious = false;
    event.telemetryCode = 0U;
    event.telemetryIndexEntryCount = 64U;
    event.telemetryCandidatesConsidered = 4U;
    event.telemetryBudgetSaturated = true;
    event.provenance = "continuity_applied";
    event.message = "machine-specific diagnostic text";
    snapshot.events.push_back(event);
    return snapshot;
}

MotionQueryTraceReplayFixtureUVE MakeFixtureUVE() {
    return BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());
}

MotionQueryTraceReplayCompatibilityUVE MakeCompatibilityUVE() {
    return MotionQueryTraceReplayCompatibilityUVE{3U, 4U, 5U, 9U};
}

} // namespace

TEST(MotionQueryTraceReplayUVETest, BuildUVE_CopiesDeterministicFieldsAndRedactsRuntimeIdentity) {
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE();

    ASSERT_EQ(fixture.schemaVersion, kMotionQueryTraceReplayFixtureSchemaVersionUVE);
    ASSERT_FALSE(fixture.truncated);
    ASSERT_EQ(fixture.events.size(), 1U);
    const MotionQueryTraceReplayEventUVE& event = fixture.events.front();
    EXPECT_EQ(event.sequence, 1U);
    EXPECT_EQ(event.frameNumber, 42U);
    EXPECT_EQ(event.kind, "accepted");
    EXPECT_EQ(event.candidatesConsidered, 8U);
    EXPECT_EQ(event.candidatesEvaluated, 4U);
    EXPECT_FLOAT_EQ(event.cost, 0.375F);
    EXPECT_EQ(event.selectedCandidateIndex, std::optional<std::size_t>{2U});
    EXPECT_EQ(event.qualityTier, 1U);
    EXPECT_EQ(event.continuityCode, 1U);
    EXPECT_TRUE(event.continuityApplied);
    EXPECT_EQ(event.transitionCode, 2U);
    EXPECT_EQ(event.telemetryCode, 0U);
    EXPECT_EQ(event.telemetryIndexEntryCount, 64U);
    EXPECT_EQ(event.telemetryCandidatesConsidered, 4U);
    EXPECT_TRUE(event.telemetryBudgetSaturated);
    EXPECT_EQ(event.provenance, "continuity_applied");
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_MatchesIdenticalTraceAndIgnoresRedactedFields) {
    const MotionQueryTraceSnapshotUVE original = MakeSnapshotUVE();
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(original);
    MotionQueryTraceSnapshotUVE replay = original;
    replay.events.front().timestampNanoseconds = 1U;
    replay.events.front().database.reset();
    replay.events.front().message = "different local diagnostic text";

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, replay);
    EXPECT_TRUE(comparison.IsMatchUVE());
    EXPECT_EQ(comparison.code, MotionQueryTraceReplayComparisonCodeUVE::Match);
    EXPECT_EQ(comparison.comparedEventCount, 1U);
    EXPECT_EQ(comparison.mismatchIndex, kMotionQueryTraceReplayNoMismatchIndexUVE);
    EXPECT_FALSE(comparison.IsTruncatedUVE());
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_ReportsFirstDeterministicEventMismatch) {
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE();
    MotionQueryTraceSnapshotUVE changed = MakeSnapshotUVE();
    changed.events.front().kind = "different_kind";
    changed.events.front().cost = 2.0F;
    changed.events.front().telemetryCandidatesConsidered = 3U;

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, changed);
    EXPECT_EQ(comparison.code, MotionQueryTraceReplayComparisonCodeUVE::EventMismatch);
    EXPECT_FALSE(comparison.IsMatchUVE());
    EXPECT_EQ(comparison.comparedEventCount, 0U);
    EXPECT_EQ(comparison.mismatchIndex, 0U);
    EXPECT_FALSE(comparison.IsTruncatedUVE());
    const std::uint32_t expectedMask =
        static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::Kind) |
        static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::Cost) |
        static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::TelemetryCandidatesConsidered);
    EXPECT_EQ(comparison.mismatchFieldMask, expectedMask);
    EXPECT_EQ(comparison.diagnosticSummary, "kind,cost,telemetryCandidatesConsidered");
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_RetainsBoundedTraceAndReportsTruncationFacts) {
    MotionQueryTraceLoggerUVE logger;
    for (std::size_t index = 0U; index < kMotionQueryMaximumTraceEventsUVE + 2U; ++index) {
        MotionQueryTraceEventUVE event;
        event.timestampNanoseconds = 100U + index;
        event.frameNumber = 10U + index;
        event.kind = "tick";
        ASSERT_TRUE(logger.RecordUVE(event).IsAcceptedUVE());
    }

    const MotionQueryTraceSnapshotUVE snapshot = logger.GetSnapshotUVE();
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(snapshot);
    ASSERT_EQ(fixture.events.size(), kMotionQueryMaximumTraceReplayEventsUVE);
    ASSERT_TRUE(fixture.truncated);
    const MotionQueryTraceReplayComparisonUVE match =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, snapshot);
    EXPECT_TRUE(match.IsMatchUVE());
    EXPECT_TRUE(match.IsTruncatedUVE());
    EXPECT_EQ(match.comparedEventCount, kMotionQueryMaximumTraceReplayEventsUVE);

    MotionQueryTraceSnapshotUVE unmarked = snapshot;
    unmarked.truncated = false;
    const MotionQueryTraceReplayComparisonUVE mismatch =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, unmarked);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplayComparisonCodeUVE::TruncationMismatch);
    EXPECT_TRUE(mismatch.IsTruncatedUVE());
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_RejectsUnsupportedSchemaAndInvalidEventPayload) {
    MotionQueryTraceReplayFixtureUVE unsupported = MakeFixtureUVE();
    unsupported.schemaVersion = kMotionQueryTraceReplayFixtureSchemaVersionUVE + 1U;
    EXPECT_EQ(CompareMotionQueryTraceReplayFixtureUVE(unsupported, MakeSnapshotUVE()).code,
              MotionQueryTraceReplayComparisonCodeUVE::SchemaMismatch);

    MotionQueryTraceReplayFixtureUVE invalid = MakeFixtureUVE();
    invalid.events.front().sequence = 0U;
    EXPECT_EQ(CompareMotionQueryTraceReplayFixtureUVE(invalid, MakeSnapshotUVE()).code,
              MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_RoundTripsCanonicalValueOnlyPayload) {
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE();
    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(fixture);
    ASSERT_TRUE(encoded.IsAcceptedUVE());
    EXPECT_EQ(encoded.code, MotionQueryTraceReplaySerializationCodeUVE::Accepted);
    EXPECT_EQ(encoded.payload.find("timestampNanoseconds"), std::string::npos);
    EXPECT_EQ(encoded.payload.find("database"), std::string::npos);
    EXPECT_EQ(encoded.payload.find("machine-specific diagnostic text"), std::string::npos);

    const MotionQueryTraceReplayDeserializationResultUVE decoded =
        DeserializeMotionQueryTraceReplayFixtureUVE(encoded.payload);
    ASSERT_TRUE(decoded.IsAcceptedUVE());
    ASSERT_TRUE(decoded.fixture.has_value());
    EXPECT_EQ(*decoded.fixture, fixture);

    const MotionQueryTraceReplaySerializationResultUVE reencoded =
        SerializeMotionQueryTraceReplayFixtureUVE(*decoded.fixture);
    ASSERT_TRUE(reencoded.IsAcceptedUVE());
    EXPECT_EQ(reencoded.payload, encoded.payload);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_RejectsEmptyMalformedSchemaAndInvalidPayloads) {
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE("").code,
              MotionQueryTraceReplaySerializationCodeUVE::EmptyPayload);
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE("not-json").code,
              MotionQueryTraceReplaySerializationCodeUVE::ParseError);

    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(MakeFixtureUVE());
    ASSERT_TRUE(encoded.IsAcceptedUVE());
    const std::string unsupported =
        "{\"schemaVersion\":2,\"truncated\":false,\"events\":[]}";
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE(unsupported).code,
              MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch);

    MotionQueryTraceReplayFixtureUVE unsupportedFixture = MakeFixtureUVE();
    unsupportedFixture.schemaVersion = kMotionQueryTraceReplayFixtureSchemaVersionUVE + 1U;
    EXPECT_EQ(SerializeMotionQueryTraceReplayFixtureUVE(unsupportedFixture).code,
              MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch);

    MotionQueryTraceReplayFixtureUVE invalid = MakeFixtureUVE();
    invalid.events.front().candidatesEvaluated = invalid.events.front().candidatesConsidered + 1U;
    EXPECT_EQ(SerializeMotionQueryTraceReplayFixtureUVE(invalid).code,
              MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture);
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_EnforcesRuntimeCompatibilityIdentity) {
    const MotionQueryTraceReplayCompatibilityUVE compatibility = MakeCompatibilityUVE();
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE(), compatibility);

    EXPECT_TRUE(CompareMotionQueryTraceReplayFixtureUVE(fixture, MakeSnapshotUVE(), compatibility)
                    .IsMatchUVE());
    const MotionQueryTraceReplayCompatibilityUVE changed{3U, 4U, 6U, 9U};
    const MotionQueryTraceReplayComparisonUVE mismatch =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, MakeSnapshotUVE(), changed);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplayComparisonCodeUVE::CompatibilityMismatch);
    EXPECT_EQ(mismatch.compatibilityMismatchMask,
              static_cast<std::uint32_t>(MotionQueryTraceReplayCompatibilityMismatchFieldUVE::NormalizationVersion));
    EXPECT_EQ(mismatch.compatibilityDiagnosticSummary, "normalizationVersion");
    EXPECT_FALSE(mismatch.IsMatchUVE());
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_RoundTripsRuntimeCompatibilityIdentity) {
    MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE(), MakeCompatibilityUVE());
    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(fixture);
    ASSERT_TRUE(encoded.IsAcceptedUVE());
    EXPECT_NE(encoded.payload.find("sourceGeneration"), std::string::npos);

    const MotionQueryTraceReplayDeserializationResultUVE decoded =
        DeserializeMotionQueryTraceReplayFixtureUVE(encoded.payload);
    ASSERT_TRUE(decoded.IsAcceptedUVE());
    ASSERT_TRUE(decoded.fixture.has_value());
    ASSERT_TRUE(decoded.fixture->compatibility.has_value());
    EXPECT_EQ(*decoded.fixture->compatibility, MakeCompatibilityUVE());
    EXPECT_EQ(*decoded.fixture, fixture);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_RejectsInvalidRuntimeCompatibilityIdentity) {
    MotionQueryTraceReplayFixtureUVE invalid = MakeFixtureUVE();
    invalid.compatibility = MotionQueryTraceReplayCompatibilityUVE{0U, 4U, 5U, 9U};
    EXPECT_EQ(SerializeMotionQueryTraceReplayFixtureUVE(invalid).code,
              MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_RejectsForbiddenAndUnexpectedFields) {
    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(MakeFixtureUVE());
    ASSERT_TRUE(encoded.IsAcceptedUVE());
    std::string forbidden = encoded.payload;
    const std::string eventPrefix = "{\"candidatesConsidered\":8";
    const std::size_t eventPosition = forbidden.find(eventPrefix);
    ASSERT_NE(eventPosition, std::string::npos);
    forbidden.insert(eventPosition + eventPrefix.size(), ",\"message\":\"raw\"");
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE(forbidden).code,
              MotionQueryTraceReplaySerializationCodeUVE::ForbiddenField);

    std::string unexpected = encoded.payload;
    const std::size_t schemaPosition = unexpected.find("\"schemaVersion\":1");
    ASSERT_NE(schemaPosition, std::string::npos);
    unexpected.insert(schemaPosition + std::string("\"schemaVersion\":1").size(),
                      ",\"futureField\":1");
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE(unexpected).code,
              MotionQueryTraceReplaySerializationCodeUVE::UnexpectedField);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_AcceptsLegacyPayloadWithoutCompatibilityKey) {
    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(MakeFixtureUVE());
    ASSERT_TRUE(encoded.IsAcceptedUVE());
    const std::string compatibilityField = "\"compatibility\":null,";
    const std::size_t fieldPosition = encoded.payload.find(compatibilityField);
    ASSERT_NE(fieldPosition, std::string::npos);

    std::string legacy = encoded.payload;
    legacy.erase(fieldPosition, compatibilityField.size());
    const MotionQueryTraceReplayDeserializationResultUVE decoded =
        DeserializeMotionQueryTraceReplayFixtureUVE(legacy);
    ASSERT_TRUE(decoded.IsAcceptedUVE());
    ASSERT_TRUE(decoded.fixture.has_value());
    EXPECT_FALSE(decoded.fixture->compatibility.has_value());
    EXPECT_EQ(decoded.fixture->events, MakeFixtureUVE().events);
}

TEST(MotionQueryTraceReplayUVETest, SerializationUVE_EnforcesBoundedPayloadSize) {
    MotionQueryTraceReplayFixtureUVE oversized = MakeFixtureUVE();
    oversized.events.front().provenance.assign(kMotionQueryMaximumDebugMessageBytesUVE, 'x');
    EXPECT_TRUE(SerializeMotionQueryTraceReplayFixtureUVE(oversized).IsAcceptedUVE());

    std::string payload(kMotionQueryMaximumTraceReplayPayloadBytesUVE + 1U, 'x');
    EXPECT_EQ(DeserializeMotionQueryTraceReplayFixtureUVE(payload).code,
              MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge);
}

} // namespace UVE::Plugins::Editor
