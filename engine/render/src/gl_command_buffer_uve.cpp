//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "gl_command_buffer_uve.h"

#include <cstdint>

#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render {

namespace {

[[nodiscard]] GLint VertexAttributeComponentCountUVE(VertexAttributeFormatUVE format) noexcept {
    switch (format) {
        case VertexAttributeFormatUVE::Float2:
            return 2;
        case VertexAttributeFormatUVE::Float3:
            return 3;
        case VertexAttributeFormatUVE::Float4:
            return 4;
    }
    return 3;
}

} // namespace

GlCommandBufferUVE::GlCommandBufferUVE(Detail::GlDeviceStateUVE& state) : m_state(&state) {}

void GlCommandBufferUVE::BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) {
    UVE_ASSERT(!m_insideRenderPass);

    if (renderPassDesc.colorAttachment == kInvalidTextureHandleUVE) {
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_tempFramebuffer = 0;
        const std::uint32_t width = m_state->windowManager->GetWidthUVE();
        const std::uint32_t height = m_state->windowManager->GetHeightUVE();
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    } else {
        const auto colorIt = m_state->textures.find(renderPassDesc.colorAttachment.value);
        if (colorIt == m_state->textures.end()) {
            UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE referenced an unknown colorAttachment handle");
            return;
        }

        GLuint framebuffer = 0;
        m_state->gl.glGenFramebuffers(1, &framebuffer);
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                            colorIt->second.glTexture, 0);

        if (renderPassDesc.depthAttachment != kInvalidTextureHandleUVE) {
            const auto depthIt = m_state->textures.find(renderPassDesc.depthAttachment.value);
            if (depthIt != m_state->textures.end()) {
                m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                                    depthIt->second.glTexture, 0);
            }
        }

        if (m_state->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE built an incomplete framebuffer");
        }

        glViewport(0, 0, static_cast<GLsizei>(colorIt->second.desc.width),
                   static_cast<GLsizei>(colorIt->second.desc.height));
        m_tempFramebuffer = framebuffer;
    }

    GLbitfield clearMask = 0;
    if (renderPassDesc.colorLoadOp == LoadOpUVE::Clear) {
        glClearColor(renderPassDesc.clearColor[0], renderPassDesc.clearColor[1], renderPassDesc.clearColor[2],
                     renderPassDesc.clearColor[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (renderPassDesc.depthLoadOp == LoadOpUVE::Clear) {
        glClearDepth(static_cast<GLdouble>(renderPassDesc.clearDepth));
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearMask != 0) {
        glClear(clearMask);
    }

    m_insideRenderPass = true;
}

void GlCommandBufferUVE::EndRenderPassUVE() {
    UVE_ASSERT(m_insideRenderPass);
    if (m_tempFramebuffer != 0) {
        m_state->gl.glDeleteFramebuffers(1, &m_tempFramebuffer);
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_tempFramebuffer = 0;
    }
    m_insideRenderPass = false;
}

void GlCommandBufferUVE::BindPipelineUVE(PipelineHandleUVE pipeline) {
    const auto pipelineIt = m_state->pipelines.find(pipeline.value);
    if (pipelineIt == m_state->pipelines.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindPipelineUVE referenced an unknown pipeline handle");
        return;
    }
    m_currentProgram = pipelineIt->second.glProgram;
    m_currentVao = pipelineIt->second.glVao;
    m_currentVertexLayout = &pipelineIt->second.vertexLayout;
    m_currentVertexStride = pipelineIt->second.vertexStride;

    m_state->gl.glUseProgram(m_currentProgram);
    m_state->gl.glBindVertexArray(m_currentVao);

    if (pipelineIt->second.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(pipelineIt->second.depthWriteEnabled ? GL_TRUE : GL_FALSE);
}

void GlCommandBufferUVE::BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    static_cast<void>(slot); // This minimal RHI describes one interleaved vertex layout per
                              // pipeline, not a per-slot binding table — every attribute in
                              // vertexLayout is configured against whichever buffer is bound here.
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE referenced an unknown buffer handle");
        return;
    }
    m_state->gl.glBindBuffer(GL_ARRAY_BUFFER, bufferIt->second.glBuffer);

    if (m_currentVertexLayout == nullptr) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE called without a bound pipeline");
        return;
    }
    for (std::size_t index = 0; index < m_currentVertexLayout->size(); ++index) {
        const VertexAttributeUVE& attribute = (*m_currentVertexLayout)[index];
        const auto attributeIndex = static_cast<GLuint>(index);
        m_state->gl.glVertexAttribPointer(
            attributeIndex, VertexAttributeComponentCountUVE(attribute.format), GL_FLOAT, GL_FALSE,
            static_cast<GLsizei>(m_currentVertexStride),
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attribute.offset)));
        m_state->gl.glEnableVertexAttribArray(attributeIndex);
    }
}

void GlCommandBufferUVE::BindIndexBufferUVE(BufferHandleUVE buffer) {
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindIndexBufferUVE referenced an unknown buffer handle");
        return;
    }
    m_state->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIt->second.glBuffer);
}

void GlCommandBufferUVE::BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) {
    const auto textureIt = m_state->textures.find(texture.value);
    if (textureIt == m_state->textures.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindTextureUVE referenced an unknown texture handle");
        return;
    }
    m_state->gl.glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + slot));
    glBindTexture(GL_TEXTURE_2D, textureIt->second.glTexture);
}

void GlCommandBufferUVE::BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindUniformBufferUVE referenced an unknown buffer handle");
        return;
    }
    m_state->gl.glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferIt->second.glBuffer);
}

void GlCommandBufferUVE::DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) {
    if (instanceCount > 1) {
        UVE_WARNING("GlCommandBufferUVE: DrawIndexedUVE instanceCount > 1 is not yet supported - drawing once");
    }
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}

void GlCommandBufferUVE::DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) {
    if (instanceCount > 1) {
        UVE_WARNING("GlCommandBufferUVE: DrawUVE instanceCount > 1 is not yet supported - drawing once");
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

} // namespace UVE::Render
