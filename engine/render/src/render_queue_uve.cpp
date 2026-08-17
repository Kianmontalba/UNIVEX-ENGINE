// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/render_queue_uve.h"

#include <algorithm>

namespace UVE::Render {

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
    particleItems.insert(particleItems.end(), snapshot.items.begin(), snapshot.items.end());
    particleItemsTruncated = particleItemsTruncated || snapshot.truncated;
}

} // namespace UVE::Render
