//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "null_command_buffer_uve.h"

#include "uve/debug/assert_uve.h"

namespace UVE::Render {

void NullCommandBufferUVE::BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) {
    UVE_ASSERT(!m_insideRenderPass);
    m_insideRenderPass = true;
    m_commands.emplace_back(BeginRenderPassCommandUVE{renderPassDesc});
}

void NullCommandBufferUVE::EndRenderPassUVE() {
    UVE_ASSERT(m_insideRenderPass);
    m_insideRenderPass = false;
    m_commands.emplace_back(EndRenderPassCommandUVE{});
}

void NullCommandBufferUVE::BindPipelineUVE(PipelineHandleUVE pipeline) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(BindPipelineCommandUVE{pipeline});
}

void NullCommandBufferUVE::BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(BindVertexBufferCommandUVE{buffer, slot});
}

void NullCommandBufferUVE::BindIndexBufferUVE(BufferHandleUVE buffer) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(BindIndexBufferCommandUVE{buffer});
}

void NullCommandBufferUVE::BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(BindTextureCommandUVE{texture, slot});
}

void NullCommandBufferUVE::BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(BindUniformBufferCommandUVE{buffer, slot});
}

void NullCommandBufferUVE::DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(DrawIndexedCommandUVE{indexCount, instanceCount});
}

void NullCommandBufferUVE::DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(DrawCommandUVE{vertexCount, instanceCount});
}

const std::vector<RecordedCommandUVE>& NullCommandBufferUVE::GetRecordedCommandsUVE() const noexcept {
    return m_commands;
}

} // namespace UVE::Render
