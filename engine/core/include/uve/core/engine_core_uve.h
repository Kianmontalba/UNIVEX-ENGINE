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
#include "uve/audio/i_audio_device_uve.h"
#include "uve/audio/i_audio_source_system_uve.h"
#include "uve/audio/i_audio_system_uve.h"
#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/core/engine_config_uve.h"
#include "uve/core/engine_services_uve.h"
#include "uve/core/engine_state_uve.h"
#include "uve/core/frame_stats_uve.h"
#include "uve/core/version_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/input/i_input_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/physics/i_raycast_system_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_mesh_renderer_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/render/i_renderer_3d_uve.h"
#include "uve/render/shader/i_shader_manager_uve.h"
#include "uve/save/i_checkpoint_manager_uve.h"
#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"
#include "uve/window/i_window_manager_uve.h"

namespace UVE::Core {

/// EngineCoreUVE owns the foundational engine services (CommandLine, Logger,
/// MemoryManager, ThreadPool, Timer, EventSystem, EntityManager, SceneGraph,
/// AssetDatabase, SceneSerializer, PrefabSystem, HotReload, AssetManager,
/// AssetImporter, AssetBundle, FileSystem, WindowManager, RenderDevice, ShaderManager,
/// RenderSystem, CameraSystem, MeshRenderer, Renderer3D, CollisionSystem, PhysicsSystem,
/// RaycastSystem, InputSystem, AudioDevice, AudioSystem, AudioSourceSystem,
/// SaveGameSystem, CheckpointManager, ConfigManager) and drives the canonical
/// engine lifecycle: Init -> Load -> N x (BeginFrame -> Update -> LateUpdate
/// -> Render -> EndFrame) -> Shutdown. Render() calls Renderer3DUVE::RenderFrameUVE()
/// (extract -> cull -> sort -> record -> submit) whenever
/// SetActiveCameraUVE() has set a valid camera entity; with no active camera
/// set (the default, and every test/sample app predating Increment 14), it
/// still logs the original no-op trace line, so existing frame-loop
/// behavior is byte-identical unless a caller opts in. Update() runs zero or
/// more fixed PhysicsSystemUVE steps (via Utilities::FixedStepResultUVE)
/// before SceneGraphUVE::UpdateUVE() each frame — entirely data-driven off
/// which entities have a RigidBodyComponentUVE/ColliderComponentUVE, so a
/// scene with none behaves exactly as it did before Increment 15, no opt-in
/// needed. RaycastSystemUVE (Increment 16) is a stateless, on-demand query
/// service — like CameraSystem/MeshRenderer, it has no Update()-loop hook
/// of its own; callers reach it via GetServicesUVE().GetRaycastSystemUVE().
/// InputSystemUVE (Increment 17) is stateful and IS driven every frame —
/// Update()'s very first statement is InputSystemUVE::UpdateUVE(), so this
/// frame's key/mouse edge state and action-triggered events are settled
/// before the fixed-timestep accumulator, event dispatch, or physics steps
/// that follow it in the same call. AudioSourceSystemUVE/AudioSystemUVE
/// (Increment 18) are driven from LateUpdate(): if SetActiveCameraUVE() has
/// set a valid camera, the audio listener is synced to that camera entity's
/// WorldTransformComponentUVE first (the spec's "AudioListenerUVE — Attached
/// to Camera3D by default"); then AudioSourceSystemUVE::SyncUVE() walks every
/// WorldTransformComponentUVE + AudioSourceComponentUVE entity (entirely
/// data-driven, like PhysicsSystemUVE — a scene with none is a cheap no-op);
/// then AudioSystemUVE::UpdateUVE() recomputes attenuated gain for every live
/// source. WindowManagerUVE/GlRenderDeviceUVE (Increment 20): unless
/// EngineConfigUVE::headlessUVE is true (also settable via the `--headless` CLI flag), Init()
/// creates a real GLFW3 window and OpenGL 4.6 Core render device; Update()'s first statement
/// (after InputSystemUVE::UpdateUVE()) pumps window events and checks
/// IWindowManagerUVE::IsCloseRequestedUVE(), calling RequestQuitUVE() if the user closed the
/// window; Render() additionally records and presents a small, explicitly temporary demo
/// triangle proving the window/GL pipeline end-to-end — deliberately outside Renderer3DUVE, which
/// still only ever renders into its own offscreen target regardless of windowed mode (see
/// docs/CODING_STANDARDS.md for the full rendering-evolution roadmap this triangle is the first
/// milestone of). If real window/context creation fails, Init() logs UVE_FATAL and falls back to
/// NullWindowManagerUVE/NullRenderDeviceUVE for the rest of this run, and Load() reports failure
/// so RunUVE() shuts down cleanly instead of proceeding into a broken windowed session.
/// CheckpointManagerUVE (Increment 19) is driven from Update()'s final statement:
/// UpdateUVE(deltaTime, entityManager, every SceneGraphUVE::GetChildrenUVE(kInvalidEntityUVE)
/// root) accumulates elapsed time and, once the configured
/// EngineConfigUVE::autoSaveIntervalSecondsUVE elapses, saves the whole scene to
/// Save::kAutoSaveSlotIndexUVE via the composed SaveGameSystemUVE — entirely data-driven, like
/// PhysicsSystemUVE/AudioSourceSystemUVE, so an empty scene with the default 300-second interval
/// never actually writes to disk during a short test run. ShaderManagerUVE (Increment 21) is
/// constructed right after RenderDevice, in both headless and windowed mode (it works identically
/// against NullRenderDeviceUVE/GlRenderDeviceUVE); Init() mounts
/// EngineConfigUVE::shaderSourceRealDirectoryUVE under shaderSourceMountPrefixUVE first, so the
/// built-in `.glsl` files resolve through the VFS. Update() calls ShaderManagerUVE::UpdateUVE()
/// every frame (draining background preprocessing, compiling/linking on the main thread, and
/// polling hot-reload) alongside the existing HotReloadUVE/AssetManagerUVE maintenance calls. The
/// demo triangle above now loads its program from the `basic_3d.glsl` built-in via
/// ShaderManagerUVE::CreateProgramUVE() instead of an inline GLSL string; since compilation is
/// asynchronous, the triangle may not actually draw until a frame or two after Init() — Render()
/// guards the draw call on ShaderProgramUVE::IsValidUVE(), so the window's clear color still
/// appears immediately regardless.
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
    /// AssetImporter, AssetBundle, FileSystem, WindowManager, RenderDevice, ShaderManager, RenderSystem,
    /// CameraSystem, MeshRenderer, Renderer3D, CollisionSystem, PhysicsSystem, RaycastSystem, InputSystem, AudioDevice, AudioSystem, AudioSourceSystem, SaveGameSystem, CheckpointManager, and ConfigManager in that order (CommandLine first — it
    /// has no dependencies of its own; immediately after, reads the `--headless` CLI flag via
    /// CommandLineUVE::HasFlagUVE("headless"), OR'd into EngineConfigUVE::headlessUVE; Logger
    /// second — every later step and
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
    /// (its bundle-backed mounts read entries through it); WindowManager right after — a real
    /// Window::WindowManagerUVE (owning the entire GLFW/GL context lifecycle) unless
    /// EngineConfigUVE::headlessUVE, in which case Window::NullWindowManagerUVE; a real window
    /// that fails to create sets a private failure flag Load() checks (see Load()'s own doc
    /// comment) rather than aborting Init() mid-construction (EngineStateUVE's transition table
    /// forbids jumping straight from Initializing to ShuttingDown); RenderDevice
    /// right after — Render::GlRenderDeviceUVE when a real, valid window exists, otherwise
    /// Render::NullRenderDeviceUVE (headless mode, or a real window that failed to create — never
    /// constructs GlRenderDeviceUVE against an invalid window); ShaderManager right after — needs
    /// ThreadPool, EventSystem, RenderDevice, and FileSystem (all already constructed by this
    /// point); mounts EngineConfigUVE::shaderSourceRealDirectoryUVE under
    /// shaderSourceMountPrefixUVE on IFileSystemUVE first, then constructs
    /// Render::Shader::ShaderManagerUVE from an EngineConfigUVE-derived
    /// Render::Shader::ShaderManagerConfigUVE; works identically in headless mode (against
    /// NullRenderDeviceUVE) and windowed mode (against GlRenderDeviceUVE); RenderSystem right
    /// after, needing RenderDevice (it records and submits command buffers
    /// through it); CameraSystem right after, stateless with no
    /// dependencies of its own (grouped with the rest of engine/render);
    /// MeshRenderer right after, likewise stateless (grouped with the rest
    /// of engine/render); Renderer3D right after, needing RenderDevice,
    /// RenderSystem, MeshRenderer, CameraSystem, AssetManager, AssetDatabase,
    /// and EventSystem — every one of which already exists by this point;
    /// CollisionSystem right after, stateless with no dependencies of its
    /// own; PhysicsSystem right after, needing only CollisionSystem (and
    /// EngineConfigUVE::gravity, already available); RaycastSystem right
    /// after, stateless with no dependencies of its own (grouped with the
    /// rest of engine/physics); InputSystem right after, needing only
    /// EventSystem (composed by reference, to queue InputActionTriggeredEventUVE);
    /// AudioDevice right after, with no dependencies of its own (a
    /// NullAudioDeviceUVE — no real audio hardware/SDK is buildable in this
    /// sandbox); AudioSystem right after, needing AudioDevice (it pushes
    /// computed gain/position through it); AudioSourceSystem right after,
    /// stateful but taking no constructor dependencies (EntityManager and
    /// AudioSystem are passed to SyncUVE() per call, like
    /// MeshRendererUVE::ExtractRenderQueueUVE()); SaveGameSystem right after, needing
    /// SceneSerializer (composed by reference) and EngineConfigUVE::saveDirectoryPath;
    /// CheckpointManager right after, needing SaveGameSystem (composed by reference) and
    /// EngineConfigUVE::autoSaveIntervalSecondsUVE; ConfigManager last, so
    /// its LoadUVE() call can log through the already-initialized Logger),
    /// then builds EngineServicesUVE from all thirty-three. Transitions
    /// Uninitialized -> Initializing -> Running.
    void Init();

