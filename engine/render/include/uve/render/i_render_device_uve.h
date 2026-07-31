//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "uve/render/buffer_handle_uve.h"
#include "uve/render/i_command_buffer_uve.h"
#include "uve/render/pipeline_handle_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/shader_handle_uve.h"
#include "uve/render/texture_handle_uve.h"

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

    /// Creates a shader per `desc`. Never returns kInvalidShaderHandleUVE on success.
    [[nodiscard]] virtual ShaderHandleUVE CreateShaderUVE(const ShaderDescUVE& desc) = 0;

    /// Destroys `shader`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyShaderUVE(ShaderHandleUVE shader) = 0;

    /// Creates a pipeline state object per `desc`. Returns kInvalidPipelineHandleUVE (logging the
    /// reason) if `desc.vertexShader`/`desc.fragmentShader` don't reference live shaders.
    [[nodiscard]] virtual PipelineHandleUVE CreatePipelineUVE(const PipelineDescUVE& desc) = 0;

    /// Destroys `pipeline`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyPipelineUVE(PipelineHandleUVE pipeline) = 0;

    /// Creates a new, empty ICommandBufferUVE ready for recording.
    [[nodiscard]] virtual std::unique_ptr<ICommandBufferUVE> CreateCommandBufferUVE() = 0;

    /// Submits a finished command buffer for execution. Consumes `commandBuffer` — it must not be
    /// used again after this call.
    virtual void SubmitUVE(std::unique_ptr<ICommandBufferUVE> commandBuffer) = 0;

    /// A short human-readable backend name (e.g. `"Null"`), for logging/diagnostics.
    [[nodiscard]] virtual std::string_view GetBackendNameUVE() const noexcept = 0;
};

} // namespace UVE::Render
