// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string_view>
#include <vector>

#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
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
    void SetUniformFloatUVE(std::string_view name, float value) override;
    void SetUniformIntUVE(std::string_view name, std::int32_t value) override;
    void SetUniformBoolUVE(std::string_view name, bool value) override;
    void SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) override;
    void SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) override;
    void DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) override;
    void DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) override;

    /// Every call recorded so far, in issue order.
    [[nodiscard]] const std::vector<RecordedCommandUVE>& GetRecordedCommandsUVE() const noexcept;

private:
    bool m_insideRenderPass = false;
    std::vector<RecordedCommandUVE> m_commands;
};

} // namespace UVE::Render
