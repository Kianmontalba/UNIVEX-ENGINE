// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "gl_command_buffer_uve.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>


#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render {

namespace {

[[nodiscard]] bool ValidateUniformTypeUVE(
    const Detail::GlDeviceStateUVE::PipelineRecordUVE::UniformRecordUVE& uniform,
    ShaderDataTypeUVE expectedType, std::string_view name) noexcept {
    if (uniform.type == expectedType) {
        return true;
    }
    UVE_ERROR("GlCommandBufferUVE: uniform type mismatch for '{}'", name);
    return false;
}

[[nodiscard]] bool RequireInsideRenderPassUVE(bool insideRenderPass, std::string_view operation) noexcept {
    UVE_ASSERT(insideRenderPass);
    if (!insideRenderPass) {
        UVE_ERROR("GlCommandBufferUVE: {} must be called inside a render pass", operation);
        return false;
    }
    return true;
}

[[nodiscard]] bool RequireOutsideRenderPassUVE(bool insideRenderPass) noexcept {
    UVE_ASSERT(!insideRenderPass);
    if (insideRenderPass) {
        UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE does not support nested render passes");
        return false;
    }
    return true;
}

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

[[nodiscard]] bool IsFiniteVector3UVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFiniteMatrix4x4UVE(const Math::Matrix4x4UVE& value) noexcept {
    for (const auto& row : value.m) {
        for (const float component : row) {
            if (!std::isfinite(component)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

GlCommandBufferUVE::GlCommandBufferUVE(Detail::GlDeviceStateUVE& state) : m_state(&state) {}

void GlCommandBufferUVE::BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) {
    if (!RequireOutsideRenderPassUVE(m_insideRenderPass)) {
        return;
    }
    if (!IsLoadOpValidUVE(renderPassDesc.colorLoadOp) || !IsLoadOpValidUVE(renderPassDesc.depthLoadOp)) {
        UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE received an unknown load operation");
        return;
    }
    if (renderPassDesc.colorLoadOp == LoadOpUVE::Clear) {
        for (const float clearChannel : renderPassDesc.clearColor) {
            if (!std::isfinite(clearChannel)) {
                UVE_ERROR("GlCommandBufferUVE: color clear value must be finite");
                return;
            }
        }
    }
    if (renderPassDesc.depthLoadOp == LoadOpUVE::Clear && !std::isfinite(renderPassDesc.clearDepth)) {
        UVE_ERROR("GlCommandBufferUVE: depth clear value must be finite");
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
        const auto colorIt = renderPassDesc.colorAttachment == kInvalidTextureHandleUVE
                                 ? m_state->textures.end()
                                 : m_state->textures.find(renderPassDesc.colorAttachment.value);
        if (renderPassDesc.colorAttachment != kInvalidTextureHandleUVE && colorIt == m_state->textures.end()) {
            UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE referenced an unknown colorAttachment handle");
            return;
        }
        const auto depthIt = renderPassDesc.depthAttachment == kInvalidTextureHandleUVE
                                 ? m_state->textures.end()
                                 : m_state->textures.find(renderPassDesc.depthAttachment.value);
        if (renderPassDesc.depthAttachment != kInvalidTextureHandleUVE && depthIt == m_state->textures.end()) {
            UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE referenced an unknown depthAttachment handle");
            return;
        }
        if (colorIt != m_state->textures.end() && colorIt->second.desc.format == TextureFormatUVE::Depth32Float) {
            UVE_ERROR("GlCommandBufferUVE: colorAttachment must use a color texture format");
            return;
        }
        if (depthIt != m_state->textures.end() && depthIt->second.desc.format != TextureFormatUVE::Depth32Float) {
            UVE_ERROR("GlCommandBufferUVE: depthAttachment must use TextureFormatUVE::Depth32Float");
            return;
        }
        if (colorIt != m_state->textures.end() && depthIt != m_state->textures.end() &&
            (colorIt->second.desc.width != depthIt->second.desc.width ||
             colorIt->second.desc.height != depthIt->second.desc.height)) {
            UVE_ERROR("GlCommandBufferUVE: colorAttachment and depthAttachment dimensions must match");
            return;
        }

        const std::uint64_t framebufferKey =
            (static_cast<std::uint64_t>(renderPassDesc.colorAttachment.value) << 32U) |
            static_cast<std::uint64_t>(renderPassDesc.depthAttachment.value);
        GLuint framebuffer = 0;
        const auto cachedFramebufferIt = m_state->framebufferCache.find(framebufferKey);
        const bool framebufferCreated = cachedFramebufferIt == m_state->framebufferCache.end();
        if (framebufferCreated) {
            m_state->gl.glGenFramebuffers(1, &framebuffer);
            m_state->framebufferCache.emplace(framebufferKey, framebuffer);
        } else {
            framebuffer = cachedFramebufferIt->second;
        }
        GLint previousFramebuffer = 0;
        if (framebufferCreated) {
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
        }
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        std::uint32_t attachmentWidth = 0;
        std::uint32_t attachmentHeight = 0;
        if (colorIt != m_state->textures.end()) {
            attachmentWidth = colorIt->second.desc.width;
            attachmentHeight = colorIt->second.desc.height;
        }
        if (depthIt != m_state->textures.end() && attachmentWidth == 0) {
            attachmentWidth = depthIt->second.desc.width;
            attachmentHeight = depthIt->second.desc.height;
        }

        if (framebufferCreated) {
            if (colorIt != m_state->textures.end()) {
                m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                                    colorIt->second.glTexture, 0);
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
            if (depthIt != m_state->textures.end()) {
                m_state->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                                    depthIt->second.glTexture, 0);
            }
            if (m_state->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                UVE_ERROR("GlCommandBufferUVE: BeginRenderPassUVE built an incomplete framebuffer");
                m_state->gl.glDeleteFramebuffers(1, &framebuffer);
                m_state->framebufferCache.erase(framebufferKey);
                m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
                return;
            }
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
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "EndRenderPassUVE")) {
        return;
    }
    if (m_tempFramebuffer != 0) {
        m_state->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_tempFramebuffer = 0;
    }
    m_insideRenderPass = false;
}

void GlCommandBufferUVE::BindPipelineUVE(PipelineHandleUVE pipeline) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindPipelineUVE")) {
        return;
    }
    if (pipeline == kInvalidPipelineHandleUVE) {
        UVE_ERROR("GlCommandBufferUVE: BindPipelineUVE referenced an invalid pipeline handle");
        return;
    }
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
    m_currentVertexStride = pipelineIt->second.vertexStride;
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
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindVertexBufferUVE")) {
        return;
    }
    static_cast<void>(slot); // This minimal RHI describes one interleaved vertex layout per
                              // pipeline, not a per-slot binding table — every attribute in
                              // vertexLayout is configured against whichever buffer is bound here.
    if (buffer == kInvalidBufferHandleUVE) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE referenced an invalid buffer handle");
        return;
    }
    if (m_boundVertexBuffer == buffer) {
        return;
    }
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE referenced an unknown buffer handle");
        return;
    }
    if (bufferIt->second.target != GL_ARRAY_BUFFER) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE requires a vertex buffer");
        return;
    }
    const auto* const pipelineRecord = FindCurrentPipelineUVE();
    if (pipelineRecord == nullptr) {
        UVE_ERROR("GlCommandBufferUVE: BindVertexBufferUVE called without a live pipeline");
        return;
    }
    m_state->gl.glBindBuffer(GL_ARRAY_BUFFER, bufferIt->second.glBuffer);

    for (std::size_t index = 0; index < pipelineRecord->vertexLayout.size(); ++index) {
        const VertexAttributeUVE& attribute = pipelineRecord->vertexLayout[index];
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
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindIndexBufferUVE")) {
        return;
    }
    if (buffer == kInvalidBufferHandleUVE) {
        UVE_ERROR("GlCommandBufferUVE: BindIndexBufferUVE referenced an invalid buffer handle");
        return;
    }
    if (m_boundIndexBuffer == buffer) {
        return;
    }
    const auto bufferIt = m_state->buffers.find(buffer.value);
    if (bufferIt == m_state->buffers.end()) {
        UVE_ERROR("GlCommandBufferUVE: BindIndexBufferUVE referenced an unknown buffer handle");
        return;
    }
    if (bufferIt->second.target != GL_ELEMENT_ARRAY_BUFFER) {
        UVE_ERROR("GlCommandBufferUVE: BindIndexBufferUVE requires an index buffer");
        return;
    }
    m_state->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIt->second.glBuffer);
    m_boundIndexBuffer = buffer;
}

