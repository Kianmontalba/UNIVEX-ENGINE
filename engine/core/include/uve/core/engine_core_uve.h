//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "uve/core/engine_config_uve.h"
#include "uve/core/engine_services_uve.h"
#include "uve/core/engine_state_uve.h"
#include "uve/core/frame_stats_uve.h"
#include "uve/core/version_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/utilities/i_timer_uve.h"

namespace UVE::Core {

/// EngineCoreUVE owns the foundational engine services (Logger, MemoryManager,
/// Timer, EventSystem) and drives the canonical engine lifecycle: Init ->
/// Load -> N x (BeginFrame -> Update -> LateUpdate -> Render -> EndFrame) ->
/// Shutdown. This increment has no windowing/rendering backend yet, so
/// Render() is the documented, working no-op seam where RenderSystemUVE
/// will plug in once one exists — every stage does something real today,
/// none are placeholders.
/// Thread-safety: not thread-safe. Every method here is intended to be
/// called from a single "engine" thread. The services EngineCoreUVE owns
/// each document their own thread-safety contract independently (e.g.
/// LoggerUVE is safe to log to from other threads even though
/// EngineCoreUVE's own methods are not thread-safe).
class EngineCoreUVE final {
public:
    explicit EngineCoreUVE(EngineConfigUVE config = {});
    ~EngineCoreUVE();

    EngineCoreUVE(const EngineCoreUVE&) = delete;
    EngineCoreUVE& operator=(const EngineCoreUVE&) = delete;

    /// Constructs and initializes Logger, MemoryManager, Timer, and
    /// EventSystem in that order (Logger first — every later step and every
    /// other system may need to log or UVE_ASSERT during its own setup;
    /// MemoryManager next, since it is the next most foundational service
    /// future systems will depend on), then builds EngineServicesUVE from
    /// all four. Transitions Uninitialized -> Initializing -> Running.
    void Init();

    /// The engine's asset/subsystem loading hook. This increment has
    /// nothing to load, so it always succeeds — that is the complete,
    /// correct behavior for the one thing this stage owns today (state
    /// bookkeeping and logging), not a placeholder for a future one.
    [[nodiscard]] bool Load();

    /// Runs Init() -> Load() -> up to `frameCount` frames (stopping early
    /// if RequestQuitUVE() was called) -> Shutdown(). `frameCount` must be
    /// >= 0. Returns 0 on success, 1 if Load() failed. Deterministic and
    /// headless-friendly — the mode used by both the uve_runtime executable
    /// and the unit test suite.
    int RunUVE(int frameCount);

    /// Runs exactly one frame: BeginFrame -> Update -> LateUpdate -> Render
    /// -> EndFrame, in that order. Exposed publicly so tests can drive
    /// individual frames without a full RunUVE() loop. Must be called only
    /// while GetStateUVE() == EngineStateUVE::Running.
    void TickFrameUVE();

    /// Requests that a currently-running RunUVE() loop stop after the
    /// current frame completes, without running further frames.
    void RequestQuitUVE() noexcept;

    /// Transitions Running -> ShuttingDown -> Shutdown, tearing down
    /// EventSystem, then Timer, then MemoryManager (logging its leak
    /// report — and, in debug builds, UVE_ASSERTing zero active
    /// allocations — before it is destroyed), then Logger — the exact
    /// reverse of Init()'s construction order — logging the final message
    /// before the logger itself is torn down last.
    void Shutdown();

    [[nodiscard]] EngineStateUVE GetStateUVE() const noexcept;
    [[nodiscard]] const FrameStatsUVE& GetFrameStatsUVE() const noexcept;

    /// Returns the service container bundling Logger/Timer/EventSystem/
    /// MemoryManager references. Valid only between Init() and Shutdown().
    [[nodiscard]] EngineServicesUVE& GetServicesUVE();

    /// Returns this build's engine version — the single source of truth
    /// future systems (assets, plugins, projects, crash reports, Hub
    /// integration) are expected to read.
    [[nodiscard]] static VersionUVE GetEngineVersionUVE() noexcept;

private:
    /// Ticks the timer, advances the frame counter, and records this
    /// frame's start instant (used by EndFrame() to compute frameTime).
    void BeginFrame();

    /// Advances the fixed-timestep accumulator and dispatches every event
    /// queued via IEventSystemUVE::QueueEvent() since the last dispatch.
    void Update();

    /// Recomputes FrameStatsUVE::fps (an exponential moving average of
    /// 1/deltaTime). The documented hook point for future post-Update,
    /// pre-Render systems (camera follow, animation retargeting).
    void LateUpdate();

    /// No-op render seam: RenderSystemUVE will plug in here once windowing
    /// and a graphics backend exist.
    void Render();

    /// Computes this frame's wall-clock frameTimeSeconds and records it
    /// into FrameStatsUVE.
    void EndFrame();

    /// Asserts IsValidTransitionUVE(m_state, newState), then applies it.
    void TransitionStateUVE(EngineStateUVE newState);

    EngineConfigUVE m_config;
    EngineStateUVE m_state = EngineStateUVE::Uninitialized;

    std::unique_ptr<Debug::ILoggerUVE> m_logger;
    std::unique_ptr<Memory::IMemoryManagerUVE> m_memoryManager;
    std::unique_ptr<Utilities::ITimerUVE> m_timer;
    std::unique_ptr<Events::IEventSystemUVE> m_eventSystem;
    std::optional<EngineServicesUVE> m_services;

    FrameStatsUVE m_frameStats;
    std::chrono::steady_clock::time_point m_frameStartTime;
    bool m_quitRequested = false;
};

} // namespace UVE::Core
