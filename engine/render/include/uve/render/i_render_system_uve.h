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

#include "uve/render/i_command_buffer_uve.h"

namespace UVE::Render {

/// IRenderSystemUVE is the engine's top-level per-frame render orchestrator (the spec's
/// `RenderSystemUVE`, Part 7.2 — "Abstract rendering backend"): it owns the one command buffer a
/// frame records into and drives its lifecycle against an injected IRenderDeviceUVE. Later
/// increments (CameraSystemUVE, MeshRendererUVE, Renderer3DUVE) record draw calls into the buffer
/// this returns; IRenderSystemUVE itself knows nothing about cameras, meshes, or materials.
/// Thread-safety: not thread-safe. BeginFrameUVE()/GetFrameCommandBufferUVE()/EndFrameUVE() must
/// be called only from the main engine thread, once per frame, matching EngineCoreUVE's
/// single-threaded frame-loop contract.
class IRenderSystemUVE {
public:
    virtual ~IRenderSystemUVE() = default;

    /// Begins a new frame: creates a fresh command buffer for GetFrameCommandBufferUVE() to
    /// return. Must not be called while a frame is already active.
    virtual void BeginFrameUVE() = 0;

    /// Returns the current frame's command buffer for recording. Must be called only between
    /// BeginFrameUVE() and EndFrameUVE().
    [[nodiscard]] virtual ICommandBufferUVE& GetFrameCommandBufferUVE() = 0;

    /// Submits the current frame's command buffer and ends the frame. Must be called exactly
    /// once per BeginFrameUVE().
    virtual void EndFrameUVE() = 0;

    /// How many frames have completed (EndFrameUVE() returned) so far. Zero before the first
    /// EndFrameUVE() call.
    [[nodiscard]] virtual std::uint64_t GetFrameIndexUVE() const noexcept = 0;
};

} // namespace UVE::Render