    /// The engine's asset/subsystem loading hook. Also the point where a real
    /// window/GL-context creation failure detected during Init() (see Init()'s own doc comment)
    /// is surfaced: if so, logs UVE_FATAL and returns false — nothing else needed loading this
    /// increment, so this is otherwise the complete, correct behavior for the one thing this
    /// stage owns today (a fatal-startup check plus logging), not a placeholder for a future one.
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
    /// ConfigManager, then CheckpointManager, then SaveGameSystem, then AudioSourceSystem, then AudioSystem, then AudioDevice, then InputSystem, then RaycastSystem, then PhysicsSystem, then CollisionSystem, then Renderer3D, then MeshRenderer, then CameraSystem, then RenderSystem, then ShaderManager, then
    /// RenderDevice, then WindowManager (in that order — every GL object RenderDevice owns must
    /// be destroyed while WindowManager's context is still valid, before WindowManager's own
    /// destructor tears the context itself down), then FileSystem, then AssetBundle, then AssetImporter,
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
    /// AssetManager/AssetImporter/AssetBundle/FileSystem/RenderDevice/ShaderManager/
    /// RenderSystem/CameraSystem/MeshRenderer/Renderer3D/CollisionSystem/
    /// PhysicsSystem/RaycastSystem/InputSystem/AudioDevice/AudioSystem/
    /// AudioSourceSystem/SaveGameSystem/CheckpointManager/WindowManager references. Valid only
    /// between Init() and Shutdown().
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

