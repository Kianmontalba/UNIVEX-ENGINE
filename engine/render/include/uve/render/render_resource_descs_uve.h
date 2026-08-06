// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "uve/render/shader_handle_uve.h"
#include "uve/render/texture_handle_uve.h"

namespace UVE::Render {

/// What a BufferDescUVE-created buffer is used for. `Storage` (arbitrary read/write buffers for
/// compute shaders) is deliberately omitted — ComputeSystemUVE (Part 7.2) doesn't exist yet;
/// adding it later is additive, not a breaking change to this enum.
enum class BufferUsageUVE : std::uint8_t { Vertex, Index, Uniform };

/// Describes a GPU buffer to create via IRenderDeviceUVE::CreateBufferUVE().
struct BufferDescUVE {
    std::uint64_t sizeBytes = 0;
    BufferUsageUVE usage = BufferUsageUVE::Vertex;
};

/// Pixel formats an IRenderDeviceUVE texture can use. `Depth32Float` exists here (even though no
/// loadable asset ever uses it — see the deliberately separate Asset::TextureFormatUVE) because
/// depth render targets are created directly through this RHI, never loaded from disk.
enum class TextureFormatUVE : std::uint8_t { RGBA8Unorm, RGBA16Float, Depth32Float };

/// Describes a GPU texture to create via IRenderDeviceUVE::CreateTextureUVE().
struct TextureDescUVE {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    TextureFormatUVE format = TextureFormatUVE::RGBA8Unorm;
    std::uint32_t mipLevels = 1;
};

/// Which programmable stage a ShaderDescUVE belongs to. `Compute` is reserved for the future
/// ComputeSystemUVE (Part 7.2) — unused by anything built so far. `Geometry` (Increment 21) is
/// compilable standalone via Shader::ShaderSourceUVE but, like Compute, has no pipeline slot yet
/// — PipelineDescUVE still only links a vertex+fragment pair; growing it with a geometry slot is
/// deferred future work, not built this increment. Append-only: never renumber existing values,
/// since ShaderStageUVE crosses the RHI boundary.
enum class ShaderStageUVE : std::uint8_t { Vertex, Fragment, Compute, Geometry };

/// Describes a shader to create via IRenderDeviceUVE::CreateShaderUVE(). `sourceCode` is stored
/// and validated as-is; NullRenderDeviceUVE never compiles it — no glslang/shaderc/DXC is
/// available in this environment (see docs/CODING_STANDARDS.md).
struct ShaderDescUVE {
    ShaderStageUVE stage = ShaderStageUVE::Vertex;
    std::string sourceCode;
    std::string entryPointName = "main";
};

/// The shape of one vertex attribute inside a PipelineDescUVE's vertex layout.
enum class VertexAttributeFormatUVE : std::uint8_t { Float2, Float3, Float4 };

/// One vertex attribute (e.g. "position", "normal", "uv") within a pipeline's vertex layout.
struct VertexAttributeUVE {
    std::string semanticName;
    VertexAttributeFormatUVE format = VertexAttributeFormatUVE::Float3;
    std::uint32_t offset = 0;
};

/// How a pipeline's bound vertex/index buffers are assembled into primitives. Only `Triangles`
/// exists today — lines/points aren't needed by anything built so far.
enum class PrimitiveTopologyUVE : std::uint8_t { Triangles };

/// Describes a pipeline state object to create via IRenderDeviceUVE::CreatePipelineUVE().
/// Fixed-function state is deliberately minimal (no blend mode, cull mode, or stencil) — will
/// grow once transparency/PostProcessUVE (a later, currently-blocked increment) actually needs
/// it; flagged as expected future growth, not a hidden gap.
struct PipelineDescUVE {
    ShaderHandleUVE vertexShader;
    ShaderHandleUVE fragmentShader;
    std::vector<VertexAttributeUVE> vertexLayout;
    PrimitiveTopologyUVE topology = PrimitiveTopologyUVE::Triangles;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;

    /// Byte distance between consecutive vertices in the bound vertex buffer — required by a real
    /// backend to interleave `vertexLayout`'s attributes correctly (e.g. GL's
    /// `glVertexAttribPointer` stride parameter). `0` (the default) is only ever valid for
    /// `NullRenderDeviceUVE`, which ignores this field like every other one it merely bookkeeps.
    std::uint32_t vertexStride = 0;
};

/// Fixed-function state accompanying a pre-compiled GL program binary passed to
/// IRenderDeviceUVE::CreatePipelineFromBinaryUVE() (Increment 21's program-binary cache). Deliberately
/// has no shader-handle fields — loading from binary skips shader-object attachment entirely, so
/// there is nothing analogous to PipelineDescUVE::vertexShader/fragmentShader here.
struct PipelineBinaryDescUVE {
    std::vector<VertexAttributeUVE> vertexLayout;
    std::uint32_t vertexStride = 0;
    PrimitiveTopologyUVE topology = PrimitiveTopologyUVE::Triangles;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
};

/// What happens to a render pass attachment's existing contents at the start of the pass.
enum class LoadOpUVE : std::uint8_t { Clear, Load, DontCare };

/// Describes one BeginRenderPassUVE() call: which color/depth textures are rendered into and how
/// they're cleared. `depthAttachment` may be `kInvalidTextureHandleUVE` for a color-only pass.
/// `colorAttachment` itself may also be `kInvalidTextureHandleUVE`, meaning "render into the
/// backend's default framebuffer" (the window's back buffer) rather than an offscreen texture —
/// a backend-specific meaning `GlRenderDeviceUVE` acts on (binding GL framebuffer object `0`);
/// `NullRenderDeviceUVE` ignores it like every other field it bookkeeps without interpreting.
struct RenderPassDescUVE {
    TextureHandleUVE colorAttachment;
    TextureHandleUVE depthAttachment;
    LoadOpUVE colorLoadOp = LoadOpUVE::Clear;
    std::array<float, 4> clearColor{0.0F, 0.0F, 0.0F, 1.0F};
    LoadOpUVE depthLoadOp = LoadOpUVE::Clear;
    float clearDepth = 1.0F;
};

} // namespace UVE::Render
