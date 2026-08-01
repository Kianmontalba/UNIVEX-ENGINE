//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <vector>

#include "gl_render_device_state_uve.h"
#include "uve/render/i_command_buffer_uve.h"

namespace UVE::Render {

/// GlCommandBufferUVE (engine/render/src/, module-private — never named outside engine/render/src/)
/// is the real, immediate-mode command buffer GlRenderDeviceUVE hands out: unlike
/// NullCommandBufferUVE's "record into a vector, replay never happens" spy pattern, every method
/// here issues real GL calls directly, since OpenGL has no separate record/replay submission step
/// the way Vulkan/D3D12 do — by the time GlRenderDeviceUVE::SubmitUVE() runs, the work already
/// executed during recording.
/// Thread-safety: not thread-safe, matching ICommandBufferUVE's documented single-recorder
/// contract.
class GlCommandBufferUVE final : public ICommandBufferUVE {
public:
    explicit GlCommandBufferUVE(Detail::GlDeviceStateUVE& state);

    void BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) override;
    void EndRenderPassUVE() override;
    void BindPipelineUVE(PipelineHandleUVE pipeline) override;
    void BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) override;
    void BindIndexBufferUVE(BufferHandleUVE buffer) override;
    void BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) override;
    void BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) override;
    void DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) override;
    void DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) override;

private:
    Detail::GlDeviceStateUVE* m_state;
    bool m_insideRenderPass = false;
    GLuint m_tempFramebuffer = 0; // Nonzero while a real-texture-backed pass is active; this pass'
                                   // FBO is created in BeginRenderPassUVE() and destroyed in
                                   // EndRenderPassUVE() rather than cached across frames — a
                                   // deliberate "Foundations"-quality simplification since nothing
                                   // in this increment is performance-sensitive (see
                                   // docs/CODING_STANDARDS.md).
    GLuint m_currentProgram = 0;
    GLuint m_currentVao = 0;
    const std::vector<VertexAttributeUVE>* m_currentVertexLayout = nullptr;
    std::uint32_t m_currentVertexStride = 0;
};

} // namespace UVE::Render
