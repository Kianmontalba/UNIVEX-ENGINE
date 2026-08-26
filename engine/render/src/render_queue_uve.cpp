// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/render_queue_uve.h"

#include <algorithm>

namespace UVE::Render {

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
    std::sort(opaqueItems.begin(), opaqueItems.end(),
              [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) { return lhs.sortDepth < rhs.sortDepth; });
    std::sort(transparentItems.begin(), transparentItems.end(),
              [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) { return lhs.sortDepth > rhs.sortDepth; });
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