    /// First calls InputSystemUVE::UpdateUVE() — settling this frame's key/mouse edge state and
    /// queueing any newly-triggered action's InputActionTriggeredEventUVE — before anything else,
    /// so the event dispatch that follows in this same call delivers it same-frame. Immediately
    /// after, calls IWindowManagerUVE::PollEventsUVE() (a no-op for NullWindowManagerUVE) and, if
    /// IsCloseRequestedUVE() is now true, calls RequestQuitUVE() — so a real window's OS close
    /// button drives the exact same graceful-shutdown path RunUVE() already uses for any other
    /// quit request. Then advances
    /// the fixed-timestep accumulator, dispatches every event
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
    /// anything non-physics that moved this frame. Finally drives
    /// CheckpointManagerUVE::UpdateUVE() with every current scene-graph root (see
    /// Save::ICheckpointManagerUVE), so any auto-save this frame captures the just-updated world
    /// state, not last frame's. Also calls ShaderManagerUVE::UpdateUVE() (Increment 21) alongside
    /// the existing HotReloadUVE::PollUVE()/AssetManagerUVE::CollectGarbageUVE() maintenance
    /// calls, draining any completed background shader preprocessing, compiling/linking on this
    /// (the main) thread, and polling hot-reload-tracked programs for on-disk changes.
    void Update();

