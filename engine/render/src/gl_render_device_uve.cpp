//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/gl_render_device_uve.h"

#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "gl_command_buffer_uve.h"
#include "gl_render_device_state_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render {

namespace {

// Bridges GLFW's proc-address lookup (GLFWglproc, a void(*)(void)) to gl_functions_uve.h's
// injected `void* (*)(const char*)` shape, so the loader itself never needs to include GLFW.
// GlRenderDeviceUVE.cpp is the one place in engine/render that includes GLFW directly (mirroring
// WindowManagerUVE's own confinement) — it never calls glfwMakeContextCurrent/glfwCreateWindow/
// glfwDestroyWindow/glfwTerminate itself; WindowManagerUVE already owns that entire lifecycle by
// the time this constructor runs.
void* GlfwProcAddressBridgeUVE(const char* name) {
    return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

[[nodiscard]] GLenum BufferUsageToGlTargetUVE(BufferUsageUVE usage) noexcept {
    switch (usage) {
        case BufferUsageUVE::Vertex:
            return GL_ARRAY_BUFFER;
        case BufferUsageUVE::Index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case BufferUsageUVE::Uniform:
            return GL_UNIFORM_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

struct GlTextureFormatUVE {
    GLint internalFormat;
    GLenum format;
    GLenum type;
};

[[nodiscard]] GlTextureFormatUVE TextureFormatToGlUVE(TextureFormatUVE format) noexcept {
    switch (format) {
        case TextureFormatUVE::RGBA8Unorm:
            return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
        case TextureFormatUVE::RGBA16Float:
            return {GL_RGBA16F, GL_RGBA, GL_FLOAT};
        case TextureFormatUVE::Depth32Float:
            return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT};
    }
    return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
}

[[nodiscard]] GLenum ShaderStageToGlUVE(ShaderStageUVE stage) noexcept {
    switch (stage) {
        case ShaderStageUVE::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderStageUVE::Fragment:
            return GL_FRAGMENT_SHADER;
        case ShaderStageUVE::Compute:
            return GL_COMPUTE_SHADER;
    }
    return GL_VERTEX_SHADER;
}

[[nodiscard]] std::string GetShaderInfoLogUVE(const Detail::GlFunctionsUVE& gl, GLuint shader) {
    GLint logLength = 0;
    gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) {
        return {};
    }
    std::string log(static_cast<std::size_t>(logLength), '\0');
    gl.glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    return log;
}

[[nodiscard]] std::string GetProgramInfoLogUVE(const Detail::GlFunctionsUVE& gl, GLuint program) {
    GLint logLength = 0;
    gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) {
        return {};
    }
    std::string log(static_cast<std::size_t>(logLength), '\0');
    gl.glGetProgramInfoLog(program, logLength, nullptr, log.data());
    return log;
}

} // namespace

struct GlRenderDeviceUVE::ImplUVE {
    Detail::GlDeviceStateUVE state;
};

