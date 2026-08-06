// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "null_command_buffer_uve.h"

#include <string>

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

void NullCommandBufferUVE::SetUniformFloatUVE(std::string_view name, float value) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(SetUniformFloatCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformIntUVE(std::string_view name, std::int32_t value) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(SetUniformIntCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformBoolUVE(std::string_view name, bool value) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(SetUniformBoolCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(SetUniformVector3CommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) {
    UVE_ASSERT(m_insideRenderPass);
    m_commands.emplace_back(SetUniformMatrix4x4CommandUVE{std::string(name), value});
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
