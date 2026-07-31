//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>

#include "uve/render/i_render_device_uve.h"
#include "uve/render/recorded_command_uve.h"

namespace UVE::Render {

/// NullRenderDeviceUVE is the only IRenderDeviceUVE backend this sandbox can build and test: it
/// performs zero real GPU work — there is no display server, GPU device node, or graphics SDK
/// available here (confirmed and documented in docs/CODING_STANDARDS.md) — and instead validates
/// and bookkeeps every call, handing out a NullCommandBufferUVE "spy" (engine/render/src/,
/// module-private) that records the exact sequence of RHI calls a real backend would have
/// received. Existing solely so the rest of the rendering pipeline (CameraSystemUVE,
/// MeshRendererUVE, Renderer3DUVE — later increments) can be built and unit-tested against a real
/// IRenderDeviceUVE& today; a genuine Vulkan/Metal/D3D12 backend is future work once this
/// environment has the SDK headers, GPU, and windowing it currently lacks.
/// Thread-safety: not thread-safe. Every method is intended to be called only from the main
/// engine/render thread, matching RenderSystemUVE's own single-threaded frame contract.
class NullRenderDeviceUVE final : public IRenderDeviceUVE {
public:
    NullRenderDeviceUVE();
    ~NullRenderDeviceUVE() override;

    NullRenderDeviceUVE(const NullRenderDeviceUVE&) = delete;
    NullRenderDeviceUVE& operator=(const NullRenderDeviceUVE&) = delete;

    [[nodiscard]] BufferHandleUVE CreateBufferUVE(const BufferDescUVE& desc,
                                                   std::span<const std::byte> initialData = {}) override;
    void DestroyBufferUVE(BufferHandleUVE buffer) override;
    [[nodiscard]] bool UpdateBufferUVE(BufferHandleUVE buffer, std::span<const std::byte> data,
                                        std::uint64_t offsetBytes = 0) override;

    [[nodiscard]] TextureHandleUVE CreateTextureUVE(const TextureDescUVE& desc,
                                                     std::span<const std::byte> initialData = {}) override;
    void DestroyTextureUVE(TextureHandleUVE texture) override;

    [[nodiscard]] ShaderHandleUVE CreateShaderUVE(const ShaderDescUVE& desc) override;
    void DestroyShaderUVE(ShaderHandleUVE shader) override;

    [[nodiscard]] PipelineHandleUVE CreatePipelineUVE(const PipelineDescUVE& desc) override;
    void DestroyPipelineUVE(PipelineHandleUVE pipeline) override;

    [[nodiscard]] std::unique_ptr<ICommandBufferUVE> CreateCommandBufferUVE() override;
    void SubmitUVE(std::unique_ptr<ICommandBufferUVE> commandBuffer) override;

    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override;

    /// Test-only hook (not part of IRenderDeviceUVE): the command list from the most recently
    /// submitted command buffer, in recorded order. Empty until the first SubmitUVE() call.
    [[nodiscard]] const std::vector<RecordedCommandUVE>& GetLastSubmittedCommandsUVE() const noexcept;

    /// Test-only hook: how many buffer/texture/shader/pipeline resources are currently alive
    /// (created but not yet destroyed) — lets tests confirm cleanup without a GPU to inspect.
    [[nodiscard]] std::size_t GetLiveResourceCountUVE() const noexcept;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render
