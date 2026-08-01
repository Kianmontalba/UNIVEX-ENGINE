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
#include <string>
#include <variant>

#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/render/buffer_handle_uve.h"
#include "uve/render/pipeline_handle_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/texture_handle_uve.h"

namespace UVE::Render {

/// One call recorded by NullCommandBufferUVE (engine/render/src/, module-private), in the exact
/// shape and order it was issued. Public (unlike NullCommandBufferUVE itself) because test code
/// — and NullRenderDeviceUVE::GetLastSubmittedCommandsUVE(), its one intentional test-only public
/// hook — needs to name this type to assert exactly what a real backend would have received.
struct BeginRenderPassCommandUVE {
    RenderPassDescUVE desc;
};
struct EndRenderPassCommandUVE {};
struct BindPipelineCommandUVE {
    PipelineHandleUVE pipeline;
};
struct BindVertexBufferCommandUVE {
    BufferHandleUVE buffer;
    std::uint32_t slot = 0;
};
struct BindIndexBufferCommandUVE {
    BufferHandleUVE buffer;
};
struct BindTextureCommandUVE {
    TextureHandleUVE texture;
    std::uint32_t slot = 0;
};
struct BindUniformBufferCommandUVE {
    BufferHandleUVE buffer;
    std::uint32_t slot = 0;
};
struct DrawIndexedCommandUVE {
    std::uint32_t indexCount = 0;
    std::uint32_t instanceCount = 1;
};
struct DrawCommandUVE {
    std::uint32_t vertexCount = 0;
    std::uint32_t instanceCount = 1;
};
struct SetUniformFloatCommandUVE {
    std::string name;
    float value = 0.0F;
};
struct SetUniformIntCommandUVE {
    std::string name;
    std::int32_t value = 0;
};
struct SetUniformBoolCommandUVE {
    std::string name;
    bool value = false;
};
struct SetUniformVector3CommandUVE {
    std::string name;
    Math::Vector3UVE value;
};
struct SetUniformMatrix4x4CommandUVE {
    std::string name;
    Math::Matrix4x4UVE value;
};

/// A single recorded ICommandBufferUVE call, tagged by which method it came from.
using RecordedCommandUVE =
    std::variant<BeginRenderPassCommandUVE, EndRenderPassCommandUVE, BindPipelineCommandUVE,
                 BindVertexBufferCommandUVE, BindIndexBufferCommandUVE, BindTextureCommandUVE,
                 BindUniformBufferCommandUVE, DrawIndexedCommandUVE, DrawCommandUVE, SetUniformFloatCommandUVE,
                 SetUniformIntCommandUVE, SetUniformBoolCommandUVE, SetUniformVector3CommandUVE,
                 SetUniformMatrix4x4CommandUVE>;

} // namespace UVE::Render
