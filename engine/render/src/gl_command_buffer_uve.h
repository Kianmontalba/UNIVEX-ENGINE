// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "gl_render_device_state_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
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
    void SetUniformFloatUVE(std::string_view name, float value) override;
    void SetUniformIntUVE(std::string_view name, std::int32_t value) override;
    void SetUniformBoolUVE(std::string_view name, bool value) override;
    void SetUniformVector3UVE(std::string_view name, const Math::Vector3UVE& value) override;
    void SetUniformMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) override;
    void DrawIndexedUVE(std::uint32_t indexCount, std::uint32_t instanceCount) override;
    void DrawUVE(std::uint32_t vertexCount, std::uint32_t instanceCount) override;

private:
    using UniformRecordUVE = Detail::GlDeviceStateUVE::PipelineRecordUVE::UniformRecordUVE;

    /// Looks `name` up in the currently bound pipeline's reflected uniform map (cached at link
    /// time - see GlRenderDeviceUVE::CreatePipelineUVE()'s ReflectPipelineUniformsUVE() call), so
    /// every SetUniform*UVE() call is a plain hash-map lookup, never a per-draw
    /// glGetUniformLocation call. Returns nullptr (logging at WARNING, not ERROR - an optimized-
    /// out or simply absent uniform is a normal occurrence) if no pipeline is bound or `name`
    /// isn't one of its active uniforms.
    [[nodiscard]] const UniformRecordUVE* FindUniformUVE(std::string_view name) const;

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
    const std::unordered_map<std::string, UniformRecordUVE>* m_currentUniforms = nullptr;
};

} // namespace UVE::Render