void GlCommandBufferUVE::BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindTextureUVE")) {
        return;
    }
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
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "BindUniformBufferUVE")) {
        return;
    }
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
    if (bufferIt->second.target != GL_UNIFORM_BUFFER) {
        UVE_ERROR("GlCommandBufferUVE: BindUniformBufferUVE requires a uniform buffer");
        return;
    }
    m_state->gl.glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferIt->second.glBuffer);
}

const Detail::GlDeviceStateUVE::PipelineRecordUVE* GlCommandBufferUVE::FindCurrentPipelineUVE() const {
    if (m_currentPipeline == kInvalidPipelineHandleUVE) {
        return nullptr;
    }
    const auto pipelineIt = m_state->pipelines.find(m_currentPipeline.value);
    if (pipelineIt == m_state->pipelines.end()) {
        return nullptr;
    }
    return &pipelineIt->second;
}

const GlCommandBufferUVE::UniformRecordUVE* GlCommandBufferUVE::FindUniformUVE(std::string_view name) const {
    const auto* const pipelineRecord = FindCurrentPipelineUVE();
    if (pipelineRecord == nullptr) {
        UVE_WARNING("GlCommandBufferUVE: SetUniform*UVE called without a live pipeline (uniform \"{}\")", name);
        return nullptr;
    }
    const auto it = pipelineRecord->uniforms.find(name);
    if (it == pipelineRecord->uniforms.end()) {
        UVE_WARNING("GlCommandBufferUVE: SetUniform*UVE - \"{}\" is not an active uniform on the bound pipeline",
                     name);
        return nullptr;
    }
    return &it->second;
}

