// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "null_command_buffer_uve.h"

#include <string>

#include "uve/debug/assert_uve.h"

namespace UVE::Render {

namespace {

[[nodiscard]] bool RequireInsideRenderPassUVE(bool insideRenderPass, std::string_view operation) noexcept {
    UVE_ASSERT(insideRenderPass);
    if (!insideRenderPass) {
        UVE_ERROR("NullCommandBufferUVE: {} must be called inside a render pass", operation);
        return false;
    }
    return true;
}

[[nodiscard]] bool RequireOutsideRenderPassUVE(bool insideRenderPass) noexcept {
    UVE_ASSERT(!insideRenderPass);
    if (insideRenderPass) {
        UVE_ERROR("NullCommandBufferUVE: BeginRenderPassUVE does not support nested render passes");
        return false;
    }
    return true;
}

} // namespace

void NullCommandBufferUVE::BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) {
    if (!RequireOutsideRenderPassUVE(m_insideRenderPass)) {
        return;
    }
    if (!IsLoadOpValidUVE(renderPassDesc.colorLoadOp) || !IsLoadOpValidUVE(renderPassDesc.depthLoadOp)) {
        UVE_ERROR("NullCommandBufferUVE: BeginRenderPassUVE received an unknown load operation");
        return;
    }
    m_insideRenderPass = true;
    m_commands.emplace_back(BeginRenderPassCommandUVE{renderPassDesc});
}

void NullCommandBufferUVE::EndRenderPassUVE() {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "EndRenderPassUVE")) {
        return;
    }
    m_insideRenderPass = false;
    m_commands.emplace_back(EndRenderPassCommandUVE{});
}

void NullCommandBufferUVE::BindPipelineUVE(PipelineHandleUVE pipeline) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindPipelineUVE")) {
        return;
    }
    m_commands.emplace_back(BindPipelineCommandUVE{pipeline});
}

void NullCommandBufferUVE::BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindVertexBufferUVE")) {
        return;
    }
    m_commands.emplace_back(BindVertexBufferCommandUVE{buffer, slot});
}

void NullCommandBufferUVE::BindIndexBufferUVE(BufferHandleUVE buffer) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindIndexBufferUVE")) {
        return;
    }
    m_commands.emplace_back(BindIndexBufferCommandUVE{buffer});
}

void NullCommandBufferUVE::BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindTextureUVE")) {
        return;
    }
    m_commands.emplace_back(BindTextureCommandUVE{texture, slot});
}

void NullCommandBufferUVE::BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindUniformBufferUVE")) {
        return;
    }
    m_commands.emplace_back(BindUniformBufferCommandUVE{buffer, slot});
}

void NullCommandBufferUVE::SetUniformFloatUVE(std::string_view name, float value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformFloatUVE")) {
        return;
    }
    m_commands.emplace_back(SetUniformFloatCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformIntUVE(std::string_view name, std::int32_t value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformIntUVE")) {
        return;
    }
    m_commands.emplace_back(SetUniformIntCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformBoolUVE(std::string_view name, bool value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformBoolUVE")) {
        return;
    }
    m_commands.emplace_back(SetUniformBoolCommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformVector3UVE")) {
        return;
    }
    m_commands.emplace_back(SetUniformVector3CommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformMatrix4x4UVE")) {
        return;
    }
    m_commands.emplace_back(SetUniformMatrix4x4CommandUVE{std::string(name), value});
}

void NullCommandBufferUVE::DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "DrawIndexedUVE")) {
        return;
    }
    m_commands.emplace_back(DrawIndexedCommandUVE{indexCount, instanceCount});
}

void NullCommandBufferUVE::DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "DrawUVE")) {
        return;
    }
    m_commands.emplace_back(DrawCommandUVE{vertexCount, instanceCount});
}

const std::vector<RecordedCommandUVE>& NullCommandBufferUVE::GetRecordedCommandsUVE() const noexcept {
    return m_commands;
}

} // namespace UVE::Render
