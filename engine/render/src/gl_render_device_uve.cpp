// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/gl_render_device_uve.h"

#include <limits>
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
        case ShaderStageUVE::Geometry:
            return GL_GEOMETRY_SHADER;
    }
    return GL_VERTEX_SHADER;
}

/// Maps a GLenum uniform type (as reported by glGetActiveUniform) to the engine-native
/// ShaderDataTypeUVE. Every sampler type (2D, cube, array, ...) reports as Int, since a sampler
/// uniform is always set the same way as a plain int - the texture unit index (glUniform1i) -
/// regardless of which concrete sampler type it is; this engine has no per-sampler-type SetUVE
/// overload to distinguish them anyway (matches basic_3d_textured.glsl's placeholder-only role
/// this increment - no real texture binding exists yet for this to matter in practice).
[[nodiscard]] ShaderDataTypeUVE GlUniformTypeToShaderDataTypeUVE(GLenum glType) noexcept {
    switch (glType) {
        case GL_FLOAT:
            return ShaderDataTypeUVE::Float;
        case GL_FLOAT_VEC2:
            return ShaderDataTypeUVE::Vec2;
        case GL_FLOAT_VEC3:
            return ShaderDataTypeUVE::Vec3;
        case GL_FLOAT_VEC4:
            return ShaderDataTypeUVE::Vec4;
        case GL_FLOAT_MAT3:
            return ShaderDataTypeUVE::Mat3;
        case GL_FLOAT_MAT4:
            return ShaderDataTypeUVE::Mat4;
        case GL_BOOL:
            return ShaderDataTypeUVE::Bool;
        case GL_INT:
        default:
            return ShaderDataTypeUVE::Int;
    }
}

/// Reflects every active uniform in `glProgram` (already linked) into `outUniforms`, keyed by
/// name with a trailing "[0]" (glGetActiveUniform's own convention for the first element of an
/// array uniform) stripped so callers can address an array uniform by its bare declared name.
void ReflectPipelineUniformsUVE(
    const Detail::GlFunctionsUVE& gl, GLuint glProgram,
    std::unordered_map<std::string, Detail::GlDeviceStateUVE::PipelineRecordUVE::UniformRecordUVE>& outUniforms) {
    outUniforms.clear();

    GLint activeUniformCount = 0;
    gl.glGetProgramiv(glProgram, GL_ACTIVE_UNIFORMS, &activeUniformCount);
    GLint maxNameLength = 0;
    gl.glGetProgramiv(glProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
    if (activeUniformCount <= 0 || maxNameLength <= 0) {
        return;
    }

    std::string nameBuffer(static_cast<std::size_t>(maxNameLength), '\0');
    for (GLint index = 0; index < activeUniformCount; ++index) {
        GLsizei writtenLength = 0;
        GLint arraySize = 0;
        GLenum glType = 0;
        gl.glGetActiveUniform(glProgram, static_cast<GLuint>(index), maxNameLength, &writtenLength, &arraySize,
                               &glType, nameBuffer.data());
        std::string name(nameBuffer.data(), static_cast<std::size_t>(writtenLength));
        if (name.size() > 3 && name.compare(name.size() - 3, 3, "[0]") == 0) {
            name.resize(name.size() - 3);
        }

        const GLint location = gl.glGetUniformLocation(glProgram, name.c_str());
        outUniforms.emplace(std::move(name), Detail::GlDeviceStateUVE::PipelineRecordUVE::UniformRecordUVE{
                                                  GlUniformTypeToShaderDataTypeUVE(glType), location,
                                                  static_cast<std::uint32_t>(arraySize)});
    }
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
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_impl->state.maxCombinedTextureImageUnits);
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &m_impl->state.maxUniformBufferBindings);
        UVE_INFO("GlRenderDeviceUVE: initialized, backend GL_VERSION={}",
                  reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    }
}

GlRenderDeviceUVE::~GlRenderDeviceUVE() = default;

