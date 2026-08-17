// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "uve/physics/area_overlap_system_uve.h"

namespace UVE::Physics {

enum class AreaOverlapTransitionKindUVE : std::uint8_t {
    Entered = 0,
    Exited,
};

struct AreaOverlapTransitionUVE final {
    AreaOverlapTransitionKindUVE kind = AreaOverlapTransitionKindUVE::Entered;
    AreaOverlapPairUVE pair;

    [[nodiscard]] bool operator==(const AreaOverlapTransitionUVE&) const noexcept = default;
};

struct AreaOverlapLifecycleReportUVE final {
    std::size_t previousActiveCount = 0U;
    std::size_t currentActiveCount = 0U;
    bool inputSnapshotTruncated = false;
    bool transitionsTruncated = false;
    std::vector<AreaOverlapTransitionUVE> transitions;

    [[nodiscard]] bool IsTruncatedUVE() const noexcept {
        return inputSnapshotTruncated || transitionsTruncated;
    }
};

/// Tracks enter/exit transitions between complete AreaOverlapSystemUVE snapshots. The tracker
/// owns only copied generation-safe pairs and never publishes events, mutates ECS/physics state,
/// or owns the query system. Truncated snapshots retain the previous active set and emit no
/// inferred exits; ResetUVE() explicitly discards the baseline without fabricating transitions.
class AreaOverlapLifecycleTrackerUVE final {
public:
    [[nodiscard]] AreaOverlapLifecycleReportUVE UpdateUVE(
        const AreaOverlapQueryResultUVE& snapshot,
        std::size_t maximumTransitions = kMaximumAreaOverlapResultsUVE);

    void ResetUVE() noexcept;

    [[nodiscard]] std::size_t GetActiveCountUVE() const noexcept { return m_activePairs.size(); }

private:
    std::vector<AreaOverlapPairUVE> m_activePairs;
};

} // namespace UVE::Physics
