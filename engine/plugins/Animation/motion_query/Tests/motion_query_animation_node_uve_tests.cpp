// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_animation_node_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Plugins {
namespace {
UVE::Core::MotionQueryFeatureSchemaUVE MakeSchemaUVE() {
    UVE::Core::MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{"velocity", UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity, 0U, 1.0F},
        UVE::Core::MotionQueryFeatureChannelUVE{"facing", UVE::Core::MotionQueryFeatureChannelKindUVE::FacingDirection, 0U, 1.0F},
    };
    return schema;
}

UVE::Core::MotionMatchingCandidateUVE MakeCandidateUVE(const char* id, float velocityX,
                                                        double sampleTime) {
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = id;
    candidate.sourceClipId = "walk";
    candidate.sampleTimeSeconds = sampleTime;
    candidate.feature.rootVelocity = UVE::Math::Vector3UVE{velocityX, 0.0F, 0.0F};
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return candidate;
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    database.candidates = {
        MakeCandidateUVE("far", 5.0F, 0.25),
        MakeCandidateUVE("near", 1.0F, 0.75),
    };
    return database;
}

std::vector<UVE::Core::AnimationClipUVE> MakeClipsUVE() {
    UVE::Core::AnimationClipUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {
        UVE::Core::PoseSampleUVE{0.0, UVE::Core::TransformPoseUVE{UVE::Math::Vector3UVE{0.0F, 0.0F, 0.0F}, UVE::Math::QuaternionUVE{}, UVE::Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
        UVE::Core::PoseSampleUVE{1.0, UVE::Core::TransformPoseUVE{UVE::Math::Vector3UVE{2.0F, 0.0F, 0.0F}, UVE::Math::QuaternionUVE{}, UVE::Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
    };
    clip.samples[0].pose.rotation.w = 1.0F;
    clip.samples[1].pose.rotation.w = 1.0F;
    return {clip};
}
class CapturingDebugSinkUVE final : public IMotionQueryAnimationDebugSinkUVE {
public:
    void PublishUVE(const MotionQueryAnimationNodeResultUVE& result,
                   const std::uint64_t timestampNanoseconds,
                   const std::uint64_t frameNumber) noexcept override {
        last = result;
        timestamp = timestampNanoseconds;
        frame = frameNumber;
        ++count;
    }

    MotionQueryAnimationNodeResultUVE last;
    std::uint64_t timestamp = 0U;
    std::uint64_t frame = 0U;
    std::size_t count = 0U;
};
} // namespace

TEST(MotionQueryAnimationNodeUVETest, EvaluateUVE_PublishesCopiedResultToOptionalDebugSink) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    CapturingDebugSinkUVE sink;
    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        query, database, schema, index, MakeClipsUVE(), MotionQueryAnimationNodeSettingsUVE{},
        &sink, 123U, 9U);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(sink.count, 1U);
    EXPECT_EQ(sink.last.candidateIndex, result.candidateIndex);
    EXPECT_EQ(sink.last.sourceClipId, result.sourceClipId);
    EXPECT_EQ(sink.timestamp, 123U);
    EXPECT_EQ(sink.frame, 9U);
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateUVE_PrefiltersThenUsesCoreMatchAndSamplesClip) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());

    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    MotionQueryAnimationNodeSettingsUVE settings;
    settings.maximumSearchResults = 1U;
    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        query, database, schema, index, MakeClipsUVE(), settings);

    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    EXPECT_EQ(result.candidateIndex, 1U);
    EXPECT_EQ(result.sourceClipId, "walk");
    EXPECT_DOUBLE_EQ(result.sampleTimeSeconds, 0.75);
    EXPECT_EQ(result.candidatesEvaluated, 1U);
    EXPECT_NEAR(result.pose.position.x, 1.5F, 1.0e-5F);
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateUVE_ReportsMissingClipWithoutLosingMatchMetadata) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};

    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        query, database, schema, index, {}, MotionQueryAnimationNodeSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryAnimationNodeCodeUVE::MissingClip);
    EXPECT_EQ(result.candidateIndex, 1U);
    EXPECT_EQ(result.sourceClipId, "walk");
    EXPECT_DOUBLE_EQ(result.sampleTimeSeconds, 0.75);
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateFromHistoryUVE_UsesAtOrBeforeFrameAndRejectsMissingFrame) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    UVE::Core::MotionQueryHistoryBufferUVE history;
    UVE::Core::MotionQueryHistoryFrameUVE frame;
    frame.sample.timeSeconds = 1.0;
    frame.sample.pose.rotation.w = 1.0F;
    frame.query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    frame.query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(history.AppendFrameUVE(frame).IsAcceptedUVE());

    const MotionQueryAnimationNodeResultUVE accepted =
        EvaluateMotionQueryAnimationNodeFromHistoryUVE(history, 1.5, database, schema, index,
                                                       MakeClipsUVE(),
                                                       MotionQueryAnimationNodeSettingsUVE{});
    ASSERT_TRUE(accepted.IsAcceptedUVE()) << accepted.message;
    EXPECT_EQ(accepted.candidateIndex, 1U);
    EXPECT_EQ(EvaluateMotionQueryAnimationNodeFromHistoryUVE(
                  history, 0.5, database, schema, index, MakeClipsUVE(),
                  MotionQueryAnimationNodeSettingsUVE{})
                  .code,
              MotionQueryAnimationNodeCodeUVE::NoHistoryFrame);
    EXPECT_EQ(EvaluateMotionQueryAnimationNodeFromHistoryUVE(
                  history, std::numeric_limits<double>::quiet_NaN(), database, schema, index,
                  MakeClipsUVE(), MotionQueryAnimationNodeSettingsUVE{})
                  .code,
              MotionQueryAnimationNodeCodeUVE::InvalidEvaluationTime);
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateFromHistoryUVE_AppliesBoundedContinuityFromPreviousFrame) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    UVE::Core::MotionQueryHistoryBufferUVE history;
    UVE::Core::MotionQueryHistoryFrameUVE previous;
    previous.sample.timeSeconds = 0.5;
    previous.sample.pose.position = UVE::Math::Vector3UVE{0.0F, 0.0F, 0.0F};
    previous.sample.pose.rotation.w = 1.0F;
    previous.query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    previous.query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(history.AppendFrameUVE(previous).IsAcceptedUVE());
    UVE::Core::MotionQueryHistoryFrameUVE current = previous;
    current.sample.timeSeconds = 1.0;
    ASSERT_TRUE(history.AppendFrameUVE(current).IsAcceptedUVE());
    MotionQueryAnimationNodeSettingsUVE settings;
    settings.continuity.policy = MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow;
    settings.continuity.maximumPreviousAgeSeconds = 1.1;
    const MotionQueryAnimationNodeResultUVE result =
        EvaluateMotionQueryAnimationNodeFromHistoryUVE(history, 1.5, database, schema, index,
                                                       MakeClipsUVE(), settings);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    EXPECT_EQ(result.continuityCode, MotionQueryContinuityCodeUVE::Applied);
    EXPECT_TRUE(result.continuityApplied);
    EXPECT_DOUBLE_EQ(result.continuityPreviousAgeSeconds, 1.0);
    EXPECT_NEAR(result.pose.position.x, 0.75F, 1.0e-5F);
    EXPECT_EQ(result.pose.rotation, (UVE::Math::QuaternionUVE{}));
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateFromHistoryUVE_HoldsPreviousCandidateWithinWindow) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    UVE::Core::MotionQueryHistoryBufferUVE history;
    UVE::Core::MotionQueryHistoryFrameUVE previous;
    previous.sample.timeSeconds = 0.1;
    previous.sample.pose.rotation.w = 1.0F;
    previous.query.rootVelocity = UVE::Math::Vector3UVE{5.0F, 0.0F, 0.0F};
    previous.query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(history.AppendFrameUVE(previous).IsAcceptedUVE());
    UVE::Core::MotionQueryHistoryFrameUVE current = previous;
    current.sample.timeSeconds = 0.2;
    current.query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    ASSERT_TRUE(history.AppendFrameUVE(current).IsAcceptedUVE());
    MotionQueryAnimationNodeSettingsUVE settings;
    settings.transition.minimumHoldSeconds = 0.1;
    settings.transition.maximumHoldWindowSeconds = 0.5;
    const MotionQueryAnimationNodeResultUVE result =
        EvaluateMotionQueryAnimationNodeFromHistoryUVE(history, 0.3, database, schema, index,
                                                       MakeClipsUVE(), settings);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    EXPECT_EQ(result.candidateIndex, 0U);
    EXPECT_EQ(result.transitionCode, MotionQueryTransitionCodeUVE::HeldPreviousCandidate);
    EXPECT_TRUE(result.transitionHeldPrevious);
}

