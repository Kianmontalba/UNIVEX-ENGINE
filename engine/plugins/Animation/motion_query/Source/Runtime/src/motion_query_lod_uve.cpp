// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_lod_uve.h"

#include <algorithm>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQuerySearchBudgetUVE MakeBudgetUVE(
    const MotionQuerySearchBudgetCodeUVE code, const std::size_t requested,
    const std::size_t effective, const char* message) noexcept {
    return MotionQuerySearchBudgetUVE{code, requested, effective, message};
}
} // namespace

MotionQuerySearchBudgetUVE ResolveMotionQuerySearchBudgetUVE(
    const MotionQueryQualityTierUVE quality, const std::size_t requestedResults) noexcept {
    if (requestedResults == 0U || requestedResults > MotionQuerySearchBudgetUVE::kMaximumResultsUVE) {
        return MakeBudgetUVE(MotionQuerySearchBudgetCodeUVE::InvalidResultCount, requestedResults, 0U,
                             "motion query requested search results are outside the bounded range");
    }

    std::size_t cap = MotionQuerySearchBudgetUVE::kMaximumResultsUVE;
    switch (quality) {
        case MotionQueryQualityTierUVE::Full:
            break;
        case MotionQueryQualityTierUVE::Reduced:
            cap = MotionQuerySearchBudgetUVE::kReducedMaximumResultsUVE;
            break;
        case MotionQueryQualityTierUVE::Minimal:
            cap = MotionQuerySearchBudgetUVE::kMinimalMaximumResultsUVE;
            break;
        default:
            return MakeBudgetUVE(MotionQuerySearchBudgetCodeUVE::InvalidQualityTier,
                                 requestedResults, 0U,
                                 "motion query quality tier is outside the supported range");
    }

    const std::size_t effective = std::min(requestedResults, cap);
    if (effective != requestedResults) {
        return MakeBudgetUVE(MotionQuerySearchBudgetCodeUVE::Downgraded, requestedResults, effective,
                             "motion query search budget was reduced by the quality tier");
    }
    return MakeBudgetUVE(MotionQuerySearchBudgetCodeUVE::Accepted, requestedResults, effective,
                         "motion query search budget accepted");
}

} // namespace UVE::Plugins