GlRenderDeviceUVE::GlRenderDeviceUVE(Window::IWindowManagerUVE& windowManager)
    : m_impl(std::make_unique<ImplUVE>()) {
    UVE_ASSERT(windowManager.IsValidUVE());
    m_impl->state.windowManager = &windowManager;
    m_impl->state.gl = Detail::LoadGlFunctionsUVE(&GlfwProcAddressBridgeUVE);

    if (!m_impl->state.gl.IsCompleteUVE()) {
        UVE_FATAL("GlRenderDeviceUVE: one or more required GL function pointers failed to load");
    } else {
        UVE_INFO("GlRenderDeviceUVE: initialized, backend GL_VERSION={}",
                  reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    }
}

GlRenderDeviceUVE::~GlRenderDeviceUVE() = default;

BufferHandleUVE GlRenderDeviceUVE::CreateBufferUVE(const BufferDescUVE& desc, std::span<const std::byte> initialData) {
    const GLenum target = BufferUsageToGlTargetUVE(desc.usage);
    GLuint glBuffer = 0;
    m_impl->state.gl.glGenBuffers(1, &glBuffer);
    m_impl->state.gl.glBindBuffer(target, glBuffer);
    m_impl->state.gl.glBufferData(target, static_cast<GLsizeiptr>(desc.sizeBytes),
                                    initialData.empty() ? nullptr : initialData.data(), GL_STATIC_DRAW);

    const std::uint32_t handleValue = m_impl->state.nextBufferHandle++;
    m_impl->state.buffers.emplace(handleValue,
                                    Detail::GlDeviceStateUVE::BufferRecordUVE{glBuffer, target, desc.sizeBytes});
    return BufferHandleUVE{handleValue};
}

void GlRenderDeviceUVE::DestroyBufferUVE(BufferHandleUVE buffer) {
    const auto it = m_impl->state.buffers.find(buffer.value);
    if (it == m_impl->state.buffers.end()) {
        UVE_ERROR("GlRenderDeviceUVE: DestroyBufferUVE called with an unknown or already-destroyed handle ({})",
                   buffer.value);
        return;
    }
    m_impl->state.gl.glDeleteBuffers(1, &it->second.glBuffer);
    m_impl->state.buffers.erase(it);
}

bool GlRenderDeviceUVE::UpdateBufferUVE(BufferHandleUVE buffer, std::span<const std::byte> data,
                                          std::uint64_t offsetBytes) {
    const auto it = m_impl->state.buffers.find(buffer.value);
    if (it == m_impl->state.buffers.end()) {
        UVE_ERROR("GlRenderDeviceUVE: UpdateBufferUVE called with an unknown handle ({})", buffer.value);
        return false;
    }
    if (offsetBytes + data.size() > it->second.sizeBytes) {
        UVE_ERROR("GlRenderDeviceUVE: UpdateBufferUVE write of {} bytes at offset {} exceeds buffer size {}",
                   data.size(), offsetBytes, it->second.sizeBytes);
        return false;
    }
    m_impl->state.gl.glBindBuffer(it->second.target, it->second.glBuffer);
    m_impl->state.gl.glBufferSubData(it->second.target, static_cast<GLintptr>(offsetBytes),
                                       static_cast<GLsizeiptr>(data.size()), data.data());
    return true;
}

TextureHandleUVE GlRenderDeviceUVE::CreateTextureUVE(const TextureDescUVE& desc,
                                                       std::span<const std::byte> initialData) {
    if (desc.mipLevels > 1) {
        UVE_WARNING("GlRenderDeviceUVE: CreateTextureUVE requested {} mip levels - only level 0 is populated",
                     desc.mipLevels);
    }

    const GlTextureFormatUVE glFormat = TextureFormatToGlUVE(desc.format);
    GLuint glTexture = 0;
    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat, static_cast<GLsizei>(desc.width),
                 static_cast<GLsizei>(desc.height), 0, glFormat.format, glFormat.type,
                 initialData.empty() ? nullptr : initialData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const std::uint32_t handleValue = m_impl->state.nextTextureHandle++;
    m_impl->state.textures.emplace(handleValue, Detail::GlDeviceStateUVE::TextureRecordUVE{glTexture, desc});
    return TextureHandleUVE{handleValue};
}

void GlRenderDeviceUVE::DestroyTextureUVE(TextureHandleUVE texture) {
    const auto it = m_impl->state.textures.find(texture.value);
    if (it == m_impl->state.textures.end()) {
        UVE_ERROR("GlRenderDeviceUVE: DestroyTextureUVE called with an unknown or already-destroyed handle ({})",
                   texture.value);
        return;
    }
    glDeleteTextures(1, &it->second.glTexture);
    m_impl->state.textures.erase(it);
}

ShaderHandleUVE GlRenderDeviceUVE::CreateShaderUVE(const ShaderDescUVE& desc) {
    const GLenum stage = ShaderStageToGlUVE(desc.stage);
    const GLuint glShader = m_impl->state.gl.glCreateShader(stage);

    const char* sourcePointer = desc.sourceCode.c_str();
    const auto sourceLength = static_cast<GLint>(desc.sourceCode.size());
    m_impl->state.gl.glShaderSource(glShader, 1, &sourcePointer, &sourceLength);
    m_impl->state.gl.glCompileShader(glShader);

    GLint compiled = GL_FALSE;
    m_impl->state.gl.glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        UVE_ERROR("GlRenderDeviceUVE: CreateShaderUVE compilation failed: {}",
                   GetShaderInfoLogUVE(m_impl->state.gl, glShader));
        m_impl->state.gl.glDeleteShader(glShader);
        return kInvalidShaderHandleUVE;
    }

    const std::uint32_t handleValue = m_impl->state.nextShaderHandle++;
    m_impl->state.shaders.emplace(handleValue, Detail::GlDeviceStateUVE::ShaderRecordUVE{glShader});
    return ShaderHandleUVE{handleValue};
}