TEST(MotionQueryAnimationNodeUVETest, SettingsValidationUVE_AcceptsDefaultAndRejectsBoundedPolicyErrors) {
    const MotionQueryAnimationNodeSettingsUVE defaults;
    EXPECT_TRUE(ValidateMotionQueryAnimationNodeSettingsUVE(defaults).IsValidUVE());

    MotionQueryAnimationNodeSettingsUVE invalidResults = defaults;
    invalidResults.maximumSearchResults = 0U;
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidResults).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidSearchResults);

    MotionQueryAnimationNodeSettingsUVE invalidQuality = defaults;
    invalidQuality.qualityTier = static_cast<MotionQueryQualityTierUVE>(255U);
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidQuality).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidQualityTier);

    MotionQueryAnimationNodeSettingsUVE invalidWeights = defaults;
    invalidWeights.weights.velocityWeight = -1.0F;
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidWeights).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidWeights);

    invalidWeights = defaults;
    invalidWeights.weights.velocityWeight = std::numeric_limits<float>::max();
    invalidWeights.weights.facingWeight = std::numeric_limits<float>::max();
    invalidWeights.weights.trajectoryWeight = std::numeric_limits<float>::max();
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidWeights).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidWeights);

    MotionQueryAnimationNodeSettingsUVE invalidContinuity = defaults;
    invalidContinuity.continuity.policy = static_cast<MotionQueryContinuityPolicyUVE>(255U);
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidContinuity).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidContinuityPolicy);

    invalidContinuity = defaults;
    invalidContinuity.continuity.maximumPreviousAgeSeconds = -1.0;
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidContinuity).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidContinuityAge);

    MotionQueryAnimationNodeSettingsUVE invalidTransition = defaults;
    invalidTransition.transition.minimumHoldSeconds = 1.0;
    invalidTransition.transition.maximumHoldWindowSeconds = 0.5;
    EXPECT_EQ(ValidateMotionQueryAnimationNodeSettingsUVE(invalidTransition).code,
              MotionQueryAnimationSettingsValidationCodeUVE::InvalidTransitionSettings);
}

TEST(MotionQueryAnimationNodeUVETest, EvaluateUVE_RejectsInvalidSettingsAndUnbuiltIndex) {
    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};

    MotionQueryAnimationNodeSettingsUVE invalidSettings;
    invalidSettings.maximumSearchResults = 0U;
    EXPECT_EQ(EvaluateMotionQueryAnimationNodeUVE(query, database, schema, index, MakeClipsUVE(),
                                                  invalidSettings)
                  .code,
              MotionQueryAnimationNodeCodeUVE::InvalidSettings);
    EXPECT_EQ(EvaluateMotionQueryAnimationNodeUVE(query, database, schema, index, MakeClipsUVE(),
                                                  MotionQueryAnimationNodeSettingsUVE{})
                  .code,
              MotionQueryAnimationNodeCodeUVE::IndexNotBuilt);
}
} // namespace UVE::Plugins
