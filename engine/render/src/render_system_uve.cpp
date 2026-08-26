// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_system_uve.h"

#include <exception>
#include <utility>

#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render {

struct RenderSystemUVE::ImplUVE {
    IRenderDeviceUVE& renderDevice;
    std::unique_ptr<ICommandBufferUVE> currentFrameCommandBuffer;
    std::unique_ptr<ICommandBufferUVE> invalidAccessCommandBuffer;
    std::uint64_t frameIndex = 0;
    bool frameActive = false;
};

RenderSystemUVE::RenderSystemUVE(IRenderDeviceUVE& renderDevice) : m_impl(std::make_unique<ImplUVE>(renderDevice)) {}

RenderSystemUVE::~RenderSystemUVE() = default;

void RenderSystemUVE::BeginFrameUVE() {
    UVE_ASSERT(!m_impl->frameActive);
    if (m_impl->frameActive) {
        UVE_ERROR("RenderSystemUVE: BeginFrameUVE called while a frame is already active");
        return;
    }
    std::unique_ptr<ICommandBufferUVE> commandBuffer = m_impl->renderDevice.CreateCommandBufferUVE();
    if (commandBuffer == nullptr) {
        UVE_ERROR("RenderSystemUVE: CreateCommandBufferUVE returned null");
        return;
    }
    m_impl->currentFrameCommandBuffer = std::move(commandBuffer);
    m_impl->frameActive = true;
}

ICommandBufferUVE& RenderSystemUVE::GetFrameCommandBufferUVE() {
    UVE_ASSERT(m_impl->frameActive);
    if (m_impl->frameActive && m_impl->currentFrameCommandBuffer != nullptr) {
        return *m_impl->currentFrameCommandBuffer;
    }
    UVE_ERROR("RenderSystemUVE: GetFrameCommandBufferUVE called without an active frame");
    if (m_impl->invalidAccessCommandBuffer == nullptr) {
        m_impl->invalidAccessCommandBuffer = m_impl->renderDevice.CreateCommandBufferUVE();
    }
    UVE_ASSERT(m_impl->invalidAccessCommandBuffer != nullptr);
    if (m_impl->invalidAccessCommandBuffer == nullptr) {
        UVE_FATAL("RenderSystemUVE: unable to create a safe invalid-access command buffer");
        std::terminate();
    }
    return *m_impl->invalidAccessCommandBuffer;
}

void RenderSystemUVE::EndFrameUVE() {
    UVE_ASSERT(m_impl->frameActive);
    if (!m_impl->frameActive || m_impl->currentFrameCommandBuffer == nullptr) {
        UVE_ERROR("RenderSystemUVE: EndFrameUVE called without an active frame");
        return;
    }
    m_impl->renderDevice.SubmitUVE(std::move(m_impl->currentFrameCommandBuffer));
    ++m_impl->frameIndex;
    m_impl->frameActive = false;
}

std::uint64_t RenderSystemUVE::GetFrameIndexUVE() const noexcept {
    return m_impl->frameIndex;
}

} // namespace UVE::Render
