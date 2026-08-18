// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>

namespace UVE::Physics::Detail {
namespace {

constexpr std::size_t kBvhLeafSizeUVE = 4U;
constexpr std::size_t kBvhMaximumTraversalStackUVE = 128U;

[[nodiscard]] bool IsValidAabbUVE(const Math::AabbUVE& aabb) noexcept {
    return std::isfinite(aabb.min.x) && std::isfinite(aabb.min.y) && std::isfinite(aabb.min.z) &&
           std::isfinite(aabb.max.x) && std::isfinite(aabb.max.y) && std::isfinite(aabb.max.z) &&
           aabb.min.x <= aabb.max.x && aabb.min.y <= aabb.max.y && aabb.min.z <= aabb.max.z;
}

} // namespace

DynamicAabbBvhUVE::DynamicAabbBvhUVE(std::vector<ColliderWorldAabbUVE> colliders)
    : m_colliders(std::move(colliders)), m_indices(m_colliders.size()),
      m_leafNodeByCacheIndex(m_colliders.size(), kInvalidNodeIndexUVE) {
    if (m_colliders.size() > kMaximumProxiesUVE ||
        !std::all_of(m_colliders.begin(), m_colliders.end(), [](const ColliderWorldAabbUVE& collider) {
            return collider.entity != Scene::kInvalidEntityUVE && IsValidAabbUVE(collider.worldAabb);
        })) {
        m_isValid = false;
        return;
    }
    for (std::size_t firstIndex = 0U; firstIndex < m_colliders.size(); ++firstIndex) {
        for (std::size_t secondIndex = firstIndex + 1U; secondIndex < m_colliders.size(); ++secondIndex) {
            if (m_colliders[firstIndex].entity == m_colliders[secondIndex].entity) {
                m_isValid = false;
                return;
            }
        }
    }
    std::iota(m_indices.begin(), m_indices.end(), 0U);
    if (!m_indices.empty()) {
        m_nodes.reserve(m_indices.size() * 2U);
        m_root = BuildNodeUVE(0U, m_indices.size(), kInvalidNodeIndexUVE);
    }
}

std::size_t DynamicAabbBvhUVE::BuildNodeUVE(const std::size_t begin, const std::size_t end,
                                            const std::size_t parent) {
    const std::size_t nodeIndex = m_nodes.size();
    m_nodes.push_back(NodeUVE{});

    Math::AabbUVE bounds = m_colliders[m_indices[begin]].worldAabb;
    for (std::size_t index = begin + 1U; index < end; ++index) {
        bounds = bounds.UnionUVE(m_colliders[m_indices[index]].worldAabb);
    }

    NodeUVE& node = m_nodes[nodeIndex];
    node.bounds = bounds;
    node.begin = begin;
    node.end = end;
    node.parent = parent;
    if (end - begin <= kBvhLeafSizeUVE) {
        node.isLeaf = true;
        for (std::size_t index = begin; index < end; ++index) {
            m_leafNodeByCacheIndex[m_indices[index]] = nodeIndex;
        }
        return nodeIndex;
    }

    const Math::Vector3UVE extents = bounds.GetExtentsUVE();
    std::size_t splitAxis = 0U;
    if (extents.y > extents.x && extents.y >= extents.z) {
        splitAxis = 1U;
    } else if (extents.z > extents.x && extents.z > extents.y) {
        splitAxis = 2U;
    }
    const auto CenterOnAxisUVE = [&](const std::size_t cacheIndex) noexcept {
        const Math::Vector3UVE center = m_colliders[cacheIndex].worldAabb.GetCenterUVE();
        return splitAxis == 0U ? center.x : (splitAxis == 1U ? center.y : center.z);
    };
    std::stable_sort(m_indices.begin() + static_cast<std::ptrdiff_t>(begin),
                     m_indices.begin() + static_cast<std::ptrdiff_t>(end),
                     [&](const std::size_t lhs, const std::size_t rhs) {
                         const float leftCenter = CenterOnAxisUVE(lhs);
                         const float rightCenter = CenterOnAxisUVE(rhs);
                         return leftCenter < rightCenter ||
                                (leftCenter == rightCenter && lhs < rhs);
                     });

    const std::size_t middle = begin + (end - begin) / 2U;
    const std::size_t left = BuildNodeUVE(begin, middle, nodeIndex);
    const std::size_t right = BuildNodeUVE(middle, end, nodeIndex);
    m_nodes[nodeIndex].left = left;
    m_nodes[nodeIndex].right = right;
    return nodeIndex;
}

void DynamicAabbBvhUVE::RefitFromLeafUVE(const std::size_t leafNodeIndex) noexcept {
    std::size_t nodeIndex = leafNodeIndex;
    while (nodeIndex != kInvalidNodeIndexUVE) {
        NodeUVE& node = m_nodes[nodeIndex];
        if (node.isLeaf) {
            node.bounds = m_colliders[m_indices[node.begin]].worldAabb;
            for (std::size_t index = node.begin + 1U; index < node.end; ++index) {
                node.bounds = node.bounds.UnionUVE(m_colliders[m_indices[index]].worldAabb);
            }
        } else {
            node.bounds = m_nodes[node.left].bounds.UnionUVE(m_nodes[node.right].bounds);
        }
        nodeIndex = node.parent;
    }
}

DynamicColliderWorldAabbUpdateResultUVE DynamicAabbBvhUVE::UpdateAabbUVE(
    const Scene::EntityUVE entity, const Math::AabbUVE& worldAabb) noexcept {
    if (entity == Scene::kInvalidEntityUVE) {
        return {DynamicColliderWorldAabbUpdateCodeUVE::InvalidEntity,
                "Dynamic BVH update rejected because the entity handle is invalid."};
    }
    if (!IsValidAabbUVE(worldAabb)) {
        return {DynamicColliderWorldAabbUpdateCodeUVE::InvalidAabb,
                "Dynamic BVH update rejected because the world AABB is invalid."};
    }
    if (!m_isValid) {
        return {DynamicColliderWorldAabbUpdateCodeUVE::UnknownProxy,
                "Dynamic BVH update rejected because the copied proxy topology is invalid."};
    }

    const auto iterator = std::find_if(m_colliders.begin(), m_colliders.end(),
                                       [entity](const ColliderWorldAabbUVE& collider) {
                                           return collider.entity == entity;
                                       });
    if (iterator == m_colliders.end()) {
        const bool sameIndex = std::any_of(m_colliders.begin(), m_colliders.end(), [entity](const auto& collider) {
            return collider.entity.index == entity.index;
        });
        return {sameIndex ? DynamicColliderWorldAabbUpdateCodeUVE::StaleGeneration
                          : DynamicColliderWorldAabbUpdateCodeUVE::UnknownProxy,
                sameIndex ? "Dynamic BVH update rejected because the entity generation is stale."
                          : "Dynamic BVH update rejected because the entity is not a BVH proxy."};
    }

    const std::size_t cacheIndex = static_cast<std::size_t>(iterator - m_colliders.begin());
    iterator->worldAabb = worldAabb;
    RefitFromLeafUVE(m_leafNodeByCacheIndex[cacheIndex]);
    return {DynamicColliderWorldAabbUpdateCodeUVE::Applied, "Dynamic BVH AABB update applied."};
}

bool DynamicAabbBvhUVE::QueryUVE(const Math::AabbUVE& bounds,
                                 std::vector<std::size_t>& candidates) const {
    candidates.clear();
    if (!m_isValid || !IsValidAabbUVE(bounds)) {
        return false;
    }
    if (m_indices.empty()) {
        return true;
    }

    std::array<std::size_t, kBvhMaximumTraversalStackUVE> stack{};
    std::size_t stackSize = 0U;
    stack[stackSize++] = m_root;
    while (stackSize > 0U) {
        const NodeUVE& node = m_nodes[stack[--stackSize]];
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

} // namespace UVE::Physics::Detail

// EOF
