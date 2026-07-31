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

#include "uve/render/i_command_buffer_uve.h"
#include "uve/render/recorded_command_uve.h"

namespace UVE::Render {

/// NullCommandBufferUVE (engine/render, module-private — never named outside engine/render/src/)
/// is the "spy" retained command buffer NullRenderDeviceUVE hands out: every call is appended, in
/// order, to an internal std::vector<RecordedCommandUVE> instead of touching any GPU (there is
/// none in this sandbox). NullRenderDeviceUVE::SubmitUVE() reads the finished list via
/// GetRecordedCommandsUVE() before the buffer is destroyed, so
/// NullRenderDeviceUVE::GetLastSubmittedCommandsUVE() can hand it to tests, letting them assert
/// exactly what a real backend would have received.
/// Thread-safety: not thread-safe, matching ICommandBufferUVE's documented single-recorder
/// contract.
class NullCommandBufferUVE final : public ICommandBufferUVE {
public:
    void BeginRenderPassUVE(const RenderPassDescUVE& renderPassDesc) override;
    void EndRenderPassUVE() override;
    void BindPipelineUVE(PipelineHandleUVE pipeline) override;
    void BindVertexBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) override;
    void BindIndexBufferUVE(BufferHandleUVE buffer) override;
    void BindTextureUVE(TextureHandleUVE texture, std::uint32_t slot) override;
    void BindUniformBufferUVE(BufferHandleUVE buffer, std::uint32_t slot) override;
    void DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) override;
    void DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) override;

    /// Every call recorded so far, in issue order.
    [[nodiscard]] const std::vector<RecordedCommandUVE>& GetRecordedCommandsUVE() const noexcept;

private:
    bool m_insideRenderPass = false;
    std::vector<RecordedCommandUVE> m_commands;
};

} // namespace UVE::Render