    /// Recomputes FrameStatsUVE::fps (an exponential moving average of
    /// 1/deltaTime). Then, if SetActiveCameraUVE() has set a valid camera entity, syncs the audio
    /// listener to that entity's WorldTransformComponentUVE (the spec's "AudioListenerUVE —
    /// Attached to Camera3D by default"); with no active camera set, the listener simply stays
    /// wherever it was last set manually. Then runs AudioSourceSystemUVE::SyncUVE() (entirely
    /// data-driven off which entities have a WorldTransformComponentUVE + AudioSourceComponentUVE,
    /// so a scene with none is a cheap no-op) followed by AudioSystemUVE::UpdateUVE() (recomputing
    /// attenuated gain for every live source). The documented hook point for future post-Update,
    /// pre-Render systems (camera follow, animation retargeting).
    void LateUpdate();

    /// Calls Renderer3DUVE::RenderFrameUVE(*m_entityManager, m_activeCamera) when
    /// m_activeCamera is valid; otherwise logs the original no-op trace line, preserving every
    /// pre-Increment-14 frame-loop test's behavior unless SetActiveCameraUVE() was called. When a
    /// real window/GL device is active (see m_windowedRenderingActiveUVE), additionally records
    /// and submits the temporary demo triangle (BeginFrameUVE/clear/bind-if-ready/draw-if-ready/
    /// EndFrameUVE against the default framebuffer) and calls IRenderDeviceUVE::PresentUVE() —
    /// entirely independent of m_activeCamera/Renderer3DUVE, proving the window/GL pipeline
    /// end-to-end without touching the ECS. See SetupDemoTriangleUVE()'s doc comment for why this
    /// is deliberately temporary.
    void Render();

    /// Computes this frame's wall-clock frameTimeSeconds and records it
    /// into FrameStatsUVE.
    void EndFrame();

    /// Asserts IsValidTransitionUVE(m_state, newState), then applies it.
    void TransitionStateUVE(EngineStateUVE newState);

    /// Builds the triangle's vertex buffer via m_renderDevice and begins asynchronously compiling
    /// its shader program (the `basic_3d.glsl` built-in) via m_shaderManager->CreateProgramUVE() —
    /// called once from Init(), only when m_windowedRenderingActiveUVE. This is explicitly
    /// temporary proof-of-life scaffold for Increment 20 ("the bridge from headless foundation to
    /// visual engine"), now sourcing its shader from ShaderManagerUVE (Increment 21) instead of an
    /// inline GLSL string, but still deliberately bypassing Renderer3DUVE/MeshRendererUVE/the
    /// asset pipeline/the ECS entirely — it must never grow into a real content path. The
    /// intended long-term rendering flow is Scene -> Renderer3DUVE -> RenderGraph -> RenderDevice
    /// -> Present, not EngineCore -> Triangle (see docs/CODING_STANDARDS.md's rendering-evolution
    /// roadmap); this scaffold is deleted, not extended, once that bridge is built.
    void SetupDemoTriangleUVE();

