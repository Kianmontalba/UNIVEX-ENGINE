//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_core_uve.h"

#include <utility>

#include "uve/debug/assert_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/threading/thread_pool_uve.h"
#include "uve/utilities/timer_uve.h"

namespace UVE::Core {

EngineCoreUVE::EngineCoreUVE(EngineConfigUVE config) : m_config(std::move(config)) {}

EngineCoreUVE::~EngineCoreUVE() {
    if (m_state == EngineStateUVE::Running) {
        Shutdown();
    }
}

void EngineCoreUVE::TransitionStateUVE(EngineStateUVE newState) {
    UVE_ASSERT(IsValidTransitionUVE(m_state, newState));
    m_state = newState;
}

void EngineCoreUVE::Init() {
    TransitionStateUVE(EngineStateUVE::Initializing);

    // Logger first: every later step below, and every other engine system,
    // may need to log or UVE_ASSERT during its own setup. See
    // docs/CODING_STANDARDS.md for the full init/shutdown ordering
    // rationale.
    auto logger = std::make_unique<Debug::LoggerUVE>();
    logger->Init(m_config.minLogLevel);
    if (m_config.enableConsoleLogging) {
        logger->AddSink(std::make_unique<Debug::ConsoleSinkUVE>());
    }
    logger->AddSink(std::make_unique<Debug::FileSinkUVE>(m_config.logFilePath));
    m_logger = std::move(logger);

    UVE_INFO("EngineCoreUVE: initializing UniVex Engine {}", GetEngineVersionUVE().ToStringUVE());

    // MemoryManager second: the next most foundational service after
    // logging — nothing constructed here has a hard dependency on it yet,
    // but future systems will, mirroring the rationale for Logger's own
    // position.
    m_memoryManager = std::make_unique<Memory::MemoryManagerUVE>();

    // ThreadPool third: sits alongside Logger/MemoryManager as a
    // foundational service (matching the spec's own Part 7.1 ordering,
    // which lists ThreadPoolUVE before EventSystemUVE/TimerUVE) and after
    // Logger/MemoryManager since its workers may immediately want to log
    // or allocate once real jobs start flowing through it.
    m_threadPool = std::make_unique<Threading::ThreadPoolUVE>(m_config.threadPoolWorkerCount);

    // Timer fourth: Update()/LateUpdate() depend on it; nothing constructed
    // here depends on EventSystem existing yet.
    auto timer = std::make_unique<Utilities::TimerUVE>();
    timer->Reset();
    timer->SetMaxDeltaTimeUVE(m_config.maxDeltaTimeSeconds);
    timer->SetFixedTimestepUVE(m_config.fixedUpdateFps > 0.0 ? (1.0 / m_config.fixedUpdateFps) : 0.0);
    m_timer = std::move(timer);

    // EventSystem fifth: it is the piece most likely to gain future
    // dependents (systems subscribing during their own Init()), so it is
    // constructed last among the five, once it is guaranteed nothing else
    // in this list still needs to be built.
    m_eventSystem = std::make_unique<Events::EventSystemUVE>();

    m_services.emplace(*m_logger, *m_timer, *m_eventSystem, *m_memoryManager, *m_threadPool);

    TransitionStateUVE(EngineStateUVE::Running);
    UVE_INFO("EngineCoreUVE: initialized");
}

bool EngineCoreUVE::Load() {
    UVE_INFO("EngineCoreUVE: Load() - nothing to load this increment");
    return true;
}

void EngineCoreUVE::BeginFrame() {
    m_frameStartTime = std::chrono::steady_clock::now();
    m_timer->Tick();
    ++m_frameStats.frameNumber;
    m_frameStats.deltaTimeSeconds = m_timer->GetDeltaTimeUVE();
    m_frameStats.totalTimeSeconds = m_timer->GetTotalTimeUVE();
    UVE_TRACE("BeginFrame {}", m_frameStats.frameNumber);
}

void EngineCoreUVE::Update() {
    const Utilities::FixedStepResultUVE fixedStep = m_timer->AdvanceFixedStepUVE();
    UVE_TRACE("Update: {} fixed step(s), alpha={}", fixedStep.stepsToRun, fixedStep.alpha);
    m_eventSystem->DispatchQueuedUVE();
}

void EngineCoreUVE::LateUpdate() {
    if (m_frameStats.deltaTimeSeconds > 0.0) {
        const double instantaneousFps = 1.0 / m_frameStats.deltaTimeSeconds;
        constexpr double kFpsSmoothingFactor = 0.1;
        m_frameStats.fps = (m_frameStats.fps <= 0.0)
                                ? instantaneousFps
                                : (m_frameStats.fps * (1.0 - kFpsSmoothingFactor) +
                                   instantaneousFps * kFpsSmoothingFactor);
    }
    UVE_TRACE("LateUpdate: fps={}", m_frameStats.fps);
}

void EngineCoreUVE::Render() {
    // No-op render seam: RenderSystemUVE will plug in here once windowing
    // and a graphics backend exist. Complete and correct as the render
    // stage for a build with no rendering backend.
    UVE_TRACE("Render (no-op)");
}

void EngineCoreUVE::EndFrame() {
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    m_frameStats.frameTimeSeconds = std::chrono::duration<double>(now - m_frameStartTime).count();
    UVE_TRACE("EndFrame {}: delta={} total={} fps={} frameTime={}", m_frameStats.frameNumber,
              m_frameStats.deltaTimeSeconds, m_frameStats.totalTimeSeconds, m_frameStats.fps,
              m_frameStats.frameTimeSeconds);
}

void EngineCoreUVE::TickFrameUVE() {
    UVE_ASSERT(m_state == EngineStateUVE::Running);
    BeginFrame();
    Update();
    LateUpdate();
    Render();
    EndFrame();
}

int EngineCoreUVE::RunUVE(int frameCount) {
    UVE_ASSERT(frameCount >= 0);
    Init();
    if (!Load()) {
        Shutdown();
        return 1;
    }
    for (int frameIndex = 0; frameIndex < frameCount && !m_quitRequested; ++frameIndex) {
        TickFrameUVE();
    }
    Shutdown();
    return 0;
}

void EngineCoreUVE::RequestQuitUVE() noexcept {
    m_quitRequested = true;
}

void EngineCoreUVE::Shutdown() {
    TransitionStateUVE(EngineStateUVE::ShuttingDown);
    UVE_INFO("EngineCoreUVE: shutting down");

    // Exact reverse of Init()'s construction order: EventSystem, then
    // Timer, then ThreadPool, then MemoryManager, then Logger. The final
    // log message is emitted before the logger itself is torn down, so it
    // is guaranteed to be recorded.
    m_services.reset();
    m_eventSystem->Clear();
    m_eventSystem.reset();
    m_timer.reset();

    // ThreadPoolUVE's destructor blocks until every worker has drained its
    // queue and joined — no jobs are silently dropped on shutdown.
    m_threadPool.reset();

    // Leak report must run while the logger is still alive; the debug-only
    // assertion turns a leak into an immediate development-time failure
    // without ever affecting Release builds.
    m_memoryManager->LogLeakReportUVE();
#if UVE_DEBUG
    UVE_ASSERT(m_memoryManager->GetActiveAllocationCountUVE() == 0);
#endif
    m_memoryManager.reset();

    UVE_INFO("EngineCoreUVE: shutdown complete");
    m_logger->Shutdown();
    m_logger.reset();

    TransitionStateUVE(EngineStateUVE::Shutdown);
}

EngineStateUVE EngineCoreUVE::GetStateUVE() const noexcept {
    return m_state;
}

const FrameStatsUVE& EngineCoreUVE::GetFrameStatsUVE() const noexcept {
    return m_frameStats;
}

EngineServicesUVE& EngineCoreUVE::GetServicesUVE() {
    UVE_ASSERT(m_services.has_value());
    return *m_services;
}

VersionUVE EngineCoreUVE::GetEngineVersionUVE() noexcept {
    return VersionUVE{0, 1, 0, 1};
}

} // namespace UVE::Core
