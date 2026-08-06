// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace UVE::Scene {

/// EntityUVE is a generational handle identifying an entity managed by an IEntityManagerUVE:
/// `index` names a slot, `generation` disambiguates that slot's current occupant from any
/// previously-destroyed occupant of the same slot (whose index may have been reused). This is
/// the standard, smallest-footprint way to catch use-after-destroy bugs cheaply — comparing an
/// old EntityUVE against a slot now occupied by a different entity fails the generation check
/// instead of silently aliasing unrelated data. EntityUVE itself is a data-only handle: it owns
/// no data or behavior, and carries no dependency on any specific IEntityManagerUVE instance.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct EntityUVE {
    std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t generation = 0;
};

/// The sentinel "no entity" value: an EntityUVE with the maximum possible index and generation
/// 0. No valid entity ever has this index, so comparing against kInvalidEntityUVE never
/// accidentally matches a real entity.
inline constexpr EntityUVE kInvalidEntityUVE{};

[[nodiscard]] constexpr bool operator==(const EntityUVE& lhs, const EntityUVE& rhs) noexcept {
    return lhs.index == rhs.index && lhs.generation == rhs.generation;
}

[[nodiscard]] constexpr bool operator!=(const EntityUVE& lhs, const EntityUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Scene

template <>
struct std::hash<UVE::Scene::EntityUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Scene::EntityUVE& entity) const noexcept {
        const std::uint64_t combined =
            (static_cast<std::uint64_t>(entity.index) << 32) | static_cast<std::uint64_t>(entity.generation);
        return std::hash<std::uint64_t>{}(combined);
    }
};
