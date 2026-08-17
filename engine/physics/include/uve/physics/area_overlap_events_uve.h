// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/physics/area_overlap_system_uve.h"

namespace UVE::Physics {

/// Queued when an area/collider pair first appears in a complete overlap snapshot.
/// The pair is copied into the event; delivery is owned by IEventSystemUVE.
struct AreaOverlapEnteredEventUVE final {
    AreaOverlapPairUVE pair;

    [[nodiscard]] bool operator==(const AreaOverlapEnteredEventUVE&) const noexcept = default;
};

/// Queued when an area/collider pair disappears from a complete overlap snapshot.
/// The pair is the copied last-known overlap evidence; delivery is owned by IEventSystemUVE.
struct AreaOverlapExitedEventUVE final {
    AreaOverlapPairUVE pair;

    [[nodiscard]] bool operator==(const AreaOverlapExitedEventUVE&) const noexcept = default;
};

} // namespace UVE::Physics
