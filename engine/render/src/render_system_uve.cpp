//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/render_system_uve.h"

#include <utility>

#include "uve/debug/assert_uve.h"

namespace UVE::Render {

struct RenderSystemUVE::ImplUVE {
    IRenderDeviceUVE& renderDevice;
    std::unique_ptr<ICommandBufferUVE> currentFrameCommandBuffer;
    std::uint64_t frameIndex = 0;
    bool frameActive = false;
};

RenderSystemUVE::RenderSystemUVE(IRenderDeviceUVE& renderDevice) : m_impl(std::make_unique<ImplUVE>(renderDevice)) {}

RenderSystemUVE::~RenderSystemUVE() = default;

void RenderSystemUVE::BeginFrameUVE() {
    UVE_ASSERT(!m_impl->frameActive);
    m_impl->currentFrameCommandBuffer = m_impl->renderDevice.CreateCommandBufferUVE();
    m_impl->frameActive = true;
}

ICommandBufferUVE& RenderSystemUVE::GetFrameCommandBufferUVE() {
    UVE_ASSERT(m_impl->frameActive);
    return *m_impl->currentFrameCommandBuffer;
}

void RenderSystemUVE::EndFrameUVE() {
    UVE_ASSERT(m_impl->frameActive);
    m_impl->renderDevice.SubmitUVE(std::move(m_impl->currentFrameCommandBuffer));
    ++m_impl->frameIndex;
    m_impl->frameActive = false;
}

std::uint64_t RenderSystemUVE::GetFrameIndexUVE() const noexcept {
    return m_impl->frameIndex;
}

} // namespace UVE::Render
