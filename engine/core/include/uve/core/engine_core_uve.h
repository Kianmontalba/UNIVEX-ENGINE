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

#include "uve/asset/i_asset_bundle_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_importer_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/asset/i_file_system_uve.h"
#include "uve/asset/i_hot_reload_uve.h"
#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/core/engine_config_uve.h"
#include "uve/core/engine_services_uve.h"
#include "uve/core/engine_state_uve.h"
#include "uve/core/frame_stats_uve.h"
#include "uve/core/version_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_mesh_renderer_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/render/i_renderer_3d_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"

namespace UVE::Core {

/// EngineCoreUVE owns the foundational engine services (CommandLine, Logger,
/// MemoryManager, ThreadPool, Timer, EventSystem, EntityManager, SceneGraph,
/// AssetDatabase, SceneSerializer, PrefabSystem, HotReload, AssetManager,
/// AssetImporter, AssetBundle, FileSystem, RenderDevice, RenderSystem,
/// CameraSystem, MeshRenderer, Renderer3D, CollisionSystem, PhysicsSystem,
/// ConfigManager) and drives the canonical engine lifecycle: Init -> Load ->
/// N x (BeginFrame -> Update -> LateUpdate -> Render -> EndFrame) ->
/// Shutdown. Render() calls Renderer3DUVE::RenderFrameUVE() (extract -> cull
/// -> sort -> record -> submit) whenever SetActiveCameraUVE() has set a
/// valid camera entity; with no active camera set (the default, and every
/// test/sample app predating Increment 14), it still logs the original
/// no-op trace line, so existing frame-loop behavior is byte-identical
/// unless a caller opts in. Update() runs zero or more fixed PhysicsSystemUVE
/// steps (via Utilities::FixedStepResultUVE) before SceneGraphUVE::UpdateUVE()
/// each frame — entirely data-driven off which entities have a
/// RigidBodyComponentUVE/ColliderComponentUVE, so a scene with none behaves
/// exactly as it did before Increment 15, no opt-in needed.
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

    /// Constructs and initializes CommandLine, Logger, MemoryManager,
    /// ThreadPool, Timer, EventSystem, EntityManager, SceneGraph,
    /// AssetDatabase, SceneSerializer, PrefabSystem, HotReload, AssetManager,
    /// AssetImporter, AssetBundle, FileSystem, RenderDevice, RenderSystem,
    /// CameraSystem, MeshRenderer, Renderer3D, CollisionSystem, PhysicsSystem, and ConfigManager in that order (CommandLine first — it
    /// has no dependencies of its own; Logger second — every later step and
    /// every other system may need to log or UVE_ASSERT during its own
    /// setup; EntityManager right after EventSystem, since it needs
    /// MemoryManager for allocation and EventSystem for entity lifecycle
    /// events; SceneGraph immediately after, though it has no dependencies
    /// of its own; AssetDatabase right after SceneGraph, needing only
    /// Logger; SceneSerializer and PrefabSystem grouped immediately after,
    /// both stateless; HotReload right after, needing only EventSystem —
    /// constructed before AssetManager (rather than after, as its own Part
    /// 7.4 doc-comment ordering might suggest) because AssetManager takes a
    /// HotReload* constructor argument and construction must stay strictly
    /// forward-dependency (immediately after, RegisterBuiltInAssetLoadersUVE()
    /// registers the built-in MeshAssetUVE/TextureAssetUVE/ShaderAssetUVE/
    /// MaterialAssetUVE loaders with it); AssetImporter and AssetBundle grouped immediately
    /// after, both stateless; FileSystem right after, needing AssetBundle
    /// (its bundle-backed mounts read entries through it); RenderDevice
    /// right after, with no dependencies of its own (a NullRenderDeviceUVE —
    /// no real GPU backend is buildable in this sandbox); RenderSystem right
    /// after, needing RenderDevice (it records and submits command buffers
    /// through it); CameraSystem right after, stateless with no
    /// dependencies of its own (grouped with the rest of engine/render);
    /// MeshRenderer right after, likewise stateless (grouped with the rest
    /// of engine/render); Renderer3D right after, needing RenderDevice,
    /// RenderSystem, MeshRenderer, CameraSystem, AssetManager, AssetDatabase,
    /// and EventSystem — every one of which already exists by this point;
    /// CollisionSystem right after, stateless with no dependencies of its
    /// own; PhysicsSystem right after, needing only CollisionSystem (and
    /// EngineConfigUVE::gravity, already available); ConfigManager last, so
    /// its LoadUVE() call can log through the already-initialized Logger),
    /// then builds EngineServicesUVE from all twenty-four. Transitions
    /// Uninitialized -> Initializing -> Running.
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
    /// ConfigManager, then PhysicsSystem, then CollisionSystem, then Renderer3D, then MeshRenderer, then CameraSystem, then RenderSystem, then
    /// RenderDevice, then FileSystem, then AssetBundle, then AssetImporter,
    /// then AssetManager (its destructor blocks until every in-flight load
    /// job finishes), then HotReload, then PrefabSystem, then
    /// SceneSerializer, then AssetDatabase, then SceneGraph, then
    /// EntityManager (its destructor frees every remaining live entity's
    /// component memory, which must happen before MemoryManager's leak
    /// check below), then EventSystem, then Timer, then ThreadPool (its
    /// destructor blocks until every worker drains and joins), then
    /// MemoryManager (logging its leak report — and, in debug builds,
    /// UVE_ASSERTing zero active allocations — before it is destroyed),
    /// then Logger, then CommandLine — the exact reverse of Init()'s
    /// construction order — logging the final message before the logger
    /// itself is torn down.
    void Shutdown();