void GlRenderDeviceUVE::DestroyShaderUVE(ShaderHandleUVE shader) {
    const auto it = m_impl->state.shaders.find(shader.value);
    if (it == m_impl->state.shaders.end()) {
        UVE_ERROR("GlRenderDeviceUVE: DestroyShaderUVE called with an unknown or already-destroyed handle ({})",
                   shader.value);
        return;
    }
    m_impl->state.gl.glDeleteShader(it->second.glShader);
    m_impl->state.shaders.erase(it);
}

PipelineHandleUVE GlRenderDeviceUVE::CreatePipelineUVE(const PipelineDescUVE& desc) {
    const auto vertexIt = m_impl->state.shaders.find(desc.vertexShader.value);
    const auto fragmentIt = m_impl->state.shaders.find(desc.fragmentShader.value);
    if (vertexIt == m_impl->state.shaders.end() || fragmentIt == m_impl->state.shaders.end()) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE referenced an unknown vertex or fragment shader handle");
        return kInvalidPipelineHandleUVE;
    }

    const GLuint glProgram = m_impl->state.gl.glCreateProgram();
    m_impl->state.gl.glAttachShader(glProgram, vertexIt->second.glShader);
    m_impl->state.gl.glAttachShader(glProgram, fragmentIt->second.glShader);
    m_impl->state.gl.glLinkProgram(glProgram);

    GLint linked = GL_FALSE;
    m_impl->state.gl.glGetProgramiv(glProgram, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE link failed: {}",
                   GetProgramInfoLogUVE(m_impl->state.gl, glProgram));
        m_impl->state.gl.glDeleteProgram(glProgram);
        return kInvalidPipelineHandleUVE;
    }

    GLuint glVao = 0;
    m_impl->state.gl.glGenVertexArrays(1, &glVao);

    const std::uint32_t handleValue = m_impl->state.nextPipelineHandle++;
    m_impl->state.pipelines.emplace(handleValue, Detail::GlDeviceStateUVE::PipelineRecordUVE{
                                                       glProgram, glVao, desc.vertexLayout, desc.vertexStride,
                                                       desc.depthTestEnabled, desc.depthWriteEnabled});
    return PipelineHandleUVE{handleValue};
}

void GlRenderDeviceUVE::DestroyPipelineUVE(PipelineHandleUVE pipeline) {
    const auto it = m_impl->state.pipelines.find(pipeline.value);
    if (it == m_impl->state.pipelines.end()) {
        UVE_ERROR("GlRenderDeviceUVE: DestroyPipelineUVE called with an unknown or already-destroyed handle ({})",
                   pipeline.value);
        return;
    }
    m_impl->state.gl.glDeleteProgram(it->second.glProgram);
    m_impl->state.gl.glDeleteVertexArrays(1, &it->second.glVao);
    m_impl->state.pipelines.erase(it);
}

std::unique_ptr<ICommandBufferUVE> GlRenderDeviceUVE::CreateCommandBufferUVE() {
    return std::make_unique<GlCommandBufferUVE>(m_impl->state);
}

void GlRenderDeviceUVE::SubmitUVE(std::unique_ptr<ICommandBufferUVE> commandBuffer) {
    UVE_ASSERT(commandBuffer != nullptr);
    auto* const glCommandBuffer = dynamic_cast<GlCommandBufferUVE*>(commandBuffer.get());
    UVE_ASSERT(glCommandBuffer != nullptr); // only this device's own CreateCommandBufferUVE() ever produces one
    // Every GL call already executed during recording (OpenGL has no separate replay step) — the
    // command buffer is simply released here.
}

void GlRenderDeviceUVE::PresentUVE() {
    m_impl->state.windowManager->SwapBuffersUVE();
}

std::string_view GlRenderDeviceUVE::GetBackendNameUVE() const noexcept {
    return "OpenGL";
}

std::size_t GlRenderDeviceUVE::GetLiveResourceCountUVE() const noexcept {
    return m_impl->state.buffers.size() + m_impl->state.textures.size() + m_impl->state.shaders.size() +
           m_impl->state.pipelines.size();
}

} // namespace UVE::Render