    /// Records and submits the demo triangle built by SetupDemoTriangleUVE(), then calls
    /// IRenderDeviceUVE::PresentUVE(). Called from Render() only when
    /// m_windowedRenderingActiveUVE. Always clears the framebuffer to the approved background
    /// color; the actual triangle draw is additionally guarded on
    /// m_demoTriangleProgram->IsValidUVE(), since CreateProgramUVE() compiles asynchronously and
    /// may not have finished by the first frame or two after Init().
    void RenderDemoTriangleUVE();

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
    std::unique_ptr<Window::IWindowManagerUVE> m_windowManager;
    std::unique_ptr<Render::IRenderDeviceUVE> m_renderDevice;
    std::unique_ptr<Render::Shader::IShaderManagerUVE> m_shaderManager;
    std::unique_ptr<Render::IRenderSystemUVE> m_renderSystem;
    std::unique_ptr<Render::ICameraSystemUVE> m_cameraSystem;
    std::unique_ptr<Render::IMeshRendererUVE> m_meshRenderer;
    std::unique_ptr<Render::IRenderer3DUVE> m_renderer3D;
    std::unique_ptr<Physics::ICollisionSystemUVE> m_collisionSystem;
    std::unique_ptr<Physics::IPhysicsSystemUVE> m_physicsSystem;
    std::unique_ptr<Physics::IRaycastSystemUVE> m_raycastSystem;
    std::unique_ptr<Input::IInputSystemUVE> m_inputSystem;
    std::unique_ptr<Audio::IAudioDeviceUVE> m_audioDevice;
    std::unique_ptr<Audio::IAudioSystemUVE> m_audioSystem;
    std::unique_ptr<Audio::IAudioSourceSystemUVE> m_audioSourceSystem;
    std::unique_ptr<Save::ISaveGameSystemUVE> m_saveGameSystem;
    std::unique_ptr<Save::ICheckpointManagerUVE> m_checkpointManager;
    std::unique_ptr<Config::IConfigManagerUVE> m_configManager;
    std::optional<EngineServicesUVE> m_services;

    FrameStatsUVE m_frameStats;
    std::chrono::steady_clock::time_point m_frameStartTime;
    bool m_quitRequested = false;
    Scene::EntityUVE m_activeCamera = Scene::kInvalidEntityUVE;

    /// True iff Init() constructed a real, valid WindowManagerUVE + GlRenderDeviceUVE pair (i.e.
    /// !EngineConfigUVE::headlessUVE and window/context creation succeeded). Gates
    /// Update()'s window-event pump/close-check and Render()'s demo-triangle present — both stay
    /// exact no-ops when this is false, matching every prior increment's headless behavior.
    bool m_windowedRenderingActiveUVE = false;

    /// Set by Init() if a real (non-headless) window/GL-context creation attempt failed. Checked
    /// by Load(), which fails in that case so RunUVE() shuts the engine down cleanly rather than
    /// proceeding into a broken windowed session.
    bool m_windowCreationFailedUVE = false;

    Render::BufferHandleUVE m_demoTriangleVertexBuffer;

    /// The demo triangle's linked shader program (Increment 21), created via
    /// m_shaderManager->CreateProgramUVE() from the `basic_3d.glsl` built-in. Owns its underlying
    /// GL shader/pipeline handles via a ShaderManagerUVE-supplied shared_ptr deleter — destroyed
    /// by resetting this member (see Shutdown()), never via a raw Destroy*UVE() call.
    std::shared_ptr<Render::Shader::ShaderProgramUVE> m_demoTriangleProgram;
};

} // namespace UVE::Core
