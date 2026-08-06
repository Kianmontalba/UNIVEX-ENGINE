// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_queue_uve.h"

#include <algorithm>

namespace UVE::Render {

void RenderQueueUVE::SortUVE() {
    std::sort(opaqueItems.begin(), opaqueItems.end(),
              [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) { return lhs.sortDepth < rhs.sortDepth; });
    std::sort(transparentItems.begin(), transparentItems.end(),
              [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) { return lhs.sortDepth > rhs.sortDepth; });
}

} // namespace UVE::Render
