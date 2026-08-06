// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/shader/shader_program_uve.h"

#include <algorithm>
#include <type_traits>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render::Shader {

std::optional<UniformReflectionUVE> ShaderProgramUVE::FindUniformUVE(std::string_view name) const {
    const auto it = std::find_if(m_uniforms.begin(), m_uniforms.end(),
                                  [&](const UniformReflectionUVE& uniform) { return uniform.name == name; });
    if (it == m_uniforms.end()) {
        return std::nullopt;
    }
    return *it;
}

void ShaderProgramUVE::SetFloatUVE(std::string_view name, float value) {
    m_pendingUniforms[std::string(name)] = value;
}

void ShaderProgramUVE::SetIntUVE(std::string_view name, std::int32_t value) {
    m_pendingUniforms[std::string(name)] = value;
}

void ShaderProgramUVE::SetBoolUVE(std::string_view name, bool value) {
    m_pendingUniforms[std::string(name)] = value;
}

void ShaderProgramUVE::SetVector3UVE(std::string_view name, const Math::Vector3UVE& value) {
    m_pendingUniforms[std::string(name)] = value;
}

void ShaderProgramUVE::SetMatrix4x4UVE(std::string_view name, const Math::Matrix4x4UVE& value) {
    m_pendingUniforms[std::string(name)] = value;
}

void ShaderProgramUVE::ApplyToUVE(ICommandBufferUVE& commandBuffer) const {
    if (!m_valid) {
        UVE_WARNING("ShaderProgramUVE: ApplyToUVE called on a program that never linked successfully - skipping");
        return;
    }

    commandBuffer.BindPipelineUVE(m_pipeline);
    for (const auto& [name, value] : m_pendingUniforms) {
        std::visit(
            [&](const auto& concreteValue) {
                using ValueTypeUVE = std::decay_t<decltype(concreteValue)>;
                if constexpr (std::is_same_v<ValueTypeUVE, float>) {
                    commandBuffer.SetUniformFloatUVE(name, concreteValue);
                } else if constexpr (std::is_same_v<ValueTypeUVE, std::int32_t>) {
                    commandBuffer.SetUniformIntUVE(name, concreteValue);
                } else if constexpr (std::is_same_v<ValueTypeUVE, bool>) {
                    commandBuffer.SetUniformBoolUVE(name, concreteValue);
                } else if constexpr (std::is_same_v<ValueTypeUVE, Math::Vector3UVE>) {
                    commandBuffer.SetUniformVector3UVE(name, concreteValue);
                } else if constexpr (std::is_same_v<ValueTypeUVE, Math::Matrix4x4UVE>) {
                    commandBuffer.SetUniformMatrix4x4UVE(name, concreteValue);
                }
            },
            value);
    }
}

} // namespace UVE::Render::Shader
