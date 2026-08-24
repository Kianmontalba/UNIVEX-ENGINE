// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {

inline constexpr std::size_t kMaximumAreaOverlapResultsUVE = 4096U;
inline constexpr std::size_t kMaximumAreaOverlapQueryAreasUVE = 4096U;

struct AreaOverlapPairUVE final {
    Scene::EntityUVE area;
    Scene::EntityUVE other;
    float penetrationDepth = 0.0F;

    [[nodiscard]] bool operator==(const AreaOverlapPairUVE&) const noexcept = default;
};

struct AreaOverlapQueryResultUVE final {
    std::size_t inspectedAreas = 0U;
    std::size_t inspectedColliders = 0U;
    bool truncated = false;
    std::vector<AreaOverlapPairUVE> overlaps;

    [[nodiscard]] bool IsTruncatedUVE() const noexcept { return truncated; }
};

/// Performs a bounded, read-only overlap query for AreaComponentUVE volumes against
/// ColliderComponentUVE volumes. Known Box/Sphere/Capsule targets use exact oriented-box, sphere,
/// and capsule penetration helpers; unknown shape values retain conservative AABB penetration.
/// Results are copied in deterministic entity iteration order and require symmetric layer/mask
/// acceptance. Areas never enter CollisionSystemUVE or PhysicsSystemUVE resolution, and this seam
/// publishes no global events or owns entity state.
class AreaOverlapSystemUVE final {
public:
    [[nodiscard]] static AreaOverlapQueryResultUVE QueryUVE(
        Scene::IEntityManagerUVE& entityManager,
        std::size_t maximumResults = kMaximumAreaOverlapResultsUVE);
};

} // namespace UVE::Physics
