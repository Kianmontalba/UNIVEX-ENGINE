// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_search_index_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Plugins {
namespace {
constexpr float kDistanceTieEpsilonUVE = 1.0e-6F;

[[nodiscard]] MotionQuerySearchIndexResultUVE MakeResultUVE(
    MotionQuerySearchIndexResultUVE::Code code, std::size_t index, const char* message) noexcept {
    return MotionQuerySearchIndexResultUVE{code, index, message};
}

[[nodiscard]] bool IsFiniteFeatureUVE(
    const UVE::Core::MotionQueryFeatureVectorUVE& feature) noexcept {
    if (!std::isfinite(feature.totalWeight)) {
        return false;
    }
    for (const float value : feature.values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return true;
}

} // namespace

MotionQuerySearchIndexResultUVE MotionQuerySearchIndexUVE::BuildUVE(
    const UVE::Core::MotionMatchingDatabaseUVE& database,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept {
    const UVE::Core::MotionMatchingDatabaseValidationResultUVE databaseValidation =
        UVE::Core::ValidateMotionMatchingDatabaseUVE(database);
    if (!databaseValidation.IsValidUVE()) {
        if (databaseValidation.code == UVE::Core::MotionMatchingDatabaseValidationCodeUVE::EmptyDatabase) {
            return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::EmptyDatabase,
                                 databaseValidation.index, "motion query search database is empty");
        }
        if (databaseValidation.code ==
            UVE::Core::MotionMatchingDatabaseValidationCodeUVE::CapacityExceeded) {
            return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::CapacityExceeded,
                                 databaseValidation.index, "motion query search database exceeds capacity");
        }
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidDatabase,
                             databaseValidation.index, databaseValidation.message.c_str());
    }

    const UVE::Core::MotionQueryFeatureValidationResultUVE schemaValidation =
        UVE::Core::ValidateMotionQueryFeatureSchemaUVE(schema);
    if (!schemaValidation.IsValidUVE()) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidSchema,
                             schemaValidation.index, schemaValidation.message.c_str());
    }

    std::vector<MotionQuerySearchEntryUVE> entries;
    entries.reserve(database.candidates.size());
    for (std::size_t index = 0U; index < database.candidates.size(); ++index) {
        UVE::Core::MotionQueryFeatureVectorUVE feature;
        const UVE::Core::MotionQueryFeatureValidationResultUVE extraction =
            UVE::Core::TryBuildMotionQueryFeatureVectorUVE(database.candidates[index].feature,
                                                            schema, feature);
        if (!extraction.IsValidUVE()) {
            return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::FeatureExtractionFailed,
                                 index, extraction.message.c_str());
        }
        entries.push_back(MotionQuerySearchEntryUVE{index, std::move(feature)});
    }
    MotionQuerySearchIndexResultUVE result = BuildFromEntriesUVE(entries);
    if (result.IsAcceptedUVE()) {
        schemaVersion_ = schema.version;
    }
    return result;
}

MotionQuerySearchIndexResultUVE MotionQuerySearchIndexUVE::BuildFromEntriesUVE(
    const std::vector<MotionQuerySearchEntryUVE>& entries) noexcept {
    ClearUVE();
    if (entries.empty()) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::EmptyEntries, 0U,
                             "motion query search entries are empty");
    }
    if (entries.size() > kMaximumEntriesUVE) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::CapacityExceeded,
                             entries.size(), "motion query search entries exceed capacity");
    }
    const std::size_t dimension = entries.front().feature.values.size();
    if (dimension == 0U) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InconsistentFeatureDimensions,
                             0U, "motion query search feature dimension is zero");
    }

    for (std::size_t index = 0U; index < entries.size(); ++index) {
        const MotionQuerySearchEntryUVE& entry = entries[index];
        if (entry.candidateIndex >= kMaximumEntriesUVE) {
            return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidCandidateIndex,
                                 index, "motion query search candidate index exceeds capacity");
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (entries[previous].candidateIndex == entry.candidateIndex) {
                return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::DuplicateCandidateIndex,
                                     index, "motion query search candidate index is duplicated");
            }
        }
        if (entry.feature.values.size() != dimension) {
            return MakeResultUVE(
                MotionQuerySearchIndexResultUVE::Code::InconsistentFeatureDimensions, index,
                "motion query search feature dimensions are inconsistent");
        }
        if (!IsFiniteFeatureUVE(entry.feature)) {
            return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::NonFiniteFeature, index,
                                 "motion query search feature contains a non-finite value");
        }
    }

    std::vector<MotionQuerySearchEntryUVE> sortedEntries = entries;
    dimension_ = dimension;
    nodes_.reserve(sortedEntries.size());
    root_ = BuildTreeUVE(sortedEntries, 0U, sortedEntries.size(), 0U);
    return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::Accepted, 0U,
                         "motion query search index built");
}

