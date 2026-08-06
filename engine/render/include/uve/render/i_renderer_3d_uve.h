// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// IRenderer3DUVE is the engine's final per-frame render orchestrator (the spec's `Renderer3DUVE`,
/// Part 7.2): for a given camera entity, it computes the view-projection, culls and sorts the
/// scene into a RenderQueueUVE (via IMeshRendererUVE), lazily uploads each encountered mesh's/
/// material's GPU-shaped resources (cached, invalidated on hot-reload), and records the resulting
/// draw calls into a real ICommandBufferUVE submitted through IRenderSystemUVE. Renders into an
/// internally-owned offscreen color+depth target only — no WindowManagerUVE/swapchain exists yet
/// in this sandbox (no display server, GPU device node, or graphics SDK headers), so there is no
/// present step. Adding one later is additive, not a redesign.
/// Thread-safety: not thread-safe. RenderFrameUVE() must be called once per frame from the main
/// engine thread only, after SceneGraphUVE::UpdateUVE() has run for this frame (it needs current
/// WorldTransformComponentUVE data), matching EngineCoreUVE's single-threaded Update -> Render
/// frame-loop contract.
class IRenderer3DUVE {
public:
    virtual ~IRenderer3DUVE() = default;

    /// Renders the scene as seen by `cameraEntity` into this Renderer3DUVE's offscreen target:
    /// extract -> sort -> record -> submit. `cameraEntity` must have both
    /// WorldTransformComponentUVE and CameraComponentUVE (the same contract ICameraSystemUVE
    /// already enforces).
    virtual void RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) = 0;
};

} // namespace UVE::Render
