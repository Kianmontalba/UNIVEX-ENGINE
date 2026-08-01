//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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
