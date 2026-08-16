// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/core/animation_clip_uve.h"
#include "uve/core/motion_query_feature_channels_uve.h"
#include "uve/core/motion_query_history_uve.h"
#include "uve/core/motion_query_uve.h"
#include "uve/plugins/motion_query_search_index_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

struct MotionQueryAnimationNodeSettingsUVE final {
    UVE::Core::MotionMatchingWeightsUVE weights;
    std::size_t maximumSearchResults = 32U;
    bool looping = true;
};

enum class MotionQueryAnimationNodeCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidSettings,
    InvalidQuery,
    InvalidDatabase,
    InvalidSchema,
    SchemaMismatch,
    IndexNotBuilt,
    SearchFailed,
    NoMatch,
    CandidateIndexOutOfRange,
    MissingClip,
    PoseSamplingFailed,
    InvalidEvaluationTime,
    NoHistoryFrame,
};

struct MotionQueryAnimationNodeResultUVE final {
    MotionQueryAnimationNodeCodeUVE code = MotionQueryAnimationNodeCodeUVE::InvalidSettings;
    std::size_t candidateIndex = 0U;
    std::size_t candidatesEvaluated = 0U;
    float cost = 0.0F;
    double sampleTimeSeconds = 0.0;
    std::string sourceClipId;
    UVE::Core::TransformPoseUVE pose;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryAnimationNodeCodeUVE::Accepted;
    }
};

/// Runtime-facing value-only sink. Implementations may live in the editor/diagnostics layer, but the
/// animation evaluator never owns or discovers them. A null sink is the normal shipping path.
class IMotionQueryAnimationDebugSinkUVE {
public:
    virtual ~IMotionQueryAnimationDebugSinkUVE() = default;
    virtual void PublishUVE(const MotionQueryAnimationNodeResultUVE& result,
                            std::uint64_t timestampNanoseconds,
                            std::uint64_t frameNumber) noexcept = 0;
};

// Evaluates one native motion-matching node. The database, schema, search index, and clips are
// borrowed for the call only. The result owns copied metadata and pose values; no asset, ECS,
// renderer, animation-tree, or managed-editor ownership crosses this adapter boundary.
[[nodiscard]] MotionQueryAnimationNodeResultUVE EvaluateMotionQueryAnimationNodeUVE(
    const UVE::Core::MotionQueryUVE& query,
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema,
    const MotionQuerySearchIndexUVE& searchIndex,
    const std::vector<UVE::Core::AnimationClipUVE>& clips,
    MotionQueryAnimationNodeSettingsUVE settings,
    IMotionQueryAnimationDebugSinkUVE* debugSink = nullptr,
    std::uint64_t timestampNanoseconds = 0U,
    std::uint64_t frameNumber = 0U) noexcept;

[[nodiscard]] MotionQueryAnimationNodeResultUVE EvaluateMotionQueryAnimationNodeFromHistoryUVE(
    const UVE::Core::MotionQueryHistoryBufferUVE& history, double evaluationTimeSeconds,
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema,
    const MotionQuerySearchIndexUVE& searchIndex,
    const std::vector<UVE::Core::AnimationClipUVE>& clips,
    MotionQueryAnimationNodeSettingsUVE settings,
    IMotionQueryAnimationDebugSinkUVE* debugSink = nullptr,
    std::uint64_t timestampNanoseconds = 0U,
    std::uint64_t frameNumber = 0U) noexcept;

} // namespace UVE::Plugins
