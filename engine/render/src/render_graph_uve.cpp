// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_graph_uve.h"

#include <algorithm>

#include "uve/debug/logging_macros_uve.h"
#include "uve/render/i_command_buffer_uve.h"

namespace UVE::Render {

RenderGraphResourceHandleUVE RenderGraphUVE::ImportTextureUVE(TextureHandleUVE texture, std::string_view debugNameUVE) {
    if (texture == kInvalidTextureHandleUVE) {
        UVE_ERROR("RenderGraphUVE: cannot import an invalid texture handle");
        return {};
    }
    const auto index = static_cast<std::uint32_t>(m_resourceCount);
    if (m_resourceCount == m_resources.size()) {
        m_resources.push_back(ImportedResourceUVE{texture, std::string(debugNameUVE)});
    } else {
        ImportedResourceUVE& resource = m_resources[m_resourceCount];
        resource.texture = texture;
        resource.debugNameUVE = debugNameUVE;
    }
    ++m_resourceCount;
    return RenderGraphResourceHandleUVE{index, m_generation};
}

void RenderGraphUVE::AddPassUVE(RenderGraphPassDescUVE desc) {
    AddPassUVE(std::string_view{desc.debugNameUVE}, desc.resources, std::move(desc.recordCallbackUVE));
}

void RenderGraphUVE::AddPassUVE(std::string_view debugNameUVE,
                                std::span<const RenderGraphResourceUseUVE> resources,
                                std::function<void(ICommandBufferUVE&)> recordCallbackUVE) {
    if (m_passCount == m_passes.size()) {
        RenderGraphPassDescUVE pass;
        pass.debugNameUVE = debugNameUVE;
        pass.resources.assign(resources.begin(), resources.end());
        pass.recordCallbackUVE = std::move(recordCallbackUVE);
        m_passes.push_back(std::move(pass));
    } else {
        RenderGraphPassDescUVE& pass = m_passes[m_passCount];
        pass.debugNameUVE = debugNameUVE;
        pass.resources.clear();
        pass.resources.reserve(resources.size());
        pass.resources.insert(pass.resources.end(), resources.begin(), resources.end());
        pass.recordCallbackUVE = std::move(recordCallbackUVE);
    }
    ++m_passCount;
}

void RenderGraphUVE::ReserveUVE(const std::size_t resourceCapacity, const std::size_t passCapacity) {
    m_resources.reserve(resourceCapacity);
    m_passes.reserve(passCapacity);
}

bool RenderGraphUVE::ExecuteUVE(ICommandBufferUVE& commandBuffer) const {
    if (m_resourceCount > m_resources.size() || m_passCount > m_passes.size()) {
        UVE_ERROR("RenderGraphUVE: active storage counts are invalid");
        return false;
    }
    for (std::size_t passIndex = 0U; passIndex < m_passCount; ++passIndex) {
        const RenderGraphPassDescUVE& pass = m_passes[passIndex];
        if (pass.debugNameUVE.empty() || !pass.recordCallbackUVE) {
            UVE_ERROR("RenderGraphUVE: pass has no debug name or recording callback");
            return false;
        }
        for (const RenderGraphResourceUseUVE& use : pass.resources) {
            if (!use.resource.IsValidUVE() || use.resource.generation != m_generation ||
                use.resource.value >= m_resourceCount) {
                UVE_ERROR("RenderGraphUVE: pass '{}' references an unknown resource", pass.debugNameUVE);
                return false;
            }
        }
    }

    // The foundation has no transient allocation, barriers, or parallel queues. Declared usages
    // make dependencies reviewable while stable insertion order preserves the current renderer
    // command contract exactly.
    for (std::size_t passIndex = 0U; passIndex < m_passCount; ++passIndex) {
        m_passes[passIndex].recordCallbackUVE(commandBuffer);
    }
    return true;
}

void RenderGraphUVE::ClearUVE() noexcept {
    for (std::size_t passIndex = 0U; passIndex < m_passCount; ++passIndex) {
        RenderGraphPassDescUVE& pass = m_passes[passIndex];
        pass.debugNameUVE.clear();
        pass.resources.clear();
        pass.recordCallbackUVE = nullptr;
    }
    for (std::size_t resourceIndex = 0U; resourceIndex < m_resourceCount; ++resourceIndex) {
        ImportedResourceUVE& resource = m_resources[resourceIndex];
        resource.texture = kInvalidTextureHandleUVE;
        resource.debugNameUVE.clear();
    }
    m_passCount = 0U;
    m_resourceCount = 0U;
    ++m_generation;
}

std::size_t RenderGraphUVE::GetPassCountUVE() const noexcept {
    return m_passCount;
}

} // namespace UVE::Render
