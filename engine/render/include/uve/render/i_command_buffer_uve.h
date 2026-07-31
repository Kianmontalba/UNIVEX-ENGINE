//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "uve/render/buffer_handle_uve.h"
#include "uve/render/pipeline_handle_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/texture_handle_uve.h"

namespace UVE::Render {

/// ICommandBufferUVE is a **retained** command buffer (per the approved architecture decision):
/// a caller records a sequence of render-pass/bind/draw calls into it, then hands the finished
/// object to IRenderDeviceUVE::SubmitUVE() as a batch — mirroring Vulkan/D3D12/Metal's explicit
/// command-list model (the spec's actual named target backends) rather than issuing draw calls
/// immediately as the scene is walked. Obtained via IRenderDeviceUVE::CreateCommandBufferUVE(),
/// never constructed directly.
/// Thread-safety: not thread-safe. A command buffer is recorded by a single thread and handed to
/// SubmitUVE() exactly once; recording the same instance from multiple threads concurrently, or
/// reusing it after submission, is undefined. Parallel recording across ThreadPoolUVE workers
/// (e.g. one secondary command buffer per worker, merged at submission) is a future increment,
/// not built here.
class ICommandBufferUVE {
public:
    virtual ~ICommandBufferUVE() = default;

    /// Begins a render pass targeting `renderPassDesc`'s attachments. Must not be called while
    /// already inside a render pass (no nested passes).
    virtual void BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) = 0;

    /// Ends the current render pass. Must be called exactly once per BeginRenderPassUVE().
    virtual void EndRenderPassUVE() = 0;

    /// Binds `pipeline` as the active pipeline state for subsequent draw calls. Must be called
    /// inside a render pass.
    virtual void BindPipelineUVE(PipelineHandleUVE pipeline) = 0;

    /// Binds `buffer` as the vertex buffer at `slot`. Must be called inside a render pass.
    virtual void BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot = 0) = 0;

    /// Binds `buffer` as the index buffer for subsequent DrawIndexedUVE() calls. Must be called
    /// inside a render pass.
    virtual void BindIndexBufferUVE(BufferHandleUVE buffer) = 0;

    /// Binds `texture` at `slot` for the active pipeline's shaders. Must be called inside a
    /// render pass.
    virtual void BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) = 0;

    /// Binds `buffer` as a uniform buffer at `slot` for the active pipeline's shaders. Must be
    /// called inside a render pass.
    virtual void BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) = 0;

    /// Draws using the currently bound index buffer. Must be called inside a render pass, after
    /// a pipeline and the buffers it needs are bound.
    virtual void DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount = 1) = 0;

    /// Draws without an index buffer. Must be called inside a render pass, after a pipeline and
    /// the buffers it needs are bound.
    virtual void DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount = 1) = 0;
};

} // namespace UVE::Render