std::size_t MotionQuerySearchIndexUVE::BuildTreeUVE(
    std::vector<MotionQuerySearchEntryUVE>& entries, std::size_t begin, std::size_t end,
    std::size_t depth) noexcept {
    if (begin >= end) {
        return kNoChildUVE;
    }
    const std::size_t splitDimension = depth % dimension_;
    const auto first = entries.begin() + static_cast<std::ptrdiff_t>(begin);
    const auto last = entries.begin() + static_cast<std::ptrdiff_t>(end);
    std::stable_sort(first, last, [splitDimension](const MotionQuerySearchEntryUVE& lhs,
                                                   const MotionQuerySearchEntryUVE& rhs) noexcept {
        const float left = lhs.feature.values[splitDimension];
        const float right = rhs.feature.values[splitDimension];
        if (left != right) {
            return left < right;
        }
        return lhs.candidateIndex < rhs.candidateIndex;
    });

    const std::size_t middle = begin + ((end - begin) / 2U);
    const std::size_t nodeIndex = nodes_.size();
    nodes_.push_back(NodeUVE{std::move(entries[middle]), splitDimension, kNoChildUVE,
                             kNoChildUVE});
    nodes_[nodeIndex].left = BuildTreeUVE(entries, begin, middle, depth + 1U);
    nodes_[nodeIndex].right = BuildTreeUVE(entries, middle + 1U, end, depth + 1U);
    return nodeIndex;
}

MotionQuerySearchIndexResultUVE MotionQuerySearchIndexUVE::FindNearestUVE(
    const UVE::Core::MotionQueryFeatureVectorUVE& query, std::size_t maximumResults,
    std::vector<std::size_t>& outCandidateIndices) const noexcept {
    outCandidateIndices.clear();
    if (!IsBuiltUVE()) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::EmptyEntries, 0U,
                             "motion query search index is not built");
    }
    if (maximumResults == 0U || maximumResults > kMaximumQueryResultsUVE) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidResultCount,
                             maximumResults, "motion query search result count is out of bounds");
    }
    if (query.values.size() != dimension_) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidQuery, 0U,
                             "motion query search query dimension does not match the index");
    }
    if (!IsFiniteFeatureUVE(query)) {
        return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::InvalidQuery, 0U,
                             "motion query search query contains a non-finite value");
    }

    std::vector<RankedCandidateUVE> ranked;
    ranked.reserve(std::min(maximumResults, nodes_.size()));
    SearchTreeUVE(root_, query.values, maximumResults, ranked);
    outCandidateIndices.reserve(ranked.size());
    for (const RankedCandidateUVE& candidate : ranked) {
        outCandidateIndices.push_back(candidate.candidateIndex);
    }
    return MakeResultUVE(MotionQuerySearchIndexResultUVE::Code::Accepted,
                         outCandidateIndices.size(), "motion query search completed");
}

void MotionQuerySearchIndexUVE::SearchTreeUVE(
    std::size_t nodeIndex, const std::vector<float>& query, std::size_t maximumResults,
    std::vector<RankedCandidateUVE>& outRanked) const noexcept {
    if (nodeIndex == kNoChildUVE) {
        return;
    }
    const NodeUVE& node = nodes_[nodeIndex];
    const double difference = static_cast<double>(query[node.splitDimension]) -
                              static_cast<double>(node.entry.feature.values[node.splitDimension]);
    const std::size_t nearChild = difference < 0.0 ? node.left : node.right;
    const std::size_t farChild = difference < 0.0 ? node.right : node.left;
    SearchTreeUVE(nearChild, query, maximumResults, outRanked);

    double distanceSquared = 0.0;
    for (std::size_t dimension = 0U; dimension < query.size(); ++dimension) {
        const double delta = static_cast<double>(query[dimension]) -
                             static_cast<double>(node.entry.feature.values[dimension]);
        distanceSquared += delta * delta;
    }
    if (std::isfinite(distanceSquared)) {
        InsertRankedCandidateUVE(
            RankedCandidateUVE{node.entry.candidateIndex, distanceSquared}, maximumResults,
            outRanked);
    }

    bool searchFar = outRanked.size() < maximumResults;
    if (!searchFar) {
        const double splitDistanceSquared = difference * difference;
        searchFar = !std::isfinite(splitDistanceSquared) ||
                    splitDistanceSquared <= outRanked.back().distanceSquared + kDistanceTieEpsilonUVE;
    }
    if (searchFar) {
        SearchTreeUVE(farChild, query, maximumResults, outRanked);
    }
}

void MotionQuerySearchIndexUVE::InsertRankedCandidateUVE(
    const RankedCandidateUVE& candidate, std::size_t maximumResults,
    std::vector<RankedCandidateUVE>& ranked) noexcept {
    const auto insertion = std::lower_bound(
        ranked.begin(), ranked.end(), candidate,
        [](const RankedCandidateUVE& lhs, const RankedCandidateUVE& rhs) noexcept {
            if (std::fabs(lhs.distanceSquared - rhs.distanceSquared) > kDistanceTieEpsilonUVE) {
                return lhs.distanceSquared < rhs.distanceSquared;
            }
            return lhs.candidateIndex < rhs.candidateIndex;
        });
    ranked.insert(insertion, candidate);
    if (ranked.size() > maximumResults) {
        ranked.pop_back();
    }
}

bool MotionQuerySearchIndexUVE::IsCompatibleWithSchemaUVE(
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema) const noexcept {
    return IsBuiltUVE() && schemaVersion_ == schema.version && dimension_ == schema.channels.size();
}

void MotionQuerySearchIndexUVE::ClearUVE() noexcept {
    nodes_.clear();
    nodes_.shrink_to_fit();
    root_ = kNoChildUVE;
    dimension_ = 0U;
    schemaVersion_ = 0U;
}

} // namespace UVE::Plugins
