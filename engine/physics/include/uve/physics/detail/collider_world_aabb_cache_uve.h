// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics::Detail {

/// One entity's world-space AABB and collision layer/mask, computed once and reused across every
/// query against it. NOT a stable public contract — this is an internal implementation detail
/// shared by CollisionSystemUVE, its transient BVH builder, and RaycastSystemUVE (Increment 16)
/// so their shape extraction and iteration inputs cannot independently drift; nothing outside
/// engine/physics should depend on this type or function.
struct ColliderWorldAabbUVE {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
    std::uint32_t collisionLayer;
    std::uint32_t collisionMask;
};

enum class DynamicColliderWorldAabbUpdateCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidEntity,
    UnknownProxy,
    StaleGeneration,
    InvalidAabb,
};

struct DynamicColliderWorldAabbUpdateResultUVE final {
    DynamicColliderWorldAabbUpdateCodeUVE code = DynamicColliderWorldAabbUpdateCodeUVE::UnknownProxy;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return code == DynamicColliderWorldAabbUpdateCodeUVE::Applied;
    }
};

/// Owns a copied static median-split topology and supports deterministic refit-only updates of
/// individual collider AABBs. Cache indices and leaf membership never change during updates, so
/// callers retain the original encounter ordering while moved bounds update broad-phase pruning.
class DynamicAabbBvhUVE final {
public:
    static constexpr std::size_t kMaximumProxiesUVE = 4096U;

    explicit DynamicAabbBvhUVE(std::vector<ColliderWorldAabbUVE> colliders);

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return m_isValid;
    }

    [[nodiscard]] const std::vector<ColliderWorldAabbUVE>& GetCollidersUVE() const noexcept {
        return m_colliders;
    }

    [[nodiscard]] DynamicColliderWorldAabbUpdateResultUVE UpdateAabbUVE(
        Scene::EntityUVE entity, const Math::AabbUVE& worldAabb) noexcept;

    [[nodiscard]] bool QueryUVE(const Math::AabbUVE& bounds, std::vector<std::size_t>& candidates) const;

private:
    static constexpr std::size_t kInvalidNodeIndexUVE = static_cast<std::size_t>(-1);

    struct NodeUVE final {
        Math::AabbUVE bounds;
        std::size_t begin = 0U;
        std::size_t end = 0U;
        std::size_t left = kInvalidNodeIndexUVE;
        std::size_t right = kInvalidNodeIndexUVE;
        std::size_t parent = kInvalidNodeIndexUVE;
        bool isLeaf = false;
    };

    [[nodiscard]] std::size_t BuildNodeUVE(std::size_t begin, std::size_t end,
                                            std::size_t parent);
    void RefitFromLeafUVE(std::size_t leafNodeIndex) noexcept;

    std::vector<ColliderWorldAabbUVE> m_colliders;
    std::vector<std::size_t> m_indices;
    std::vector<std::size_t> m_leafNodeByCacheIndex;
    std::vector<NodeUVE> m_nodes;
    std::size_t m_root = kInvalidNodeIndexUVE;
    bool m_isValid = true;
};

/// Builds a flat cache of every entity with both WorldTransformComponentUVE and
/// ColliderComponentUVE via one ForEachUVE pass. CollisionSystemUVE builds its transient static
/// BVH from this snapshot, while RaycastSystemUVE retains its deterministic linear query path.
[[nodiscard]] std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(
    Scene::IEntityManagerUVE& entityManager);

} // namespace UVE::Physics::Detail
