// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "uve/render/buffer_handle_uve.h"
#include "uve/render/i_command_buffer_uve.h"
#include "uve/render/pipeline_handle_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/shader_handle_uve.h"
#include "uve/render/texture_handle_uve.h"
#include "uve/render/uniform_reflection_uve.h"

namespace UVE::Render {

/// IRenderDeviceUVE is the engine's backend-agnostic RHI (render hardware interface): the
/// "modern explicit" style (per the approved architecture decision), mirroring Vulkan/D3D12/
/// Metal directly — pipeline state objects, explicit render passes, and resources created/
/// destroyed by handle rather than through implicit global state. The only implementation this
/// sandbox can build and test is NullRenderDeviceUVE (engine/render); a real Vulkan/Metal/D3D12
/// backend needs SDK headers, a GPU, and windowing this environment doesn't have (see
/// docs/CODING_STANDARDS.md) and is future work once it does. Every future backend implements
/// exactly this interface, so nothing above the RHI (CameraSystemUVE, MeshRendererUVE,
/// Renderer3DUVE — later increments) needs to change when one arrives.
/// Thread-safety: implementation-defined; NullRenderDeviceUVE documents its own contract. Callers
/// should assume a render device is only safe to use from the main engine/render thread unless a
/// concrete implementation states otherwise.
class IRenderDeviceUVE {
public:
    virtual ~IRenderDeviceUVE() = default;

    /// Creates a GPU buffer per `desc`, optionally uploading `initialData` (must be no larger
    /// than `desc.sizeBytes`). Never returns kInvalidBufferHandleUVE on success.
    [[nodiscard]] virtual BufferHandleUVE CreateBufferUVE(const BufferDescUVE& desc,
                                                           std::span<const std::byte> initialData = {}) = 0;

    /// Destroys `buffer`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyBufferUVE(BufferHandleUVE buffer) = 0;

    /// Overwrites `buffer`'s contents at `offsetBytes` with `data`. Returns false (logging the
    /// reason) if `buffer` is unknown or the write would exceed the buffer's size.
    [[nodiscard]] virtual bool UpdateBufferUVE(BufferHandleUVE buffer, std::span<const std::byte> data,
                                                std::uint64_t offsetBytes = 0) = 0;

    /// Creates a GPU texture per `desc`, optionally uploading `initialData`. Never returns
    /// kInvalidTextureHandleUVE on success.
    [[nodiscard]] virtual TextureHandleUVE CreateTextureUVE(const TextureDescUVE& desc,
                                                             std::span<const std::byte> initialData = {}) = 0;

    /// Destroys `texture`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyTextureUVE(TextureHandleUVE texture) = 0;

    /// Creates a shader per `desc`. Never returns kInvalidShaderHandleUVE on success. If
    /// `outInfoLog` is non-null, the backend's raw compile info log is written to it regardless
    /// of success/failure (Increment 21 — Shader::ShaderManagerUVE parses this into structured,
    /// line-numbered diagnostics; a successful compile can still produce non-fatal warning text).
    /// NullRenderDeviceUVE never populates `outInfoLog` (leaves it untouched) — it never really
    /// compiles anything, so it has no log to give.
    [[nodiscard]] virtual ShaderHandleUVE CreateShaderUVE(const ShaderDescUVE& desc,
                                                           std::string* outInfoLog = nullptr) = 0;

    /// Destroys `shader`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyShaderUVE(ShaderHandleUVE shader) = 0;

    /// Creates a pipeline state object per `desc`. Returns kInvalidPipelineHandleUVE (logging the
    /// reason) if `desc.vertexShader`/`desc.fragmentShader` don't reference live shaders. Same
    /// `outInfoLog` contract as CreateShaderUVE(), but for the link step's info log.
    [[nodiscard]] virtual PipelineHandleUVE CreatePipelineUVE(const PipelineDescUVE& desc,
                                                               std::string* outInfoLog = nullptr) = 0;

    /// Destroys `pipeline`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyPipelineUVE(PipelineHandleUVE pipeline) = 0;

    /// Returns every uniform `pipeline`'s shaders declare, reflected once at link time (Increment
    /// 21). Empty for an unknown handle. NullRenderDeviceUVE always returns empty — it never
    /// really links anything, so it has nothing to reflect.
    [[nodiscard]] virtual std::vector<UniformReflectionUVE> GetPipelineUniformsUVE(
        PipelineHandleUVE pipeline) const = 0;

    /// Retrieves `pipeline`'s compiled GL program binary (Increment 21's on-disk shader cache),
    /// writing it to `outBinary` and the backend-specific binary format to `outFormat`. Returns
    /// false (leaving both out-params untouched) if `pipeline` is unknown or the backend/driver
    /// can't produce a binary — always false for NullRenderDeviceUVE, since it never compiles
    /// anything real.
    [[nodiscard]] virtual bool GetPipelineBinaryUVE(PipelineHandleUVE pipeline, std::vector<std::byte>& outBinary,
                                                     std::uint32_t& outFormat) const = 0;

    /// Creates a pipeline directly from a previously retrieved GetPipelineBinaryUVE() blob and
    /// its `format`, skipping shader compilation/linking entirely — the fast path Shader::
    /// ShaderManagerUVE's on-disk cache uses on a cache hit. Returns kInvalidPipelineHandleUVE
    /// (logging the reason) if the backend/driver rejects `binary` — e.g. a stale cache entry
    /// from before a driver update — which callers must treat as an ordinary cache miss, never a
    /// hard failure. NullRenderDeviceUVE always succeeds, bookkeeping `desc`'s fixed-function
    /// state exactly like CreatePipelineUVE() does (with invalid shader handles, since none was
    /// ever compiled).
    [[nodiscard]] virtual PipelineHandleUVE CreatePipelineFromBinaryUVE(std::span<const std::byte> binary,
                                                                         std::uint32_t format,
                                                                         const PipelineBinaryDescUVE& desc) = 0;

    /// Creates a new, empty ICommandBufferUVE ready for recording.
    [[nodiscard]] virtual std::unique_ptr<ICommandBufferUVE> CreateCommandBufferUVE() = 0;

    /// Submits a finished command buffer for execution. Consumes `commandBuffer` — it must not be
    /// used again after this call.
    virtual void SubmitUVE(std::unique_ptr<ICommandBufferUVE> commandBuffer) = 0;

    /// Presents the backend's default framebuffer (the window's back buffer), analogous to a
    /// real Vulkan/D3D12 swapchain's present call — a distinct, explicit step after command
    /// buffer submission, matching this RHI's own "modern explicit" design. Call once per frame,
    /// after SubmitUVE(). NullRenderDeviceUVE's implementation is a bookkeeping no-op (see
    /// GetPresentCallCountUVE()); a windowless backend that never renders to a window has nothing
    /// meaningful to do here either.
    virtual void PresentUVE() = 0;

    /// A short human-readable backend name (e.g. `"Null"`), for logging/diagnostics.
    [[nodiscard]] virtual std::string_view GetBackendNameUVE() const noexcept = 0;
};

} // namespace UVE::Render
