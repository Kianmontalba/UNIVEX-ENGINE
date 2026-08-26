// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_graph_uve.h"

#include <algorithm>

#include "uve/debug/logging_macros_uve.h"
#include "uve/render/i_command_buffer_uve.h"

namespace UVE::Render {

RenderGraphResourceHandleUVE RenderGraphUVE::ImportTextureUVE(TextureHandleUVE texture, std::string debugNameUVE) {
    if (texture == kInvalidTextureHandleUVE) {
        UVE_ERROR("RenderGraphUVE: cannot import an invalid texture handle");
        return {};
    }
    const auto index = static_cast<std::uint32_t>(m_resources.size());
    m_resources.push_back(ImportedResourceUVE{texture, std::move(debugNameUVE)});
    return RenderGraphResourceHandleUVE{index};
}

void RenderGraphUVE::AddPassUVE(RenderGraphPassDescUVE desc) {
    m_passes.push_back(std::move(desc));
}

void RenderGraphUVE::ReserveUVE(const std::size_t resourceCapacity, const std::size_t passCapacity) {
    m_resources.reserve(resourceCapacity);
    m_passes.reserve(passCapacity);
}

bool RenderGraphUVE::ExecuteUVE(ICommandBufferUVE& commandBuffer) const {
    for (const RenderGraphPassDescUVE& pass : m_passes) {
        if (pass.debugNameUVE.empty() || !pass.recordCallbackUVE) {
            UVE_ERROR("RenderGraphUVE: pass has no debug name or recording callback");
            return false;
        }
        for (const RenderGraphResourceUseUVE& use : pass.resources) {
            if (!use.resource.IsValidUVE() || use.resource.value >= m_resources.size()) {
                UVE_ERROR("RenderGraphUVE: pass '{}' references an unknown resource", pass.debugNameUVE);
                return false;
            }
        }
    }

    // The foundation has no transient allocation, barriers, or parallel queues. Declared usages
    // make dependencies reviewable while stable insertion order preserves the current renderer
    // command contract exactly.
    for (const RenderGraphPassDescUVE& pass : m_passes) {
        pass.recordCallbackUVE(commandBuffer);
    }
    return true;
}

void RenderGraphUVE::ClearUVE() noexcept {
    m_passes.clear();
    m_resources.clear();
}

std::size_t RenderGraphUVE::GetPassCountUVE() const noexcept {
    return m_passes.size();
}

} // namespace UVE::Render