void GlCommandBufferUVE::SetUniformFloatUVE(std::string_view name, float value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformFloatUVE")) {
        return;
    }
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr || !ValidateUniformTypeUVE(*uniform, ShaderDataTypeUVE::Float, name)) {
        return;
    }
    if (!std::isfinite(value)) {
        UVE_ERROR("GlCommandBufferUVE: float uniform '{}' must be finite", name);
        return;
    }
    m_state->gl.glUniform1f(uniform->location, value);
}

void GlCommandBufferUVE::SetUniformIntUVE(std::string_view name, std::int32_t value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformIntUVE")) {
        return;
    }
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr || !ValidateUniformTypeUVE(*uniform, ShaderDataTypeUVE::Int, name)) {
        return;
    }
    m_state->gl.glUniform1i(uniform->location, static_cast<GLint>(value));
}

void GlCommandBufferUVE::SetUniformBoolUVE(std::string_view name, bool value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformBoolUVE")) {
        return;
    }
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr || !ValidateUniformTypeUVE(*uniform, ShaderDataTypeUVE::Bool, name)) {
        return;
    }
    m_state->gl.glUniform1i(uniform->location, value ? 1 : 0);
}

void GlCommandBufferUVE::SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformVector3UVE")) {
        return;
    }
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr || !ValidateUniformTypeUVE(*uniform, ShaderDataTypeUVE::Vec3, name)) {
        return;
    }
    if (!IsFiniteVector3UVE(value)) {
        UVE_ERROR("GlCommandBufferUVE: vector uniform '{}' must be finite", name);
        return;
    }
    m_state->gl.glUniform3fv(uniform->location, 1, &value.x);
}

