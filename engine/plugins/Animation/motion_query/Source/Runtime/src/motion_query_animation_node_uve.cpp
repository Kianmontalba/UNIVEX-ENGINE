// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_animation_node_uve.h"

#include <algorithm>
#include <cmath>
#include <iterator>
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
    const double totalWeight = static_cast<double>(weights.velocityWeight) +
                                static_cast<double>(weights.facingWeight) +
                                static_cast<double>(weights.trajectoryWeight);
    const float floatTotalWeight = static_cast<float>(totalWeight);
    return std::isfinite(weights.velocityWeight) && std::isfinite(weights.facingWeight) &&
           std::isfinite(weights.trajectoryWeight) && std::isfinite(totalWeight) &&
           std::isfinite(floatTotalWeight) && weights.velocityWeight >= 0.0F &&
           weights.facingWeight >= 0.0F && weights.trajectoryWeight >= 0.0F && totalWeight > 0.0;
}
} // namespace

MotionQueryAnimationSettingsValidationResultUVE ValidateMotionQueryAnimationNodeSettingsUVE(
    const MotionQueryAnimationNodeSettingsUVE& settings) noexcept {
    const MotionQuerySearchBudgetUVE budget =
        ResolveMotionQuerySearchBudgetUVE(settings.qualityTier, settings.maximumSearchResults);
    if (budget.code == MotionQuerySearchBudgetCodeUVE::InvalidResultCount) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidSearchResults,
            budget.message};
    }
    if (budget.code == MotionQuerySearchBudgetCodeUVE::InvalidQualityTier) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidQualityTier,
            budget.message};
    }
    if (!HasValidWeightsUVE(settings.weights)) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidWeights,
            "motion query animation node weights are invalid"};
    }
    if (settings.continuity.policy != MotionQueryContinuityPolicyUVE::Disabled &&
        settings.continuity.policy != MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidContinuityPolicy,
            "motion query animation node continuity policy is unsupported"};
    }
    if (!std::isfinite(settings.continuity.maximumPreviousAgeSeconds) ||
        settings.continuity.maximumPreviousAgeSeconds < 0.0) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidContinuityAge,
            "motion query animation node continuity age is invalid"};
    }
    if (!std::isfinite(settings.transition.minimumCostImprovement) ||
        settings.transition.minimumCostImprovement < 0.0F ||
        settings.transition.minimumCostImprovement > MotionQueryTransitionSettingsUVE::kMaximumCostImprovementUVE ||
        !std::isfinite(settings.transition.minimumHoldSeconds) ||
        settings.transition.minimumHoldSeconds < 0.0 ||
        !std::isfinite(settings.transition.maximumHoldWindowSeconds) ||
        settings.transition.maximumHoldWindowSeconds < 0.0 ||
        settings.transition.maximumHoldWindowSeconds >
            MotionQueryTransitionSettingsUVE::kMaximumHoldWindowSecondsUVE ||
        settings.transition.minimumHoldSeconds > settings.transition.maximumHoldWindowSeconds) {
        return MotionQueryAnimationSettingsValidationResultUVE{
            MotionQueryAnimationSettingsValidationCodeUVE::InvalidTransitionSettings,
            "motion query animation node transition settings are invalid"};
    }
    return MotionQueryAnimationSettingsValidationResultUVE{
        MotionQueryAnimationSettingsValidationCodeUVE::Valid, "valid"};
}

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
    const MotionQueryAnimationSettingsValidationResultUVE settingsValidation =
        ValidateMotionQueryAnimationNodeSettingsUVE(settings);
    if (!settingsValidation.IsValidUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidSettings,
                             settingsValidation.message.c_str()));
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
    const MotionQuerySearchBudgetUVE budget =
        ResolveMotionQuerySearchBudgetUVE(settings.qualityTier, settings.maximumSearchResults);
    if (!budget.IsAcceptedUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::InvalidSettings,
                             budget.message.c_str()));
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
        searchIndex.FindNearestUVE(queryFeature, budget.effectiveResults, indexedCandidates);
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
    const MotionQueryRuntimeTelemetryUVE telemetry = BuildMotionQueryRuntimeTelemetryUVE(
        searchIndex.SizeUVE(), budget, indexedCandidates.size(), match.candidatesEvaluated);
    if (!telemetry.IsAcceptedUVE()) {
        return publish(MakeResultUVE(MotionQueryAnimationNodeCodeUVE::SearchFailed,
                                     telemetry.message.c_str()));
    }
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
        result.requestedSearchResults = budget.requestedResults;
        result.effectiveSearchResults = budget.effectiveResults;
        result.qualityTier = settings.qualityTier;
        result.searchBudgetDowngraded = budget.WasDowngradedUVE();
        result.cost = match.cost;
        result.candidatesEvaluated = match.candidatesEvaluated;
        result.sampleTimeSeconds = candidate.sampleTimeSeconds;
        result.sourceClipId = candidate.sourceClipId;
        result.telemetryCode = telemetry.code;
        result.telemetryIndexEntryCount = telemetry.indexEntryCount;
        result.telemetryCandidatesConsidered = telemetry.candidatesConsidered;
        result.telemetryBudgetSaturated = telemetry.searchBudgetSaturated;
        return publish(std::move(result));
    }

    UVE::Core::TransformPoseUVE pose;
    if (!UVE::Core::TrySampleAnimationClipUVE(*clip, candidate.sampleTimeSeconds, settings.looping,
                                             pose)) {
        MotionQueryAnimationNodeResultUVE result = MakeResultUVE(
            MotionQueryAnimationNodeCodeUVE::PoseSamplingFailed,
            "motion query animation node failed to sample the selected animation clip");
        result.candidateIndex = originalCandidateIndex;
        result.requestedSearchResults = budget.requestedResults;
        result.effectiveSearchResults = budget.effectiveResults;
        result.qualityTier = settings.qualityTier;
        result.searchBudgetDowngraded = budget.WasDowngradedUVE();
        result.cost = match.cost;
        result.candidatesEvaluated = match.candidatesEvaluated;
        result.sampleTimeSeconds = candidate.sampleTimeSeconds;
        result.sourceClipId = candidate.sourceClipId;
        result.telemetryCode = telemetry.code;
        result.telemetryIndexEntryCount = telemetry.indexEntryCount;
        result.telemetryCandidatesConsidered = telemetry.candidatesConsidered;
        result.telemetryBudgetSaturated = telemetry.searchBudgetSaturated;
        return publish(std::move(result));
    }

    MotionQueryAnimationNodeResultUVE result;
    result.code = MotionQueryAnimationNodeCodeUVE::Accepted;
    result.candidateIndex = originalCandidateIndex;
    result.candidatesEvaluated = match.candidatesEvaluated;
    result.requestedSearchResults = budget.requestedResults;
    result.effectiveSearchResults = budget.effectiveResults;
    result.qualityTier = settings.qualityTier;
    result.searchBudgetDowngraded = budget.WasDowngradedUVE();
    result.cost = match.cost;
    result.sampleTimeSeconds = candidate.sampleTimeSeconds;
    result.sourceClipId = candidate.sourceClipId;
    result.pose = pose;
    result.telemetryCode = telemetry.code;
    result.telemetryIndexEntryCount = telemetry.indexEntryCount;
    result.telemetryCandidatesConsidered = telemetry.candidatesConsidered;
    result.telemetryBudgetSaturated = telemetry.searchBudgetSaturated;
    result.message = "motion query animation node evaluated successfully";
    return publish(std::move(result));
}

