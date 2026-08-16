// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_animation_node_uve.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryAnimationNodeResultUVE MakeResultUVE(
    MotionQueryAnimationNodeCodeUVE code, const char* message) noexcept {
    MotionQueryAnimationNodeResultUVE result;
    result.code = code;
    result.message = message;
    return result;
}

[[nodiscard]] bool HasValidWeightsUVE(const UVE::Core::MotionMatchingWeightsUVE weights) noexcept {
    return std::isfinite(weights.velocityWeight) && std::isfinite(weights.facingWeight) &&
           std::isfinite(weights.trajectoryWeight) && weights.velocityWeight >= 0.0F &&
           weights.facingWeight >= 0.0F && weights.trajectoryWeight >= 0.0F &&
           (weights.velocityWeight + weights.facingWeight + weights.trajectoryWeight) > 0.0F;
}
} // namespace

MotionQueryAnimationNodeResultUVE EvaluateMotionQueryAnimationNodeUVE(
    const UVE::Core::MotionQueryUVE& query,
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema,
    const MotionQuerySearchIndexUVE& searchIndex,
    const std::vector<UVE::Core::AnimationClipUVE>& clips,
    MotionQueryAnimationNodeSettingsUVE settings,
    IMotionQueryAnimationDebugSinkUVE* debugSink,
    const std::uint64_t timestampNanoseconds,
    const std::uint64_t frameNumber) noexcept {
    const auto publish = [debugSink, timestampNanoseconds, frameNumber](
                             MotionQueryAnimationNodeResultUVE result) noexcept {
        if (debugSink != nullptr) {
            debugSink->PublishUVE(result, timestampNanoseconds, frameNumber);
        }
        return result;
    };
    if (settings.maximumSearchResults == 0U ||
        settings.maximumSearchResults > MotionQuerySearchIndexUVE::kMaximumQueryResultsUVE ||
        !HasValidWeightsUVE(settings.weights)) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidSettings,
                             "motion query animation node settings are invalid"));
    }
    const UVE::Core::MotionQueryValidationResultUVE queryValidation =
        UVE::Core::ValidateMotionQueryUVE(query);
    if (!queryValidation.IsValidUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidQuery,
                             queryValidation.message.c_str()));
    }
    const UVE::Core::MotionMatchingDatabaseValidationResultUVE databaseValidation =
        UVE::Core::ValidateMotionMatchingDatabaseUVE(database);
    if (!databaseValidation.IsValidUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidDatabase,
                             databaseValidation.message.c_str()));
    }
    const UVE::Core::MotionQueryFeatureValidationResultUVE schemaValidation =
        UVE::Core::ValidateMotionQueryFeatureSchemaUVE(schema);
    if (!schemaValidation.IsValidUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidSchema,
                             schemaValidation.message.c_str()));
    }
    if (!searchIndex.IsBuiltUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::IndexNotBuilt,
                             "motion query animation node search index is not built"));
    }
    if (!searchIndex.IsCompatibleWithSchemaUVE(schema)) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::SchemaMismatch,
                             "motion query animation node search index schema is incompatible"));
    }

    UVE::Core::MotionQueryFeatureVectorUVE queryFeature;
    const UVE::Core::MotionQueryFeatureValidationResultUVE extraction =
        UVE::Core::TryBuildMotionQueryFeatureVectorUVE(query, schema, queryFeature);
    if (!extraction.IsValidUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidQuery,
                             extraction.message.c_str()));
    }

    std::vector<std::size_t> indexedCandidates;
    const MotionQuerySearchIndexResultUVE searchResult =
        searchIndex.FindNearestUVE(queryFeature, settings.maximumSearchResults, indexedCandidates);
    if (!searchResult.IsAcceptedUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::SearchFailed,
                             searchResult.message.c_str()));
    }
    if (indexedCandidates.empty()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::NoMatch,
                             "motion query animation node found no indexed candidates"));
    }

    UVE::Core::MotionMatchingDatabaseUVE filteredDatabase;
    filteredDatabase.candidates.reserve(indexedCandidates.size());
    for (const std::size_t candidateIndex : indexedCandidates) {
        if (candidateIndex >= database.candidates.size()) {
            return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::CandidateIndexOutOfRange,
                                 "motion query animation node index points outside the database"));
        }
        filteredDatabase.candidates.push_back(database.candidates[candidateIndex]);
    }

    const UVE::Core::MotionMatchingResultUVE match = UVE::Core::FindBestMotionMatchUVE(
        query, filteredDatabase, settings.weights);
    if (!match.IsMatchUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::NoMatch, match.message.c_str()));
    }
    if (match.candidateIndex >= indexedCandidates.size()) {
            return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::CandidateIndexOutOfRange,
                             "motion query animation node match index is out of range"));
    }

    const std::size_t originalCandidateIndex = indexedCandidates[match.candidateIndex];
    const UVE::Core::MotionMatchingCandidateUVE& candidate = database.candidates[originalCandidateIndex];
    const auto clip = std::find_if(clips.cbegin(), clips.cend(), [&candidate](const auto& value) {
        return value.clipId == candidate.sourceClipId;
    });
    if (clip == clips.cend()) {
        MotionQueryAnimationNodeResultUVE result = MakeResultUVE(
            MotionQueryAnimationNodeCodeUVE::MissingClip,
            "motion query animation node candidate references a missing animation clip");
        result.candidateIndex = originalCandidateIndex;
        result.cost = match.cost;
        result.candidatesEvaluated = match.candidatesEvaluated;
        result.sampleTimeSeconds = candidate.sampleTimeSeconds;
        result.sourceClipId = candidate.sourceClipId;
        return publish(std::move(result));
    }

    UVE::Core::TransformPoseUVE pose;
    if (!UVE::Core::TrySampleAnimationClipUVE(*clip, candidate.sampleTimeSeconds, settings.looping,
                                             pose)) {
        MotionQueryAnimationNodeResultUVE result = MakeResultUVE(
            MotionQueryAnimationNodeCodeUVE::PoseSamplingFailed,
            "motion query animation node failed to sample the selected animation clip");
        result.candidateIndex = originalCandidateIndex;
        result.cost = match.cost;
        result.candidatesEvaluated = match.candidatesEvaluated;
        result.sampleTimeSeconds = candidate.sampleTimeSeconds;
        result.sourceClipId = candidate.sourceClipId;
        return publish(std::move(result));
    }

    MotionQueryAnimationNodeResultUVE result;
    result.code = MotionQueryAnimationNodeCodeUVE::Accepted;
    result.candidateIndex = originalCandidateIndex;
    result.candidatesEvaluated = match.candidatesEvaluated;
    result.cost = match.cost;
    result.sampleTimeSeconds = candidate.sampleTimeSeconds;
    result.sourceClipId = candidate.sourceClipId;
    result.pose = pose;
    result.message = "motion query animation node evaluated successfully";
    return publish(std::move(result));
}