void GlCommandBufferUVE::SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "SetUniformMatrix4x4UVE")) {
        return;
    }
    const UniformRecordUVE* const uniform = FindUniformUVE(name);
    if (uniform == nullptr || !ValidateUniformTypeUVE(*uniform, ShaderDataTypeUVE::Mat4, name)) {
        return;
    }
    if (!IsFiniteMatrix4x4UVE(value)) {
        UVE_ERROR("GlCommandBufferUVE: matrix uniform '{}' must be finite", name);
        return;
    }
    // Matrix4x4UVE is row-major storage (docs/CODING_STANDARDS.md, "Matrix convention"). Desktop
    // GL accepts GL_TRUE and performs the transpose, but GLES3 requires transpose == GL_FALSE, so
    // Android uploads an explicit column-major stack copy instead of issuing an invalid call.
#if defined(__ANDROID__)
    std::array<float, 16> columnMajor{};
    for (std::size_t row = 0; row < 4U; ++row) {
        for (std::size_t column = 0; column < 4U; ++column) {
            columnMajor[column * 4U + row] = value.m[row][column];
        }
    }
    m_state->gl.glUniformMatrix4fv(uniform->location, 1, GL_FALSE, columnMajor.data());
#else
    m_state->gl.glUniformMatrix4fv(uniform->location, 1, GL_TRUE, &value.m[0][0]);
#endif
}

void GlCommandBufferUVE::DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) {
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "DrawIndexedUVE")) {
        return;
    }
    if (FindCurrentPipelineUVE() == nullptr) {
        UVE_ERROR("GlCommandBufferUVE: DrawIndexedUVE called without a live pipeline");
        return;
    }
    if (m_boundIndexBuffer == kInvalidBufferHandleUVE) {
        UVE_ERROR("GlCommandBufferUVE: DrawIndexedUVE called without a bound index buffer");
        return;
    }
    const auto indexBufferIt = m_state->buffers.find(m_boundIndexBuffer.value);
    if (indexBufferIt == m_state->buffers.end() || indexBufferIt->second.target != GL_ELEMENT_ARRAY_BUFFER) {
        UVE_ERROR("GlCommandBufferUVE: DrawIndexedUVE has no valid bound index buffer");
        return;
    }
    const std::uint64_t indexCapacity = indexBufferIt->second.sizeBytes / sizeof(std::uint32_t);
    if (static_cast<std::uint64_t>(indexCount) > indexCapacity) {
        UVE_ERROR("GlCommandBufferUVE: DrawIndexedUVE indexCount exceeds the bound index buffer");
        return;
    }
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
    if (!RequireInsideRenderPassUVE(m_insideRenderPass, "DrawUVE")) {
        return;
    }
    if (m_currentPipeline != kInvalidPipelineHandleUVE && FindCurrentPipelineUVE() == nullptr) {
        UVE_ERROR("GlCommandBufferUVE: DrawUVE called with a destroyed pipeline");
        return;
    }
    if (m_boundVertexBuffer != kInvalidBufferHandleUVE) {
        const auto vertexBufferIt = m_state->buffers.find(m_boundVertexBuffer.value);
        if (vertexBufferIt == m_state->buffers.end() || vertexBufferIt->second.target != GL_ARRAY_BUFFER) {
            UVE_ERROR("GlCommandBufferUVE: DrawUVE has no valid bound vertex buffer");
            return;
        }
        if (m_currentVertexStride == 0U ||
            static_cast<std::uint64_t>(vertexCount) >
                vertexBufferIt->second.sizeBytes / static_cast<std::uint64_t>(m_currentVertexStride)) {
            UVE_ERROR("GlCommandBufferUVE: DrawUVE vertexCount exceeds the bound vertex buffer");
            return;
        }
    }
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