BufferHandleUVE GlRenderDeviceUVE::CreateBufferUVE(const BufferDescUVE& desc, std::span<const std::byte> initialData) {
    if (!ValidateBufferUploadUVE(desc, initialData)) {
        UVE_ERROR("GlRenderDeviceUVE: CreateBufferUVE initial data exceeds buffer size");
        return kInvalidBufferHandleUVE;
    }
    if (!IsBufferUsageValidUVE(desc.usage)) {
        UVE_ERROR("GlRenderDeviceUVE: CreateBufferUVE received an unknown buffer usage");
        return kInvalidBufferHandleUVE;
    }
    if (desc.sizeBytes > static_cast<std::uint64_t>(std::numeric_limits<GLsizeiptr>::max())) {
        UVE_ERROR("GlRenderDeviceUVE: CreateBufferUVE size exceeds the GLsizeiptr range");
        return kInvalidBufferHandleUVE;
    }
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
    if (!ValidateTextureUploadUVE(desc, initialData)) {
        UVE_ERROR("GlRenderDeviceUVE: CreateTextureUVE received an invalid descriptor or initial upload");
        return kInvalidTextureHandleUVE;
    }
    if (desc.mipLevels > 1) {
        UVE_WARNING("GlRenderDeviceUVE: CreateTextureUVE requested {} mip levels - only level 0 is populated",
                     desc.mipLevels);
    }
    if (desc.width > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()) ||
        desc.height > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        UVE_ERROR("GlRenderDeviceUVE: CreateTextureUVE dimensions exceed the GLsizei range");
        return kInvalidTextureHandleUVE;
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

ShaderHandleUVE GlRenderDeviceUVE::CreateShaderUVE(const ShaderDescUVE& desc, std::string* outInfoLog) {
    if (!IsShaderStageValidUVE(desc.stage)) {
        if (outInfoLog != nullptr) {
            *outInfoLog = "Unknown shader stage.";
        }
        UVE_ERROR("GlRenderDeviceUVE: CreateShaderUVE received an unknown shader stage");
        return kInvalidShaderHandleUVE;
    }
    const GLenum stage = ShaderStageToGlUVE(desc.stage);
    const GLuint glShader = m_impl->state.gl.glCreateShader(stage);

    const char* sourcePointer = desc.sourceCode.c_str();
    const auto sourceLength = static_cast<GLint>(desc.sourceCode.size());
    m_impl->state.gl.glShaderSource(glShader, 1, &sourcePointer, &sourceLength);
    m_impl->state.gl.glCompileShader(glShader);

    const std::string infoLog = GetShaderInfoLogUVE(m_impl->state.gl, glShader);
    if (outInfoLog != nullptr) {
        *outInfoLog = infoLog;
    }

    GLint compiled = GL_FALSE;
    m_impl->state.gl.glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        UVE_ERROR("GlRenderDeviceUVE: CreateShaderUVE compilation failed: {}", infoLog);
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

PipelineHandleUVE GlRenderDeviceUVE::CreatePipelineUVE(const PipelineDescUVE& desc, std::string* outInfoLog) {
    if (!IsVertexLayoutValidUVE(desc.vertexLayout)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE received an unknown vertex attribute format");
        return kInvalidPipelineHandleUVE;
    }
    if (!IsPipelineBlendModeValidUVE(desc.blendMode)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE received an unknown blend mode");
        return kInvalidPipelineHandleUVE;
    }
    if (!IsPrimitiveTopologyValidUVE(desc.topology)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE received an unknown primitive topology");
        return kInvalidPipelineHandleUVE;
    }
    if (!desc.vertexLayout.empty() &&
        (!IsVertexLayoutWithinStrideUVE(desc.vertexLayout, desc.vertexStride) || desc.vertexStride == 0U ||
         desc.vertexStride > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()))) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE vertex attributes exceed the GL stride range");
        return kInvalidPipelineHandleUVE;
    }
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

    const std::string infoLog = GetProgramInfoLogUVE(m_impl->state.gl, glProgram);
    if (outInfoLog != nullptr) {
        *outInfoLog = infoLog;
    }

    GLint linked = GL_FALSE;
    m_impl->state.gl.glGetProgramiv(glProgram, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineUVE link failed: {}", infoLog);
        m_impl->state.gl.glDeleteProgram(glProgram);
        return kInvalidPipelineHandleUVE;
    }

    GLuint glVao = 0;
    m_impl->state.gl.glGenVertexArrays(1, &glVao);

    Detail::GlDeviceStateUVE::PipelineRecordUVE record{
        glProgram, glVao, desc.vertexLayout, desc.vertexStride, desc.depthTestEnabled, desc.depthWriteEnabled,
        desc.blendMode, {}};
    ReflectPipelineUniformsUVE(m_impl->state.gl, glProgram, record.uniforms);

    const std::uint32_t handleValue = m_impl->state.nextPipelineHandle++;
    m_impl->state.pipelines.emplace(handleValue, std::move(record));
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

std::vector<UniformReflectionUVE> GlRenderDeviceUVE::GetPipelineUniformsUVE(PipelineHandleUVE pipeline) const {
    const auto it = m_impl->state.pipelines.find(pipeline.value);
    if (it == m_impl->state.pipelines.end()) {
        return {};
    }
    std::vector<UniformReflectionUVE> result;
    result.reserve(it->second.uniforms.size());
    for (const auto& [name, record] : it->second.uniforms) {
        result.push_back(UniformReflectionUVE{name, record.type, record.location, record.arraySize});
    }
    return result;
}

bool GlRenderDeviceUVE::GetPipelineBinaryUVE(PipelineHandleUVE pipeline, std::vector<std::byte>& outBinary,
                                              std::uint32_t& outFormat) const {
    const auto it = m_impl->state.pipelines.find(pipeline.value);
    if (it == m_impl->state.pipelines.end()) {
        UVE_ERROR("GlRenderDeviceUVE: GetPipelineBinaryUVE called with an unknown handle ({})", pipeline.value);
        return false;
    }

    GLint binaryLength = 0;
    m_impl->state.gl.glGetProgramiv(it->second.glProgram, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    if (binaryLength <= 0) {
        UVE_WARNING("GlRenderDeviceUVE: GetPipelineBinaryUVE - driver reports no binary available for pipeline {}",
                     pipeline.value);
        return false;
    }

    outBinary.resize(static_cast<std::size_t>(binaryLength));
    GLsizei writtenLength = 0;
    GLenum glBinaryFormat = 0;
    m_impl->state.gl.glGetProgramBinary(it->second.glProgram, binaryLength, &writtenLength, &glBinaryFormat,
                                          outBinary.data());
    outBinary.resize(static_cast<std::size_t>(writtenLength));
    outFormat = static_cast<std::uint32_t>(glBinaryFormat);
    return true;
}

PipelineHandleUVE GlRenderDeviceUVE::CreatePipelineFromBinaryUVE(std::span<const std::byte> binary,
                                                                  std::uint32_t format,
                                                                  const PipelineBinaryDescUVE& desc) {
    if (!IsVertexLayoutValidUVE(desc.vertexLayout)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE received an unknown vertex attribute format");
        return kInvalidPipelineHandleUVE;
    }
    if (!IsPipelineBlendModeValidUVE(desc.blendMode)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE received an unknown blend mode");
        return kInvalidPipelineHandleUVE;
    }
    if (!IsPrimitiveTopologyValidUVE(desc.topology)) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE received an unknown primitive topology");
        return kInvalidPipelineHandleUVE;
    }
    if (!desc.vertexLayout.empty() &&
        (!IsVertexLayoutWithinStrideUVE(desc.vertexLayout, desc.vertexStride) || desc.vertexStride == 0U ||
         desc.vertexStride > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()))) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE vertex attributes exceed the GL stride range");
        return kInvalidPipelineHandleUVE;
    }
    if (binary.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        UVE_ERROR("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE binary exceeds the GLsizei range");
        return kInvalidPipelineHandleUVE;
    }
    const GLuint glProgram = m_impl->state.gl.glCreateProgram();
    m_impl->state.gl.glProgramBinary(glProgram, static_cast<GLenum>(format), binary.data(),
                                       static_cast<GLsizei>(binary.size()));

    GLint linked = GL_FALSE;
    m_impl->state.gl.glGetProgramiv(glProgram, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        // A driver-rejected binary (e.g. stale after a driver/GPU update) is an expected,
        // recoverable condition - callers must treat this exactly like a cache miss, never a
        // hard compile failure, so this logs at WARNING rather than ERROR.
        UVE_WARNING("GlRenderDeviceUVE: CreatePipelineFromBinaryUVE - driver rejected the supplied binary: {}",
                     GetProgramInfoLogUVE(m_impl->state.gl, glProgram));
        m_impl->state.gl.glDeleteProgram(glProgram);
        return kInvalidPipelineHandleUVE;
    }

    GLuint glVao = 0;
    m_impl->state.gl.glGenVertexArrays(1, &glVao);

    Detail::GlDeviceStateUVE::PipelineRecordUVE record{
        glProgram, glVao, desc.vertexLayout, desc.vertexStride, desc.depthTestEnabled, desc.depthWriteEnabled,
        desc.blendMode, {}};
    // Uniform locations are not guaranteed portable across a binary load even though behavior
    // is - reflection must always be re-run here, never assumed inherited from the original
    // compile that produced this binary.
    ReflectPipelineUniformsUVE(m_impl->state.gl, glProgram, record.uniforms);

    const std::uint32_t handleValue = m_impl->state.nextPipelineHandle++;
    m_impl->state.pipelines.emplace(handleValue, std::move(record));
    return PipelineHandleUVE{handleValue};
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
