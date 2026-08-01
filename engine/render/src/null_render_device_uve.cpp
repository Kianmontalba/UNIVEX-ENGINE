//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/null_render_device_uve.h"

#include <unordered_map>
#include <utility>

#include "null_command_buffer_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render {

struct NullRenderDeviceUVE::ImplUVE {
    std::unordered_map<std::uint32_t, BufferDescUVE> buffers;
    std::uint32_t nextBufferHandle = 1;
    std::unordered_map<std::uint32_t, TextureDescUVE> textures;
    std::uint32_t nextTextureHandle = 1;
    std::unordered_map<std::uint32_t, ShaderDescUVE> shaders;
    std::uint32_t nextShaderHandle = 1;
    std::unordered_map<std::uint32_t, PipelineDescUVE> pipelines;
    std::uint32_t nextPipelineHandle = 1;
    std::vector<RecordedCommandUVE> lastSubmittedCommands;
    std::uint64_t presentCallCount = 0;
};

NullRenderDeviceUVE::NullRenderDeviceUVE() : m_impl(std::make_unique<ImplUVE>()) {}

NullRenderDeviceUVE::~NullRenderDeviceUVE() = default;

BufferHandleUVE NullRenderDeviceUVE::CreateBufferUVE(const BufferDescUVE& desc,
                                                       std::span<const std::byte> initialData) {
    static_cast<void>(initialData); // NullRenderDeviceUVE performs no real upload, bookkeeping only.
    const std::uint32_t handleValue = m_impl->nextBufferHandle++;
    m_impl->buffers.emplace(handleValue, desc);
    return BufferHandleUVE{handleValue};
}

void NullRenderDeviceUVE::DestroyBufferUVE(BufferHandleUVE buffer) {
    if (m_impl->buffers.erase(buffer.value) == 0) {
        UVE_ERROR("NullRenderDeviceUVE: DestroyBufferUVE called with an unknown or already-destroyed handle ({})",
                   buffer.value);
    }
}

bool NullRenderDeviceUVE::UpdateBufferUVE(BufferHandleUVE buffer, std::span<const std::byte> data,
                                           std::uint64_t offsetBytes) {
    const auto iterator = m_impl->buffers.find(buffer.value);
    if (iterator == m_impl->buffers.end()) {
        UVE_ERROR("NullRenderDeviceUVE: UpdateBufferUVE called with an unknown handle ({})", buffer.value);
        return false;
    }
    if (offsetBytes + data.size() > iterator->second.sizeBytes) {
        UVE_ERROR("NullRenderDeviceUVE: UpdateBufferUVE write of {} bytes at offset {} exceeds buffer size {}",
                   data.size(), offsetBytes, iterator->second.sizeBytes);
        return false;
    }
    return true;
}

TextureHandleUVE NullRenderDeviceUVE::CreateTextureUVE(const TextureDescUVE& desc,
                                                         std::span<const std::byte> initialData) {
    static_cast<void>(initialData); // NullRenderDeviceUVE performs no real upload, bookkeeping only.
    const std::uint32_t handleValue = m_impl->nextTextureHandle++;
    m_impl->textures.emplace(handleValue, desc);
    return TextureHandleUVE{handleValue};
}

void NullRenderDeviceUVE::DestroyTextureUVE(TextureHandleUVE texture) {
    if (m_impl->textures.erase(texture.value) == 0) {
        UVE_ERROR("NullRenderDeviceUVE: DestroyTextureUVE called with an unknown or already-destroyed handle ({})",
                   texture.value);
    }
}

ShaderHandleUVE NullRenderDeviceUVE::CreateShaderUVE(const ShaderDescUVE& desc) {
    const std::uint32_t handleValue = m_impl->nextShaderHandle++;
    m_impl->shaders.emplace(handleValue, desc);
    return ShaderHandleUVE{handleValue};
}

void NullRenderDeviceUVE::DestroyShaderUVE(ShaderHandleUVE shader) {
    if (m_impl->shaders.erase(shader.value) == 0) {
        UVE_ERROR("NullRenderDeviceUVE: DestroyShaderUVE called with an unknown or already-destroyed handle ({})",
                   shader.value);
    }
}

PipelineHandleUVE NullRenderDeviceUVE::CreatePipelineUVE(const PipelineDescUVE& desc) {
    if (!m_impl->shaders.contains(desc.vertexShader.value) || !m_impl->shaders.contains(desc.fragmentShader.value)) {
        UVE_ERROR("NullRenderDeviceUVE: CreatePipelineUVE referenced an unknown vertex or fragment shader handle");
        return kInvalidPipelineHandleUVE;
    }
    const std::uint32_t handleValue = m_impl->nextPipelineHandle++;
    m_impl->pipelines.emplace(handleValue, desc);
    return PipelineHandleUVE{handleValue};
}

void NullRenderDeviceUVE::DestroyPipelineUVE(PipelineHandleUVE pipeline) {
    if (m_impl->pipelines.erase(pipeline.value) == 0) {
        UVE_ERROR("NullRenderDeviceUVE: DestroyPipelineUVE called with an unknown or already-destroyed handle ({})",
                   pipeline.value);
    }
}

std::unique_ptr<ICommandBufferUVE> NullRenderDeviceUVE::CreateCommandBufferUVE() {
    return std::make_unique<NullCommandBufferUVE>();
}

void NullRenderDeviceUVE::SubmitUVE(std::unique_ptr<ICommandBufferUVE> commandBuffer) {
    UVE_ASSERT(commandBuffer != nullptr);
    auto* const nullCommandBuffer = dynamic_cast<NullCommandBufferUVE*>(commandBuffer.get());
    UVE_ASSERT(nullCommandBuffer != nullptr); // only this device's own CreateCommandBufferUVE() ever produces one
    m_impl->lastSubmittedCommands = nullCommandBuffer->GetRecordedCommandsUVE();
}

void NullRenderDeviceUVE::PresentUVE() {
    ++m_impl->presentCallCount;
}

std::string_view NullRenderDeviceUVE::GetBackendNameUVE() const noexcept {
    return "Null";
}

const std::vector<RecordedCommandUVE>& NullRenderDeviceUVE::GetLastSubmittedCommandsUVE() const noexcept {
    return m_impl->lastSubmittedCommands;
}

std::size_t NullRenderDeviceUVE::GetLiveResourceCountUVE() const noexcept {
    return m_impl->buffers.size() + m_impl->textures.size() + m_impl->shaders.size() + m_impl->pipelines.size();
}

std::uint64_t NullRenderDeviceUVE::GetPresentCallCountUVE() const noexcept {
    return m_impl->presentCallCount;
}

} // namespace UVE::Render
