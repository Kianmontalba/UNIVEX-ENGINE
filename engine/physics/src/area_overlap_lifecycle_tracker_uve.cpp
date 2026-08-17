// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/area_overlap_lifecycle_tracker_uve.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace UVE::Physics {

namespace {

[[nodiscard]] bool PairLessUVE(const AreaOverlapPairUVE& lhs, const AreaOverlapPairUVE& rhs) noexcept {
    if (lhs.area.index != rhs.area.index) {
        return lhs.area.index < rhs.area.index;
    }
    if (lhs.area.generation != rhs.area.generation) {
        return lhs.area.generation < rhs.area.generation;
    }
    if (lhs.other.index != rhs.other.index) {
        return lhs.other.index < rhs.other.index;
    }
    return lhs.other.generation < rhs.other.generation;
}

[[nodiscard]] bool PairIdentityEqualUVE(const AreaOverlapPairUVE& lhs,
                                        const AreaOverlapPairUVE& rhs) noexcept {
    return lhs.area == rhs.area && lhs.other == rhs.other;
}

} // namespace

AreaOverlapLifecycleReportUVE AreaOverlapLifecycleTrackerUVE::UpdateUVE(
    const AreaOverlapQueryResultUVE& snapshot, const std::size_t maximumTransitions) {
    AreaOverlapLifecycleReportUVE report;
    report.previousActiveCount = m_activePairs.size();
    report.inputSnapshotTruncated = snapshot.truncated || snapshot.overlaps.size() > kMaximumAreaOverlapResultsUVE;
    if (report.inputSnapshotTruncated) {
        report.currentActiveCount = m_activePairs.size();
        return report;
    }

    std::vector<AreaOverlapPairUVE> currentPairs = snapshot.overlaps;
    std::sort(currentPairs.begin(), currentPairs.end(), PairLessUVE);
    currentPairs.erase(std::unique(currentPairs.begin(), currentPairs.end(), PairIdentityEqualUVE), currentPairs.end());

    report.currentActiveCount = currentPairs.size();
    const std::size_t transitionCap = std::min(maximumTransitions, kMaximumAreaOverlapResultsUVE);
    report.transitions.reserve(std::min(transitionCap, m_activePairs.size() + currentPairs.size()));

    for (const AreaOverlapPairUVE& previous : m_activePairs) {
        if (!std::binary_search(currentPairs.begin(), currentPairs.end(), previous, PairLessUVE)) {
            if (report.transitions.size() >= transitionCap) {
                report.transitionsTruncated = true;
                continue;
            }
            report.transitions.push_back({AreaOverlapTransitionKindUVE::Exited, previous});
        }
    }
    for (const AreaOverlapPairUVE& current : currentPairs) {
        if (!std::binary_search(m_activePairs.begin(), m_activePairs.end(), current, PairLessUVE)) {
            if (report.transitions.size() >= transitionCap) {
                report.transitionsTruncated = true;
                continue;
            }
            report.transitions.push_back({AreaOverlapTransitionKindUVE::Entered, current});
        }
    }

    std::sort(report.transitions.begin(), report.transitions.end(), [](const AreaOverlapTransitionUVE& lhs,
                                                                        const AreaOverlapTransitionUVE& rhs) {
        if (PairLessUVE(lhs.pair, rhs.pair)) {
            return true;
        }
        if (PairLessUVE(rhs.pair, lhs.pair)) {
            return false;
        }
        return lhs.kind < rhs.kind;
    });
    m_activePairs = std::move(currentPairs);
    return report;
}

void AreaOverlapLifecycleTrackerUVE::ResetUVE() noexcept { m_activePairs.clear(); }

} // namespace UVE::Physics
