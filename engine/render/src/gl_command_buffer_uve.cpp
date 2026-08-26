// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "gl_command_buffer_uve.h"
#include <cstdint>
#include <limits>


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
    if (!IsLoadOpValidUVE(renderPassDesc.colorLoadOp) || !IsLoadOpValidUVE(renderPassDesc.depthLoadOp)) {
        UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE received an unknown load operation");
        return;
    }

    if (renderPassDesc.colorAttachment == kInvalidTextureHandleUVE &&
        renderPassDesc.depthAttachment == kInvalidTextureHandleUVE) {
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_tempFramebuffer = 0;
        const std::uint32_t width = m_state->windowManager->GetWidthUVE();
        const std::uint32_t height = m_state->windowManager->GetHeightUVE();
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    } else {
        const std::uint64_t framebufferKey =
            (static_cast<std::uint64_t>(renderPassDesc.colorAttachment.value) << 32U) |
            static_cast<std::uint64_t>(renderPassDesc.depthAttachment.value);
        GLuint framebuffer = 0;
        const auto cachedFramebufferIt = m_state->framebufferCache.find(framebufferKey);
        if (cachedFramebufferIt == m_state->framebufferCache.end()) {
            m_state->gl.glGenFramebuffers(1, &framebuffer);
            m_state->framebufferCache.emplace(framebufferKey, framebuffer);
        } else {
            framebuffer = cachedFramebufferIt->second;
        }
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        std::uint32_t attachmentWidth = 0;
        std::uint32_t attachmentHeight = 0;

        if (renderPassDesc.colorAttachment != kInvalidTextureHandleUVE) {
            const auto colorIt = m_state->textures.find(renderPassDesc.colorAttachment.value);
            if (colorIt == m_state->textures.end()) {
                UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE referenced an unknown colorAttachment handle");
                return;
            }
            m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                                colorIt->second.glTexture, 0);
            attachmentWidth = colorIt->second.desc.width;
            attachmentHeight = colorIt->second.desc.height;
        } else {
            // Depth-only pass (e.g. a shadow map's depth pre-pass, Increment 26): a core-profile
            // FBO with no color attachment must explicitly declare it has none, or
            // glCheckFramebufferStatus reports GL_FRAMEBUFFER_INCOMPLETE_DRAW/READ_BUFFER.
            // Desktop core OpenGL requires an explicit no-color draw/read buffer for a depth-only
            // FBO. GLES3 has no glDrawBuffer/glReadBuffer entry points; its framebuffer contract
            // already treats a depth-only FBO as having no color target.
#if !defined(__ANDROID__)
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
#endif
        }

        if (renderPassDesc.depthAttachment != kInvalidTextureHandleUVE) {
            const auto depthIt = m_state->textures.find(renderPassDesc.depthAttachment.value);
            if (depthIt != m_state->textures.end()) {
                m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                                    depthIt->second.glTexture, 0);
                if (attachmentWidth == 0) {
                    attachmentWidth = depthIt->second.desc.width;
                    attachmentHeight = depthIt->second.desc.height;
                }
            }
        }

        if (m_state->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE built an incomplete framebuffer");
        }

        glViewport(0, 0, static_cast<GLsizei>(attachmentWidth), static_cast<GLsizei>(attachmentHeight));
        m_tempFramebuffer = framebuffer;
    }

    GLbitfield clearMask = 0;
    if (renderPassDesc.colorLoadOp == LoadOpUVE::Clear) {
        glClearColor(renderPassDesc.clearColor[0], renderPassDesc.clearColor[1], renderPassDesc.clearColor[2],
                     renderPassDesc.clearColor[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (renderPassDesc.depthLoadOp == LoadOpUVE::Clear) {
        // glClear(GL_DEPTH_BUFFER_BIT) respects GL_DEPTH_WRITEMASK. A prior fullscreen pass
        // deliberately disables depth writes, so a render pass that explicitly requests a depth
        // clear must restore the write mask first or its clear becomes a silent no-op and all
        // same-depth geometry in subsequent frames can fail GL_LESS.
        glDepthMask(GL_TRUE);
#if defined(__ANDROID__)
        glClearDepthf(renderPassDesc.clearDepth);
#else
        glClearDepth(static_cast<GLdouble>(renderPassDesc.clearDepth));
#endif
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
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_tempFramebuffer = 0;
    }
    m_insideRenderPass = false;
}

void GlCommandBufferUVE::BindPipelineUVE(PipelineHandleUVE pipeline) {
    if (m_currentPipeline == pipeline) {
        return;
    }
    const auto pipelineIt = m_state->pipelines.find(pipeline.value);
    if (pipelineIt == m_state->pipelines.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindPipelineUVE referenced an unknown pipeline handle");
        return;
    }
    m_currentProgram = pipelineIt->second.glProgram;
    m_currentVao = pipelineIt->second.glVao;
    m_currentPipeline = pipeline;
    m_boundVertexBuffer = kInvalidBufferHandleUVE;
    m_boundIndexBuffer = kInvalidBufferHandleUVE;
    m_currentVertexLayout = &pipelineIt->second.vertexLayout;
    m_currentVertexStride = pipelineIt->second.vertexStride;
    m_currentUniforms = &pipelineIt->second.uniforms;

    m_state->gl.glUseProgram(m_currentProgram);
    m_state->gl.glBindVertexArray(m_currentVao);

    if (pipelineIt->second.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(pipelineIt->second.depthWriteEnabled ? GL_TRUE : GL_FALSE);
    if (pipelineIt->second.blendMode == PipelineBlendModeUVE::SourceAlphaOver) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void GlCommandBufferUVE::BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    static_cast<void>(slot); // This minimal RHI describes one interleaved vertex layout per
                              // pipeline, not a per-slot binding table — every attribute in
                              // vertexLayout is configured against whichever buffer is bound here.
    if (m_boundVertexBuffer == buffer) {
        return;
    }
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
    m_boundVertexBuffer = buffer;
}

void GlCommandBufferUVE::BindIndexBufferUVE(BufferHandleUVE buffer) {
    if (m_boundIndexBuffer == buffer) {
        return;
    }
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindIndexBufferUVE referenced an unknown buffer handle");
        return;
    }
    m_state->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIt->second.glBuffer);
    m_boundIndexBuffer = buffer;
}

void GlCommandBufferUVE::BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) {
    if (m_state->maxCombinedTextureImageUnits <= 0 ||
        slot >= static_cast<std::uint32_t>(m_state->maxCombinedTextureImageUnits)) {
        UVE_ERROR("GlCommandBufferUVE: BindTextureUVE texture slot exceeds GL texture-unit limits");
        return;
    }
    const auto textureIt = m_state->textures.find(texture.value);
    if (textureIt == m_state->textures.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindTextureUVE referenced an unknown texture handle");
        return;
    }
    const auto boundTextureIt = m_boundTextures.find(slot);
    if (boundTextureIt != m_boundTextures.end() && boundTextureIt->second == texture) {
        return;
    }
    m_state->gl.glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + slot));
    glBindTexture(GL_TEXTURE_2D, textureIt->second.glTexture);
    m_boundTextures[slot] = texture;
}