MotionQueryAnimationNodeResultUVE EvaluateMotionQueryAnimationNodeWithContinuityUVE(
    const UVE::Core::MotionQueryUVE& query,
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema,
    const MotionQuerySearchIndexUVE& searchIndex,
    const std::vector<UVE::Core::AnimationClipUVE>& clips,
    MotionQueryAnimationNodeSettingsUVE settings,
    const UVE::Core::PoseSampleUVE* previousSample,
    const double continuityTimeSeconds,
    const MotionQueryAnimationNodeResultUVE* previousResult,
    IMotionQueryAnimationDebugSinkUVE* debugSink,
    const std::uint64_t timestampNanoseconds,
    const std::uint64_t frameNumber) noexcept {
    MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        query, database, schema, searchIndex, clips, settings, debugSink,
        timestampNanoseconds, frameNumber);
    if (!result.IsAcceptedUVE()) {
        return result;
    }
    const double continuityTime = continuityTimeSeconds >= 0.0
                                      ? continuityTimeSeconds
                                      : result.sampleTimeSeconds;
    const MotionQueryContinuityResultUVE continuity = ApplyMotionQueryContinuityUVE(
        result.pose, previousSample, continuityTime, settings.continuity);
    result.continuityCode = continuity.code;
    result.continuityPreviousAgeSeconds = continuity.previousAgeSeconds;
    result.continuityApplied = continuity.WasAppliedUVE();
    if (!continuity.IsAcceptedUVE()) {
        result.code = continuity.code == MotionQueryContinuityCodeUVE::InvalidSettings
                          ? MotionQueryAnimationNodeCodeUVE::InvalidSettings
                          : MotionQueryAnimationNodeCodeUVE::ContinuityFailed;
        result.message = continuity.message;
        return result;
    }
    result.pose = continuity.pose;
    result.message = continuity.message;

    MotionQueryTransitionRequestUVE transitionRequest;
    transitionRequest.hasPreviousSelection = previousResult != nullptr && previousResult->IsAcceptedUVE();
    if (transitionRequest.hasPreviousSelection) {
        transitionRequest.previousCandidateIndex = previousResult->candidateIndex;
        transitionRequest.previousCost = previousResult->cost;
        transitionRequest.elapsedSeconds = continuityTime - previousResult->sampleTimeSeconds;
    }
    transitionRequest.currentCandidateIndex = result.candidateIndex;
    transitionRequest.currentCost = result.cost;
    const MotionQueryTransitionResultUVE transition =
        ArbitrateMotionQueryTransitionUVE(transitionRequest, settings.transition);
    result.transitionCode = transition.code;
    result.transitionCostImprovement = transition.costImprovement;
    result.transitionHeldPrevious = transition.ShouldHoldPreviousUVE();
    if (!transition.IsAcceptedUVE()) {
        result.code = MotionQueryAnimationNodeCodeUVE::InvalidSettings;
        result.message = transition.message;
        return result;
    }
    if (transition.ShouldHoldPreviousUVE()) {
        MotionQueryAnimationNodeResultUVE held = *previousResult;
        held.transitionCode = transition.code;
        held.transitionCostImprovement = transition.costImprovement;
        held.transitionHeldPrevious = true;
        held.message = transition.message;
        return held;
    }
    result.message = transition.message;
    return result;
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
    const UVE::Core::PoseSampleUVE* previousSample = nullptr;
    MotionQueryAnimationNodeResultUVE previousResult;
    const MotionQueryAnimationNodeResultUVE* previousResultPointer = nullptr;
    const auto previousFrame = std::next(frame);
    if (previousFrame != frames.crend()) {
        previousSample = &previousFrame->sample;
        previousResult = EvaluateMotionQueryAnimationNodeUVE(
            previousFrame->query, database, schema, searchIndex, clips, settings, nullptr);
        if (previousResult.IsAcceptedUVE()) {
            previousResultPointer = &previousResult;
        }
    }
    return EvaluateMotionQueryAnimationNodeWithContinuityUVE(
        frame->query, database, schema, searchIndex, clips, settings, previousSample,
        evaluationTimeSeconds, previousResultPointer, debugSink, timestampNanoseconds, frameNumber);
}

} // namespace UVE::Plugins