    [[nodiscard]] EngineStateUVE GetStateUVE() const noexcept;
    [[nodiscard]] const FrameStatsUVE& GetFrameStatsUVE() const noexcept;

    /// Sets the entity Render() passes to Renderer3DUVE::RenderFrameUVE() as the camera to render
    /// from, starting with the next frame. Passing Scene::kInvalidEntityUVE (the default) reverts
    /// Render() to its original no-op trace — this is the sole opt-in switch that keeps every
    /// frame-loop test/sample app predating this increment byte-identical unless it explicitly
    /// calls this.
    void SetActiveCameraUVE(Scene::EntityUVE cameraEntity) noexcept;

    /// The entity most recently passed to SetActiveCameraUVE(), or Scene::kInvalidEntityUVE if
    /// never called.
    [[nodiscard]] Scene::EntityUVE GetActiveCameraUVE() const noexcept;

    /// Returns the service container bundling Logger/Timer/EventSystem/
    /// MemoryManager/ThreadPool/CommandLine/ConfigManager/EntityManager/
    /// SceneGraph/AssetDatabase/SceneSerializer/PrefabSystem/HotReload/
    /// AssetManager/AssetImporter/AssetBundle/FileSystem/RenderDevice/
    /// RenderSystem/CameraSystem/MeshRenderer/Renderer3D/CollisionSystem/
    /// PhysicsSystem references. Valid only between Init() and Shutdown().
    [[nodiscard]] EngineServicesUVE& GetServicesUVE();

    /// Returns this build's engine version — the single source of truth
    /// future systems (assets, plugins, projects, crash reports, Hub
    /// integration) are expected to read.
    [[nodiscard]] static VersionUVE GetEngineVersionUVE() noexcept;

private:
    /// Registers the built-in MeshAssetUVE/TextureAssetUVE/ShaderAssetUVE/MaterialAssetUVE
    /// loaders with AssetManagerUVE (Part 7.2's rendering-facing asset types). Called once from
    /// Init(), immediately after AssetManagerUVE is constructed. A private orchestration step,
    /// not a new service — AssetManagerUVE itself stays generic and unaware of these concrete
    /// asset types; only EngineCoreUVE's composition root knows about both.
    void RegisterBuiltInAssetLoadersUVE();

    /// Ticks the timer, advances the frame counter, and records this
    /// frame's start instant (used by EndFrame() to compute frameTime).
    void BeginFrame();

