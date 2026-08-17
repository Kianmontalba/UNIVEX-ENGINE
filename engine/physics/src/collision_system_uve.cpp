// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/collision_system_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <optional>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

namespace UVE::Physics {
namespace {

constexpr std::size_t kBvhLeafSizeUVE = 4U;
constexpr std::size_t kBvhMaximumTraversalStackUVE = 128U;

[[nodiscard]] bool IsValidAabbUVE(const Math::AabbUVE& aabb) noexcept {
    return std::isfinite(aabb.min.x) && std::isfinite(aabb.min.y) && std::isfinite(aabb.min.z) &&
           std::isfinite(aabb.max.x) && std::isfinite(aabb.max.y) && std::isfinite(aabb.max.z) &&
           aabb.min.x <= aabb.max.x && aabb.min.y <= aabb.max.y && aabb.min.z <= aabb.max.z;
}

struct BvhNodeUVE final {
    Math::AabbUVE bounds;
    std::size_t begin = 0U;
    std::size_t end = 0U;
    std::size_t left = 0U;
    std::size_t right = 0U;
    bool isLeaf = false;
};

class StaticAabbBvhUVE final {
public:
    explicit StaticAabbBvhUVE(const std::vector<Detail::ColliderWorldAabbUVE>& colliders)
        : m_colliders(colliders), m_indices(colliders.size()) {
        m_isValid = std::all_of(colliders.begin(), colliders.end(), [](const auto& collider) {
            return IsValidAabbUVE(collider.worldAabb);
        });
        if (!m_isValid) {
            return;
        }
        std::iota(m_indices.begin(), m_indices.end(), 0U);
        if (!m_indices.empty()) {
            m_nodes.reserve(m_indices.size() * 2U);
            m_root = BuildNodeUVE(0U, m_indices.size());
        }
    }

    [[nodiscard]] bool QueryUVE(const Math::AabbUVE& bounds, std::vector<std::size_t>& candidates) const {
        candidates.clear();
        if (!m_isValid) {
            return false;
        }
        if (m_indices.empty()) {
            return true;
        }

        std::array<std::size_t, kBvhMaximumTraversalStackUVE> stack{};
        std::size_t stackSize = 0U;
        stack[stackSize++] = m_root;
        while (stackSize > 0U) {
            const BvhNodeUVE& node = m_nodes[stack[--stackSize]];
            if (!bounds.IntersectsUVE(node.bounds)) {
                continue;
            }
            if (node.isLeaf) {
                candidates.insert(candidates.end(), m_indices.begin() + static_cast<std::ptrdiff_t>(node.begin),
                                  m_indices.begin() + static_cast<std::ptrdiff_t>(node.end));
                continue;
            }
            if (stackSize + 2U > stack.size()) {
                return false;
            }
            stack[stackSize++] = node.right;
            stack[stackSize++] = node.left;
        }
        return true;
    }

private:
    [[nodiscard]] std::size_t BuildNodeUVE(std::size_t begin, std::size_t end) {
        const std::size_t nodeIndex = m_nodes.size();
        m_nodes.push_back(BvhNodeUVE{});

        Math::AabbUVE bounds = m_colliders[m_indices[begin]].worldAabb;
        for (std::size_t index = begin + 1U; index < end; ++index) {
            bounds = bounds.UnionUVE(m_colliders[m_indices[index]].worldAabb);
        }

        m_nodes[nodeIndex].bounds = bounds;
        m_nodes[nodeIndex].begin = begin;
        m_nodes[nodeIndex].end = end;
        if (end - begin <= kBvhLeafSizeUVE) {
            m_nodes[nodeIndex].isLeaf = true;
            return nodeIndex;
        }

        const Math::Vector3UVE extents = bounds.GetExtentsUVE();
        std::size_t splitAxis = 0U;
        if (extents.y > extents.x && extents.y >= extents.z) {
            splitAxis = 1U;
        } else if (extents.z > extents.x && extents.z > extents.y) {
            splitAxis = 2U;
        }
        const auto CenterOnAxisUVE = [&](std::size_t cacheIndex) noexcept {
            const Math::Vector3UVE center = m_colliders[cacheIndex].worldAabb.GetCenterUVE();
            return splitAxis == 0U ? center.x : (splitAxis == 1U ? center.y : center.z);
        };
        std::stable_sort(m_indices.begin() + static_cast<std::ptrdiff_t>(begin),
                         m_indices.begin() + static_cast<std::ptrdiff_t>(end),
                         [&](std::size_t lhs, std::size_t rhs) {
                             const float leftCenter = CenterOnAxisUVE(lhs);
                             const float rightCenter = CenterOnAxisUVE(rhs);
                             return leftCenter < rightCenter ||
                                    (leftCenter == rightCenter && lhs < rhs);
                         });

        const std::size_t middle = begin + (end - begin) / 2U;
        const std::size_t left = BuildNodeUVE(begin, middle);
        const std::size_t right = BuildNodeUVE(middle, end);
        m_nodes[nodeIndex].left = left;
        m_nodes[nodeIndex].right = right;
        return nodeIndex;
    }

    const std::vector<Detail::ColliderWorldAabbUVE>& m_colliders;
    std::vector<std::size_t> m_indices;
    std::vector<BvhNodeUVE> m_nodes;
    std::size_t m_root = 0U;
    bool m_isValid = true;
};

[[nodiscard]] bool LayersAcceptPairUVE(const Detail::ColliderWorldAabbUVE& first,
                                       const Detail::ColliderWorldAabbUVE& second) noexcept {
    return (first.collisionMask & second.collisionLayer) != 0U &&
           (second.collisionMask & first.collisionLayer) != 0U;
}

void AppendPairIfOverlappingUVE(const std::vector<Detail::ColliderWorldAabbUVE>& colliders, std::size_t firstIndex,
                                std::size_t secondIndex, std::vector<CollisionPairUVE>& pairs) {
    const Detail::ColliderWorldAabbUVE& first = colliders[firstIndex];
    const Detail::ColliderWorldAabbUVE& second = colliders[secondIndex];
    if (!LayersAcceptPairUVE(first, second)) {
        return;
    }

    const std::optional<Math::PenetrationUVE> penetration =
        Math::ComputePenetrationUVE(first.worldAabb, second.worldAabb);
    if (penetration.has_value()) {
        pairs.push_back(CollisionPairUVE{first.entity, second.entity, penetration->axis, penetration->depth});
    }
}

} // namespace

std::vector<CollisionPairUVE> CollisionSystemUVE::DetectCollisionsUVE(Scene::IEntityManagerUVE& entityManager) const {
    const std::vector<Detail::ColliderWorldAabbUVE> colliders = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    std::vector<CollisionPairUVE> pairs;
    StaticAabbBvhUVE bvh(colliders);
    std::vector<std::size_t> candidates;
    for (std::size_t firstIndex = 0U; firstIndex < colliders.size(); ++firstIndex) {
        if (!bvh.QueryUVE(colliders[firstIndex].worldAabb, candidates)) {
            for (std::size_t secondIndex = firstIndex + 1U; secondIndex < colliders.size(); ++secondIndex) {
                AppendPairIfOverlappingUVE(colliders, firstIndex, secondIndex, pairs);
            }
            continue;
        }

        std::sort(candidates.begin(), candidates.end());
        for (const std::size_t secondIndex : candidates) {
            if (secondIndex > firstIndex) {
                AppendPairIfOverlappingUVE(colliders, firstIndex, secondIndex, pairs);
            }
        }
    }
    return pairs;
}

} // namespace UVE::Physics
