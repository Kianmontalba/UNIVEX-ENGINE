// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/render/i_command_buffer_uve.h"
#include "uve/render/pipeline_handle_uve.h"
#include "uve/render/shader/shader_compile_diagnostics_uve.h"
#include "uve/render/uniform_reflection_uve.h"

namespace UVE::Render::Shader {

class ShaderManagerUVE;

/// A linked vertex+fragment program (a wrapper over PipelineHandleUVE) plus its reflected
/// uniforms and a small pending-uniform-value cache (Increment 21). Always handed out as a
/// std::shared_ptr<ShaderProgramUVE> by IShaderManagerUVE::CreateProgramUVE() — never constructed
/// directly. A hot-reload swap mutates this same object's handle/diagnostics/uniform table in
/// place; every currently-set pending uniform value survives the swap unchanged.
/// Thread-safety: not thread-safe — see ShaderSourceUVE's identical contract.
class ShaderProgramUVE final {
public:
    [[nodiscard]] PipelineHandleUVE GetPipelineHandleUVE() const noexcept { return m_pipeline; }
    [[nodiscard]] bool IsReadyUVE() const noexcept { return m_ready; }
    [[nodiscard]] bool IsValidUVE() const noexcept { return m_valid; }
    [[nodiscard]] const ShaderCompileDiagnosticsUVE& GetDiagnosticsUVE() const noexcept { return m_diagnostics; }
    [[nodiscard]] std::span<const UniformReflectionUVE> GetUniformsUVE() const noexcept { return m_uniforms; }
    [[nodiscard]] std::uint64_t GetContentHashUVE() const noexcept { return m_contentHash; }

    /// Looks up one reflected uniform by name, or std::nullopt if `name` isn't active on this
    /// program's linked shaders.
    [[nodiscard]] std::optional<UniformReflectionUVE> FindUniformUVE(std::string_view name) const;

    /// Queues a uniform value, applied the next time ApplyToUVE() runs (not immediately) — so a
    /// caller can set uniforms in any order, any number of times per frame, before the program is
    /// ever bound. Setting a name this program doesn't declare is harmless (the value is simply
    /// never applied; ApplyToUVE()'s underlying ICommandBufferUVE::SetUniform*UVE() call logs a
    /// low-severity warning for an unknown name, matching the RHI's own contract).
    void SetFloatUVE(std::string_view name, float value);
    void SetIntUVE(std::string_view name, std::int32_t value);
    void SetBoolUVE(std::string_view name, bool value);
    void SetVector3UVE(std::string_view name, const Math::Vector3UVE& value);
    void SetMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value);

    /// Binds this program's pipeline on `commandBuffer` and flushes every currently-queued
    /// uniform value onto it. Must be called inside a render pass, exactly where a raw
    /// BindPipelineUVE() call would go. A safe, logged no-op if !IsValidUVE() (e.g. still
    /// compiling, or its most recent compile failed) — never binds a stale/invalid handle.
    void ApplyToUVE(ICommandBufferUVE& commandBuffer) const;

private:
    friend class ShaderManagerUVE;
    ShaderProgramUVE() = default;

    using PendingUniformValueUVE = std::variant<float, std::int32_t, bool, Math::Vector3UVE, Math::Matrix4x4UVE>;

    PipelineHandleUVE m_pipeline = kInvalidPipelineHandleUVE;
    bool m_ready = false;
    bool m_valid = false;
    ShaderCompileDiagnosticsUVE m_diagnostics;
    std::vector<UniformReflectionUVE> m_uniforms;
    std::uint64_t m_contentHash = 0;
    std::unordered_map<std::string, PendingUniformValueUVE> m_pendingUniforms;
};

} // namespace UVE::Render::Shader
