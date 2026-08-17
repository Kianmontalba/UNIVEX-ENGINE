// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <vector>

#include "uve/render/render_item_uve.h"

namespace UVE::Render {

/// A sorted, bucketed set of draw items for one frame — the spec's `RenderQueueUVE` ("sortable
/// render command buffer", Part 7.2). Opaque items are sorted front-to-back (ascending
/// sortDepth) to maximize early-z rejection; transparent items are sorted back-to-front
/// (descending sortDepth) for correct alpha blending. Populated by
/// MeshRendererUVE::ExtractRenderQueueUVE(); SortUVE() must be called before a future
/// Renderer3DUVE consumes it.
/// Thread-safety: value type; not thread-safe to mutate concurrently.
struct RenderQueueUVE {
    std::vector<RenderItemUVE> opaqueItems;
    std::vector<RenderItemUVE> transparentItems;

    /// Per-frame asset-resolution facts copied during ECS extraction. Counts are references/handles,
    /// not fallback resources or ownership transfers; they explain why eligible entities were not
    /// emitted into either draw-item bucket.
    std::size_t invalidAssetReferences = 0U;
    std::size_t pendingAssetLoads = 0U;
    std::size_t failedAssetLoads = 0U;

    /// Sorts opaqueItems ascending by sortDepth (front-to-back) and transparentItems descending
    /// by sortDepth (back-to-front), in place.
    void SortUVE();
};

} // namespace UVE::Render