MotionQueryAnimationNodeResultUVE EvaluateMotionQueryAnimationNodeFromHistoryUVE(
    const UVE::Core::MotionQueryHistoryBufferUVE& history, const double evaluationTimeSeconds,
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema,
    const MotionQuerySearchIndexUVE& searchIndex,
    const std::vector<UVE::Core::AnimationClipUVE>& clips,
    MotionQueryAnimationNodeSettingsUVE settings,
    IMotionQueryAnimationDebugSinkUVE* debugSink,
    const std::uint64_t timestampNanoseconds,
    const std::uint64_t frameNumber) noexcept {
    const auto publish = [debugSink, timestampNanoseconds, frameNumber](
                             MotionQueryAnimationNodeResultUVE result) noexcept {
        if (debugSink != nullptr) {
            debugSink->PublishUVE(result, timestampNanoseconds, frameNumber);
        }
        return result;
    };
    if (!std::isfinite(evaluationTimeSeconds)) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidEvaluationTime,
                             "motion query animation node evaluation time is not finite"));
    }
    const auto& frames = history.GetFramesUVE();
    const auto frame = std::find_if(frames.crbegin(), frames.crend(), [evaluationTimeSeconds](const auto& value) {
        return value.sample.timeSeconds <= evaluationTimeSeconds;
    });
    if (frame == frames.crend()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::NoHistoryFrame,
                             "motion query animation node has no history frame at or before evaluation time"));
    }
    return EvaluateMotionQueryAnimationNodeUVE(frame->query, database, schema, searchIndex, clips,
                                               settings, debugSink, timestampNanoseconds, frameNumber);
}

} // namespace UVE::Plugins