void GlCommandBufferUVE::BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) {
    if (m_state->maxUniformBufferBindings <= 0 ||
        slot >= static_cast<std::uint32_t>(m_state->maxUniformBufferBindings)) {
        UVE_ERROR("GlCommandBufferUVE: BindUniformBufferUVE slot exceeds GL uniform-buffer limits");
        return;
    }
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindUniformBufferUVE referenced an unknown buffer handle");
        return;
    }
    m_state->gl.glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferIt->second.glBuffer);
}

const GlCommandBufferUVE::UniformRecordUVE* GlCommandBufferUVE::FindUniformUVE(std::string_view name) const {
    if (m_currentUniforms == nullptr) {
        UVE_WARNING("GlCommandBufferUVE: SetUniform*UVE called without a bound pipeline (uniform \"{}\")", name);
        return nullptr;
    }
    const auto it = m_currentUniforms->find(name);
    if (it == m_currentUniforms->end()) {
        UVE_WARNING("GlCommandBufferUVE: SetUniform*UVE - \"{}\" is not an active uniform on the bound pipeline",
                     name);
        return nullptr;
    }
    return &it->second;
}

void GlCommandBufferUVE::SetUniformFloatUVE(std::string_view name, float value) {
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr) {
        return;
    }
    m_state->gl.glUniform1f(uniform->location, value);
}

void GlCommandBufferUVE::SetUniformIntUVE(std::string_view name, std::int32_t value) {
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr) {
        return;
    }
    m_state->gl.glUniform1i(uniform->location, static_cast<GLint>(value));
}

void GlCommandBufferUVE::SetUniformBoolUVE(std::string_view name, bool value) {
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr) {
        return;
    }
    m_state->gl.glUniform1i(uniform->location, value ? 1 : 0);
}

void GlCommandBufferUVE::SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) {
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr) {
        return;
    }
    m_state->gl.glUniform3fv(uniform->location, 1, &value.x);
}

void GlCommandBufferUVE::SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) {
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr) {
        return;
    }
    // Matrix4x4UVE is row-major storage (docs/CODING_STANDARDS.md, "Matrix convention") while GL's
    // native layout is column-major - GL_TRUE tells the driver to transpose our data into its own
    // layout rather than requiring a manual transpose copy here.
    m_state->gl.glUniformMatrix4fv(uniform->location, 1, GL_TRUE, &value.m[0][0]);
}

void GlCommandBufferUVE::DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) {
    if (indexCount > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        UVE_ERROR("GlCommandBufferUVE: DrawIndexedUVE indexCount exceeds the GLsizei range");
        return;
    }
    if (instanceCount > 1) {
        UVE_WARNING("GlCommandBufferUVE: DrawIndexedUVE instanceCount > 1 is not yet supported - drawing once");
    }
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}

void GlCommandBufferUVE::DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) {
    if (vertexCount > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        UVE_ERROR("GlCommandBufferUVE: DrawUVE vertexCount exceeds the GLsizei range");
        return;
    }
    if (instanceCount > 1) {
        UVE_WARNING("GlCommandBufferUVE: DrawUVE instanceCount > 1 is not yet supported - drawing once");
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

} // namespace UVE::Render
