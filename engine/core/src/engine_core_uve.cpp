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

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/asset/hot_reload_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/shader_asset_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/commandline/command_line_uve.h"
#include "uve/config/config_manager_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/physics/physics_system_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/render/camera_system_uve.h"
#include "uve/render/mesh_renderer_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/render_system_uve.h"
#include "uve/render/renderer_3d_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/prefab_system_uve.h"
#include "uve/scene/scene_graph_uve.h"
#include "uve/scene/scene_serializer_uve.h"
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

void EngineCoreUVE::RegisterBuiltInAssetLoadersUVE() {
    m_assetManager->RegisterLoaderUVE<Asset::MeshAssetUVE>(&Asset::LoadMeshAssetUVE);
    m_assetManager->RegisterLoaderUVE<Asset::TextureAssetUVE>(&Asset::LoadTextureAssetUVE);
    m_assetManager->RegisterLoaderUVE<Asset::ShaderAssetUVE>(&Asset::LoadShaderAssetUVE);
    m_assetManager->RegisterLoaderUVE<Asset::MaterialAssetUVE>(&Asset::LoadMaterialAssetUVE);
}

void EngineCoreUVE::Init() {
    TransitionStateUVE(EngineStateUVE::Initializing);

    // CommandLine first: it has zero dependencies (pure parsing of the
    // args already captured in EngineConfigUVE), and its parsed flags are
    // the kind of thing a later step could plausibly want during its own
    // setup in a future increment.
    m_commandLine = std::make_unique<CommandLine::CommandLineUVE>(m_config.commandLineArgs);

    // Logger second: every later step below, and every other engine
    // system, may need to log or UVE_ASSERT during its own setup. See
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

    // MemoryManager third: the next most foundational service after
    // logging — nothing constructed here has a hard dependency on it yet,
    // but future systems will, mirroring the rationale for Logger's own
    // position.
    m_memoryManager = std::make_unique<Memory::MemoryManagerUVE>();

    // ThreadPool fourth: sits alongside Logger/MemoryManager as a
    // foundational service (matching the spec's own Part 7.1 ordering,
    // which lists ThreadPoolUVE before EventSystemUVE/TimerUVE) and after
    // Logger/MemoryManager since its workers may immediately want to log
    // or allocate once real jobs start flowing through it.
    m_threadPool = std::make_unique<Threading::ThreadPoolUVE>(m_config.threadPoolWorkerCount);

    // Timer fifth: Update()/LateUpdate() depend on it; nothing constructed
    // here depends on EventSystem existing yet.
    auto timer = std::make_unique<Utilities::TimerUVE>();
    timer->Reset();
    timer->SetMaxDeltaTimeUVE(m_config.maxDeltaTimeSeconds);
    timer->SetFixedTimestepUVE(m_config.fixedUpdateFps > 0.0 ? (1.0 / m_config.fixedUpdateFps) : 0.0);
    m_timer = std::move(timer);

    // EventSystem sixth: it is the piece most likely to gain future
    // dependents (systems subscribing during their own Init()), so it is
    // constructed after every other foundational service except
    // EntityManager/SceneGraph/ConfigManager, once it is guaranteed nothing
    // else in this list still needs to be built.
    m_eventSystem = std::make_unique<Events::EventSystemUVE>();

    // EntityManager seventh: needs MemoryManager (for chunk allocation) and
    // EventSystem (for entity lifecycle events), so it is built right after
    // both exist.
    m_entityManager = std::make_unique<Scene::EntityManagerUVE>(
        m_memoryManager->GetDefaultAllocatorUVE(), *m_eventSystem);

    // SceneGraph eighth: has no dependencies of its own; grouped
    // immediately after EntityManager for readability.
    m_sceneGraph = std::make_unique<Scene::SceneGraphUVE>();

    // AssetDatabase ninth: needs only Logger, which already exists. Its
    // LoadUVE() call logs its outcome (missing/malformed/success) the same
    // way ConfigManager's does below.
    auto assetDatabase = std::make_unique<Asset::AssetDatabaseUVE>();
    assetDatabase->LoadUVE(m_config.assetDatabaseFilePath);
    m_assetDatabase = std::move(assetDatabase);

    // SceneSerializer and PrefabSystem tenth/eleventh: both stateless,
    // grouped immediately after AssetDatabase for readability.
    m_sceneSerializer = std::make_unique<Scene::SceneSerializerUVE>();
    m_prefabSystem = std::make_unique<Scene::PrefabSystemUVE>();

    // HotReload twelfth: needs only EventSystem. Constructed before
    // AssetManager (not after, as its Part 7.4 spec-listing order might
    // suggest) purely because AssetManager's constructor takes a
    // HotReloadUVE* — construction must stay strictly forward-dependency.
    m_hotReload = std::make_unique<Asset::HotReloadUVE>(*m_eventSystem, m_config.hotReloadPollIntervalSecondsUVE);

    // AssetManager thirteenth: needs ThreadPool (to run loads) and
    // EventSystem (to publish AssetLoadCompletedEventUVE), and is always
    // given a real HotReloadUVE* (never null) — EngineConfigUVE::
    // hotReloadEnabledUVE only gates whether Update() calls PollUVE(), not
    // whether HotReloadUVE exists at all.
    m_assetManager = std::make_unique<Asset::AssetManagerUVE>(*m_threadPool, *m_eventSystem, m_hotReload.get());
    RegisterBuiltInAssetLoadersUVE();

    // AssetImporter and AssetBundle fourteenth/fifteenth: both stateless,
    // grouped immediately after AssetManager for readability.
    m_assetImporter = std::make_unique<Asset::AssetImporterUVE>();
    m_assetBundle = std::make_unique<Asset::AssetBundleUVE>();

    // FileSystem sixteenth: needs AssetBundle, since its bundle-backed
    // mounts read entries through IAssetBundleUVE.
    m_fileSystem = std::make_unique<Asset::FileSystemUVE>(*m_assetBundle);

    // RenderDevice seventeenth: no dependencies of its own. Always a
    // NullRenderDeviceUVE — no real GPU/windowing backend is buildable in
    // this sandbox (no display server, GPU device node, or graphics SDK
    // headers; see docs/CODING_STANDARDS.md).
    m_renderDevice = std::make_unique<Render::NullRenderDeviceUVE>();

    // RenderSystem eighteenth: needs RenderDevice, since it records and
    // submits command buffers through it.
    m_renderSystem = std::make_unique<Render::RenderSystemUVE>(*m_renderDevice);

    // CameraSystem nineteenth: stateless, no dependencies of its own —
    // grouped with the rest of engine/render.
    m_cameraSystem = std::make_unique<Render::CameraSystemUVE>();

    // MeshRenderer twentieth: stateless, no dependencies of its own —
    // grouped with the rest of engine/render.
    m_meshRenderer = std::make_unique<Render::MeshRendererUVE>();

    // Renderer3D twenty-first: needs RenderDevice, RenderSystem, MeshRenderer, CameraSystem,
    // AssetManager, AssetDatabase, and EventSystem — every one of which already exists by this
    // point. Its offscreen render target is fixed at EngineConfigUVE::renderTargetWidth/Height
    // for this EngineCoreUVE's lifetime (no WindowManagerUVE exists yet to resize against).
    m_renderer3D = std::make_unique<Render::Renderer3DUVE>(*m_renderDevice, *m_renderSystem, *m_meshRenderer,
                                                             *m_cameraSystem, *m_assetManager, *m_assetDatabase,
                                                             *m_eventSystem, m_config.renderTargetWidth,
                                                             m_config.renderTargetHeight);

    // CollisionSystem twenty-second: stateless, no dependencies of its own.
    m_collisionSystem = std::make_unique<Physics::CollisionSystemUVE>();

    // PhysicsSystem twenty-third: needs only CollisionSystem (composed by reference) and
    // EngineConfigUVE::gravity, both already available.
    m_physicsSystem = std::make_unique<Physics::PhysicsSystemUVE>(*m_collisionSystem, m_config.gravity);

    // ConfigManager last: it immediately calls LoadUVE(), which logs its
    // outcome (missing/malformed/success) through the Logger constructed
    // above — so Logger must already exist by this point.
    auto configManager = std::make_unique<Config::ConfigManagerUVE>();
    configManager->LoadUVE(m_config.settingsFilePath);
    m_configManager = std::move(configManager);

    m_services.emplace(*m_logger, *m_timer, *m_eventSystem, *m_memoryManager, *m_threadPool,
                        *m_commandLine, *m_configManager, *m_entityManager, *m_sceneGraph,
                        *m_assetDatabase, *m_sceneSerializer, *m_prefabSystem, *m_hotReload,
                        *m_assetManager, *m_assetImporter, *m_assetBundle, *m_fileSystem,
                        *m_renderDevice, *m_renderSystem, *m_cameraSystem, *m_meshRenderer,
                        *m_renderer3D, *m_collisionSystem, *m_physicsSystem);

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

    const float fixedDeltaTimeSeconds =
        m_config.fixedUpdateFps > 0.0 ? static_cast<float>(1.0 / m_config.fixedUpdateFps) : 0.0F;
    for (int step = 0; step < fixedStep.stepsToRun; ++step) {
        m_physicsSystem->StepUVE(*m_entityManager, *m_sceneGraph, fixedDeltaTimeSeconds);
    }

    m_sceneGraph->UpdateUVE(*m_entityManager);

    if (m_config.hotReloadEnabledUVE) {
        m_hotReload->PollUVE(*m_assetManager, *m_assetDatabase, m_timer->GetDeltaTimeUVE());
    }
    m_assetManager->CollectGarbageUVE();
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
    if (m_activeCamera != Scene::kInvalidEntityUVE) {
        m_renderer3D->RenderFrameUVE(*m_entityManager, m_activeCamera);
    } else {
        UVE_TRACE("Render (no-op)");
    }
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

    // Exact reverse of Init()'s construction order: ConfigManager, then
    // PhysicsSystem, then CollisionSystem, then Renderer3D, then MeshRenderer, then CameraSystem, then RenderSystem, then RenderDevice,
    // then FileSystem, then AssetBundle, then AssetImporter, then
    // AssetManager (its destructor blocks until every in-flight load job
    // finishes), then HotReload, then PrefabSystem, then SceneSerializer,
    // then AssetDatabase, then SceneGraph, then EntityManager, then
    // EventSystem, then Timer, then ThreadPool, then MemoryManager, then
    // Logger, then CommandLine. The final log message is emitted before the
    // logger itself is torn down, so it is guaranteed to be recorded.
    m_services.reset();
    m_configManager.reset();
    m_physicsSystem.reset();
    m_collisionSystem.reset();
    m_renderer3D.reset();
    m_meshRenderer.reset();
    m_cameraSystem.reset();
    m_renderSystem.reset();
    m_renderDevice.reset();
    m_fileSystem.reset();
    m_assetBundle.reset();
    m_assetImporter.reset();
    m_assetManager.reset();
    m_hotReload.reset();
    m_prefabSystem.reset();
    m_sceneSerializer.reset();
    m_assetDatabase.reset();
    m_sceneGraph.reset();

    // EntityManagerUVE's destructor frees every remaining live entity's
    // component memory — this must happen before MemoryManager's leak
    // report below, or a perfectly correct program would show false-
    // positive leaks for every still-alive entity at shutdown.
    m_entityManager.reset();

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
    m_commandLine.reset();

    TransitionStateUVE(EngineStateUVE::Shutdown);
}

EngineStateUVE EngineCoreUVE::GetStateUVE() const noexcept {
    return m_state;
}

const FrameStatsUVE& EngineCoreUVE::GetFrameStatsUVE() const noexcept {
    return m_frameStats;
}

void EngineCoreUVE::SetActiveCameraUVE(Scene::EntityUVE cameraEntity) noexcept {
    m_activeCamera = cameraEntity;
}

Scene::EntityUVE EngineCoreUVE::GetActiveCameraUVE() const noexcept {
    return m_activeCamera;
}

EngineServicesUVE& EngineCoreUVE::GetServicesUVE() {
    UVE_ASSERT(m_services.has_value());
    return *m_services;
}

VersionUVE EngineCoreUVE::GetEngineVersionUVE() noexcept {
    return VersionUVE{0, 1, 0, 1};
}

} // namespace UVE::Core
