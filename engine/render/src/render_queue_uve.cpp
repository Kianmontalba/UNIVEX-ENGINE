// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/render_queue_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Render {

namespace {

[[nodiscard]] bool RenderItemTieBreakLessUVE(const RenderItemUVE& lhs, const RenderItemUVE& rhs) noexcept {
    const std::uint64_t lhsMaterialGuid = lhs.materialHandle.GetGuidUVE().value;
    const std::uint64_t rhsMaterialGuid = rhs.materialHandle.GetGuidUVE().value;
    if (lhsMaterialGuid != rhsMaterialGuid) {
        return lhsMaterialGuid < rhsMaterialGuid;
    }
    return lhs.meshHandle.GetGuidUVE().value < rhs.meshHandle.GetGuidUVE().value;
}

} // namespace

void RenderQueueUVE::ClearUVE() noexcept {
    opaqueItems.clear();
    transparentItems.clear();
    particleItems.clear();
    particleItemsTruncated = false;
    invalidAssetReferences = 0U;
    pendingAssetLoads = 0U;
    failedAssetLoads = 0U;
    invalidRenderEligibility = 0U;
}

void RenderQueueUVE::ReserveUVE(const std::size_t opaqueCapacity, const std::size_t transparentCapacity,
                                const std::size_t particleCapacity) {
    opaqueItems.reserve(opaqueCapacity);
    transparentItems.reserve(transparentCapacity);
    particleItems.reserve(particleCapacity);
}

void RenderQueueUVE::SortUVE() {
    std::sort(opaqueItems.begin(), opaqueItems.end(), [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) {
        const bool lhsFinite = std::isfinite(lhs.sortDepth);
        const bool rhsFinite = std::isfinite(rhs.sortDepth);
        if (lhsFinite != rhsFinite) {
            return lhsFinite;
        }
        if (lhsFinite && lhs.sortDepth != rhs.sortDepth) {
            return lhs.sortDepth < rhs.sortDepth;
        }
        return RenderItemTieBreakLessUVE(lhs, rhs);
    });
    std::sort(transparentItems.begin(), transparentItems.end(), [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) {
        const bool lhsFinite = std::isfinite(lhs.sortDepth);
        const bool rhsFinite = std::isfinite(rhs.sortDepth);
        if (lhsFinite != rhsFinite) {
            return lhsFinite;
        }
        if (lhsFinite && lhs.sortDepth != rhs.sortDepth) {
            return lhs.sortDepth > rhs.sortDepth;
        }
        return RenderItemTieBreakLessUVE(lhs, rhs);
    });
    std::sort(particleItems.begin(), particleItems.end(), [](const ParticleRenderItemUVE& lhs,
                                                             const ParticleRenderItemUVE& rhs) {
        if (lhs.sortDepth != rhs.sortDepth) {
            return lhs.sortDepth > rhs.sortDepth;
        }
        if (lhs.entity.index != rhs.entity.index) {
            return lhs.entity.index < rhs.entity.index;
        }
        if (lhs.entity.generation != rhs.entity.generation) {
            return lhs.entity.generation < rhs.entity.generation;
        }
        return lhs.sequence < rhs.sequence;
    });
}

void RenderQueueUVE::AppendParticleSnapshotUVE(const ParticleRenderSnapshotUVE& snapshot) {
    if (!ValidateParticleRenderSnapshotUVE(snapshot)) {
        particleItemsTruncated = true;
        return;
    }
    particleItems.insert(particleItems.end(), snapshot.items.begin(), snapshot.items.end());
    particleItemsTruncated = particleItemsTruncated || snapshot.truncated;
}

} // namespace UVE::Render
