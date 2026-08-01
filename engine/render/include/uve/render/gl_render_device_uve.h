//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <memory>

#include "uve/render/i_render_device_uve.h"
#include "uve/window/i_window_manager_uve.h"

namespace UVE::Render {

/// GlRenderDeviceUVE is the real, OpenGL 4.6 Core Profile-targeting implementation of
/// IRenderDeviceUVE. Constructed strictly *after* an IWindowManagerUVE& that has already created
/// its window and made its GL context current — GlRenderDeviceUVE never creates, destroys, or
/// activates a GL context itself (see docs/CODING_STANDARDS.md, "WindowManagerUVE owns the GL
/// context lifecycle"); it only loads GL function pointers (via a small hand-rolled loader —
/// gl_functions_uve.h/.cpp, module-private) and issues GL calls against the context that already
/// exists. Every GL header (`<GL/gl.h>`, `<GL/glext.h>`) and every GLuint/GLenum type is confined
/// to engine/render/src/ — this header, like every other public IRenderDeviceUVE consumer's, only
/// ever sees the same backend-agnostic RHI types NullRenderDeviceUVE does.
/// Thread-safety: not thread-safe, matching IRenderDeviceUVE's own documented contract; every
/// method is intended to be called only from the main engine/render thread.
class GlRenderDeviceUVE final : public IRenderDeviceUVE {
public:
    /// `windowManager` must outlive this GlRenderDeviceUVE, must already have a current GL
    /// context (IsValidUVE() == true), and its window must never be destroyed before this
    /// GlRenderDeviceUVE is — every GL object this device owns must be destroyed while the
    /// context windowManager owns is still valid.
    explicit GlRenderDeviceUVE(Window::IWindowManagerUVE& windowManager);
    ~GlRenderDeviceUVE() override;

    GlRenderDeviceUVE(const GlRenderDeviceUVE&) = delete;
    GlRenderDeviceUVE& operator=(const GlRenderDeviceUVE&) = delete;

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
    void PresentUVE() override;

    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override;

    /// Test-only hook (not part of IRenderDeviceUVE): how many buffer/texture/shader/pipeline
    /// resources are currently alive — mirrors NullRenderDeviceUVE::GetLiveResourceCountUVE()'s
    /// role so tests can confirm cleanup the same way regardless of backend.
    [[nodiscard]] std::size_t GetLiveResourceCountUVE() const noexcept;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render