    /// Advances the fixed-timestep accumulator, dispatches every event
    /// queued via IEventSystemUVE::QueueEvent() since the last dispatch,
    /// runs zero or more PhysicsSystemUVE::StepUVE() calls (one per whole
    /// fixed step FixedStepResultUVE::stepsToRun reports this frame — zero
    /// on a fast frame that hasn't accumulated a full step yet), then runs
    /// SceneGraphUVE::UpdateUVE() (transform-dirty-flag propagation) — after
    /// event dispatch and physics, so reparenting done by an event handler
    /// and positions moved by physics this frame are both picked up, and
    /// before LateUpdate()/Render(), so anything reading world transforms
    /// later in the frame sees up-to-date values. Each PhysicsSystemUVE::StepUVE()
    /// call already propagates its own intermediate world-transform updates
    /// internally, so this final UpdateUVE() call only needs to catch
    /// anything non-physics that moved this frame.
    void Update();

    /// Recomputes FrameStatsUVE::fps (an exponential moving average of
    /// 1/deltaTime). The documented hook point for future post-Update,
    /// pre-Render systems (camera follow, animation retargeting).
    void LateUpdate();

    /// Calls Renderer3DUVE::RenderFrameUVE(*m_entityManager, m_activeCamera) when
    /// m_activeCamera is valid; otherwise logs the original no-op trace line, preserving every
    /// pre-Increment-14 frame-loop test's behavior unless SetActiveCameraUVE() was called.
    void Render();

    /// Computes this frame's wall-clock frameTimeSeconds and records it
    /// into FrameStatsUVE.
    void EndFrame();

    /// Asserts IsValidTransitionUVE(m_state, newState), then applies it.
    void TransitionStateUVE(EngineStateUVE newState);

    EngineConfigUVE m_config;
    EngineStateUVE m_state = EngineStateUVE::Uninitialized;

    std::unique_ptr<CommandLine::ICommandLineUVE> m_commandLine;
    std::unique_ptr<Debug::ILoggerUVE> m_logger;
    std::unique_ptr<Memory::IMemoryManagerUVE> m_memoryManager;
    std::unique_ptr<Threading::IThreadPoolUVE> m_threadPool;
    std::unique_ptr<Utilities::ITimerUVE> m_timer;
    std::unique_ptr<Events::IEventSystemUVE> m_eventSystem;
    std::unique_ptr<Scene::IEntityManagerUVE> m_entityManager;
    std::unique_ptr<Scene::ISceneGraphUVE> m_sceneGraph;
    std::unique_ptr<Asset::IAssetDatabaseUVE> m_assetDatabase;
    std::unique_ptr<Scene::ISceneSerializerUVE> m_sceneSerializer;
    std::unique_ptr<Scene::IPrefabSystemUVE> m_prefabSystem;
    std::unique_ptr<Asset::IHotReloadUVE> m_hotReload;
    std::unique_ptr<Asset::IAssetManagerUVE> m_assetManager;
    std::unique_ptr<Asset::IAssetImporterUVE> m_assetImporter;
    std::unique_ptr<Asset::IAssetBundleUVE> m_assetBundle;
    std::unique_ptr<Asset::IFileSystemUVE> m_fileSystem;
    std::unique_ptr<Render::IRenderDeviceUVE> m_renderDevice;
    std::unique_ptr<Render::IRenderSystemUVE> m_renderSystem;
    std::unique_ptr<Render::ICameraSystemUVE> m_cameraSystem;
    std::unique_ptr<Render::IMeshRendererUVE> m_meshRenderer;
    std::unique_ptr<Render::IRenderer3DUVE> m_renderer3D;
    std::unique_ptr<Physics::ICollisionSystemUVE> m_collisionSystem;
    std::unique_ptr<Physics::IPhysicsSystemUVE> m_physicsSystem;
    std::unique_ptr<Config::IConfigManagerUVE> m_configManager;
    std::optional<EngineServicesUVE> m_services;

    FrameStatsUVE m_frameStats;
    std::chrono::steady_clock::time_point m_frameStartTime;
    bool m_quitRequested = false;
    Scene::EntityUVE m_activeCamera = Scene::kInvalidEntityUVE;
};

} // namespace UVE::Core
