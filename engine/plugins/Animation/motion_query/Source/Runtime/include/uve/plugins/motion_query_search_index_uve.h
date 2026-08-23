// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_feature_channels_uve.h"
#include "uve/plugins/motion_query_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

struct MotionQuerySearchIndexResultUVE final {
    enum class Code : std::uint8_t {
        Accepted = 0,
        EmptyDatabase,
        CapacityExceeded,
        InvalidDatabase,
        InvalidSchema,
        FeatureExtractionFailed,
        EmptyEntries,
        InvalidCandidateIndex,
        DuplicateCandidateIndex,
        InconsistentFeatureDimensions,
        NonFiniteFeature,
        InvalidQuery,
        InvalidResultCount,
    };

    Code code = Code::InvalidDatabase;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == Code::Accepted;
    }
};

// A value-only index entry. The index stores copied features and candidate indices, never
// database references, ECS pointers, asset handles, or renderer resources.
struct MotionQuerySearchEntryUVE final {
    std::size_t candidateIndex = 0U;
    UVE::Core::MotionQueryFeatureVectorUVE feature;

    [[nodiscard]] bool operator==(const MotionQuerySearchEntryUVE&) const = default;
};

class MotionQuerySearchIndexUVE final {
public:
    static constexpr std::size_t kMaximumEntriesUVE = 4096U;
    static constexpr std::size_t kMaximumQueryResultsUVE = 4096U;

    MotionQuerySearchIndexUVE() = default;

    [[nodiscard]] MotionQuerySearchIndexResultUVE BuildUVE(
        const UVE::Core::MotionMatchingDatabaseUVE& database,
        const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept;

    [[nodiscard]] MotionQuerySearchIndexResultUVE BuildFromEntriesUVE(
        const std::vector<MotionQuerySearchEntryUVE>& entries) noexcept;

    [[nodiscard]] MotionQuerySearchIndexResultUVE FindNearestUVE(
        const UVE::Core::MotionQueryFeatureVectorUVE& query, std::size_t maximumResults,
        std::vector<std::size_t>& outCandidateIndices) const noexcept;

    void ClearUVE() noexcept;

    [[nodiscard]] bool IsBuiltUVE() const noexcept {
        return !nodes_.empty();
    }

    [[nodiscard]] std::size_t SizeUVE() const noexcept {
        return nodes_.size();
    }

    [[nodiscard]] std::size_t DimensionUVE() const noexcept {
        return dimension_;
    }

    [[nodiscard]] bool IsCompatibleWithSchemaUVE(
        const UVE::Core::MotionQueryFeatureSchemaUVE& schema) const noexcept;

private:
    static constexpr std::size_t kNoChildUVE = static_cast<std::size_t>(-1);

    struct NodeUVE final {
        MotionQuerySearchEntryUVE entry;
        std::size_t splitDimension = 0U;
        std::size_t left = kNoChildUVE;
        std::size_t right = kNoChildUVE;
    };

    struct RankedCandidateUVE final {
        std::size_t candidateIndex = 0U;
        double distanceSquared = 0.0;
    };

    [[nodiscard]] std::size_t BuildTreeUVE(std::vector<MotionQuerySearchEntryUVE>& entries,
                                           std::size_t begin, std::size_t end,
                                           std::size_t depth) noexcept;

    void SearchTreeUVE(std::size_t nodeIndex, const std::vector<float>& query,
                      std::size_t maximumResults,
                      std::vector<RankedCandidateUVE>& outRanked) const noexcept;

    static void InsertRankedCandidateUVE(const RankedCandidateUVE& candidate,
                                         std::size_t maximumResults,
                                         std::vector<RankedCandidateUVE>& ranked) noexcept;

    std::vector<NodeUVE> nodes_;
    std::size_t root_ = kNoChildUVE;
    std::size_t dimension_ = 0U;
    std::uint32_t schemaVersion_ = 0U;
};

} // namespace UVE::Plugins
