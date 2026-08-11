//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#include "uve/core/engine_core_uve.h"

#include <cstddef>
#include <span>
#include <string>
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
#include "uve/audio/audio_source_system_uve.h"
#include "uve/audio/audio_system_uve.h"
#include "uve/audio/null_audio_device_uve.h"
#include "uve/commandline/command_line_uve.h"
#include "uve/config/config_manager_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/input/input_system_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/physics/physics_system_uve.h"
#include "uve/physics/raycast_system_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/render/camera_system_uve.h"
#include "uve/render/gl_render_device_uve.h"
#include "uve/render/light_system_uve.h"
#include "uve/render/mesh_renderer_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/render_system_uve.h"
#include "uve/render/renderer_3d_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_manager_uve.h"
#include "uve/save/checkpoint_manager_uve.h"
#include "uve/save/save_game_system_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/prefab_system_uve.h"
#include "uve/scene/scene_graph_uve.h"
#include "uve/scene/scene_serializer_uve.h"
#include "uve/threading/thread_pool_uve.h"
#include "uve/utilities/timer_uve.h"
#include "uve/window/null_window_manager_uve.h"
#include "uve/window/window_manager_uve.h"

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

void EngineCoreUVE::SetupDemoTriangleUVE() {
    // clang-format off
    constexpr float kVertices[] = {
        0.0F, 0.5F, 0.0F,
        -0.5F, -0.5F, 0.0F,
        0.5F, -0.5F, 0.0F,
    };
    // clang-format on
    const std::span<const std::byte> vertexBytes = std::as_bytes(std::span<const float>(kVertices));
    m_demoTriangleVertexBuffer = m_renderDevice->CreateBufferUVE(
        Render::BufferDescUVE{vertexBytes.size(), Render::BufferUsageUVE::Vertex}, vertexBytes);

    // basic_3d.glsl (see Render::Shader::BuiltIn) declares uModel/uViewProjection/uColor, set each
    // frame in RenderDemoTriangleUVE() — #0D0D0D background / #00D4FF triangle are the approved
    // design decision's exact colors.
    Render::Shader::ShaderProgramDescUVE programDesc;
    programDesc.virtualFilePath = std::string(Render::Shader::BuiltIn::kBasic3DVirtualPath);
    programDesc.embeddedFallbackSourceCode = std::string(Render::Shader::BuiltIn::kBasic3DSource);
    programDesc.vertexLayout = {
        Render::VertexAttributeUVE{"POSITION", Render::VertexAttributeFormatUVE::Float3, 0}};
    programDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    programDesc.depthTestEnabled = false;
    programDesc.depthWriteEnabled = false;
    programDesc.debugNameUVE = "DemoTriangle";
    m_demoTriangleProgram = m_shaderManager->CreateProgramUVE(programDesc);
}

void EngineCoreUVE::RenderDemoTriangleUVE() {
    m_renderSystem->BeginFrameUVE();
    Render::ICommandBufferUVE& commandBuffer = m_renderSystem->GetFrameCommandBufferUVE();

    Render::RenderPassDescUVE passDesc;
    passDesc.colorAttachment = Render::kInvalidTextureHandleUVE; // the window's default framebuffer
    passDesc.depthAttachment = Render::kInvalidTextureHandleUVE;
    passDesc.colorLoadOp = Render::LoadOpUVE::Clear;
    passDesc.clearColor = {0x0D / 255.0F, 0x0D / 255.0F, 0x0D / 255.0F, 1.0F};
    passDesc.depthLoadOp = Render::LoadOpUVE::DontCare;

    commandBuffer.BeginRenderPassUVE(passDesc);
    // The program may still be asynchronously compiling (see ShaderManagerUVE's threading model)
    // — the framebuffer clear above still runs every frame regardless, so the window never shows
    // undefined content; only the draw call itself is gated on readiness.
    if (m_demoTriangleProgram->IsValidUVE()) {
        m_demoTriangleProgram->SetMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
        m_demoTriangleProgram->SetMatrix4x4UVE("uViewProjection", Math::Matrix4x4UVE::IdentityUVE());
        m_demoTriangleProgram->SetVector3UVE("uColor", Math::Vector3UVE{0.0F, 0.83137255F, 1.0F});
        m_demoTriangleProgram->ApplyToUVE(commandBuffer);
        commandBuffer.BindVertexBufferUVE(m_demoTriangleVertexBuffer);
        commandBuffer.DrawUVE(3);
    }
    commandBuffer.EndRenderPassUVE();

    m_renderSystem->EndFrameUVE();
    m_renderDevice->PresentUVE();
}

void EngineCoreUVE::Init() {
    TransitionStateUVE(EngineStateUVE::Initializing);

    // CommandLine first: it has zero dependencies (pure parsing of the
    // args already captured in EngineConfigUVE), and its parsed flags are
    // the kind of thing a later step could plausibly want during its own
    // setup in a future increment.
    m_commandLine = std::make_unique<CommandLine::CommandLineUVE>(m_config.commandLineArgs);

    // --headless overrides EngineConfigUVE::headlessUVE the moment CommandLine exists, before
    // anything below reads it.
    if (m_commandLine->HasFlagUVE("headless")) {
        m_config.headlessUVE = true;
    }

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

    // WindowManager seventeenth: a real Window::WindowManagerUVE (owning the entire GLFW/GL
    // context lifecycle — init, create, activate, destroy, terminate) unless
    // EngineConfigUVE::headlessUVE, in which case Window::NullWindowManagerUVE. A real window
    // that fails to create logs UVE_FATAL and sets m_windowCreationFailedUVE rather than aborting
    // Init() mid-construction — EngineStateUVE's transition table forbids jumping straight from
    // Initializing to ShuttingDown, so every remaining service below still finishes constructing
    // normally; Load() checks the flag afterward (see Load()'s doc comment).
    if (m_config.headlessUVE) {
        m_windowManager = std::make_unique<Window::NullWindowManagerUVE>();
    } else {
        Window::WindowDescUVE windowDesc;
        windowDesc.title = m_config.windowTitle;
        windowDesc.width = m_config.windowWidth;
        windowDesc.height = m_config.windowHeight;
        windowDesc.resizable = m_config.windowResizableUVE;
        windowDesc.vsyncEnabled = m_config.vsyncEnabledUVE;
        windowDesc.glVersionMajor = m_config.windowGlVersionMajor;
        windowDesc.glVersionMinor = m_config.windowGlVersionMinor;
        m_windowManager = std::make_unique<Window::WindowManagerUVE>(*m_eventSystem, windowDesc);
        if (!m_windowManager->IsValidUVE()) {
            UVE_FATAL("EngineCoreUVE: window creation failed - this run will not proceed past Load()");
            m_windowCreationFailedUVE = true;
        }
    }
    m_windowedRenderingActiveUVE = !m_config.headlessUVE && m_windowManager->IsValidUVE();

    // RenderDevice eighteenth: Render::GlRenderDeviceUVE when m_windowedRenderingActiveUVE (a
    // real, valid window/GL context exists), otherwise Render::NullRenderDeviceUVE — headless
    // mode, or a real window that failed to create above. Never constructs GlRenderDeviceUVE
    // against an invalid window (there is no current GL context to build one against).
    if (m_windowedRenderingActiveUVE) {
        m_renderDevice = std::make_unique<Render::GlRenderDeviceUVE>(*m_windowManager);
    } else {
        m_renderDevice = std::make_unique<Render::NullRenderDeviceUVE>();
    }

    // ShaderManager nineteenth: needs ThreadPool, EventSystem, RenderDevice, and FileSystem — all
    // already constructed by this point. Mounts EngineConfigUVE::shaderSourceRealDirectoryUVE
    // under shaderSourceMountPrefixUVE first, so the built-in .glsl files (and any #include
    // closure among them) resolve through the VFS and participate in hot-reload; a
    // missing/unreachable directory is not an error here either — every built-in also carries an
    // embedded string fallback (see Render::Shader::BuiltIn::kBasic3DSource) ShaderManagerUVE uses
    // automatically when the mount doesn't resolve. Works identically in headless mode (against
    // NullRenderDeviceUVE) and windowed mode (against GlRenderDeviceUVE).
    m_fileSystem->MountDirectoryUVE(m_config.shaderSourceMountPrefixUVE, m_config.shaderSourceRealDirectoryUVE, 0);
    Render::Shader::ShaderManagerConfigUVE shaderManagerConfig;
    shaderManagerConfig.cachePath = m_config.shaderCachePath;
    shaderManagerConfig.hotReloadEnabledUVE = m_config.shaderHotReloadEnabledUVE;
    shaderManagerConfig.hotReloadPollIntervalSecondsUVE = m_config.shaderHotReloadPollIntervalSecondsUVE;
#if UVE_DEBUG
    shaderManagerConfig.injectDebugDefineUVE = true;
#else
    shaderManagerConfig.injectDebugDefineUVE = false;
#endif
    m_shaderManager = std::make_unique<Render::Shader::ShaderManagerUVE>(
        *m_threadPool, *m_eventSystem, *m_renderDevice, *m_fileSystem, shaderManagerConfig);

    // RenderSystem twentieth: needs RenderDevice, since it records and
    // submits command buffers through it.
    m_renderSystem = std::make_unique<Render::RenderSystemUVE>(*m_renderDevice);

    // CameraSystem twenty-first: stateless, no dependencies of its own —
    // grouped with the rest of engine/render.
    m_cameraSystem = std::make_unique<Render::CameraSystemUVE>();

    // MeshRenderer twenty-second: stateless, no dependencies of its own —
    // grouped with the rest of engine/render.
    m_meshRenderer = std::make_unique<Render::MeshRendererUVE>();

    // LightSystem twenty-third: stateless, no dependencies of its own — grouped with the rest
    // of engine/render, constructed right before Renderer3D since Renderer3D's constructor
    // needs it.
    m_lightSystem = std::make_unique<Render::LightSystemUVE>();

    // Renderer3D twenty-fourth: needs RenderDevice, RenderSystem, MeshRenderer, CameraSystem,
    // LightSystem, ShaderManager (Increment 26 — compiles the built-in shadow-depth program),
    // AssetManager, AssetDatabase, EventSystem, EngineConfigUVE::ambientColor, and
    // EngineConfigUVE::shadowMapResolution/shadowMapHalfExtent/shadowMapNearPlane/
    // shadowMapFarPlane — every one of which already exists by this point (ShaderManager is
    // constructed twentieth, above). Its offscreen render target is fixed at
    // EngineConfigUVE::renderTargetWidth/Height for this EngineCoreUVE's lifetime, entirely
    // independent of WindowManagerUVE/the real window size — Renderer3DUVE never renders into the
    // window itself (see docs/CODING_STANDARDS.md's rendering-evolution roadmap for how these two
    // eventually connect).
    m_renderer3D = std::make_unique<Render::Renderer3DUVE>(
        *m_renderDevice, *m_renderSystem, *m_meshRenderer, *m_cameraSystem, *m_lightSystem, *m_shaderManager,
        *m_assetManager, *m_assetDatabase, *m_eventSystem, m_config.renderTargetWidth, m_config.renderTargetHeight,
        m_config.ambientColor, m_config.shadowMapResolution, m_config.shadowMapHalfExtent,
        m_config.shadowMapNearPlane, m_config.shadowMapFarPlane, m_config.shadowFrustumPadding,
        m_config.shadowPcfKernelRadius);

    // The demo triangle is set up right after Renderer3D, only when windowed rendering is
    // active — see SetupDemoTriangleUVE()'s own doc comment for why this is deliberately
    // temporary scaffold, entirely independent of Renderer3D/the ECS.
    if (m_windowedRenderingActiveUVE) {
        SetupDemoTriangleUVE();
    }

    // CollisionSystem twenty-fifth: stateless, no dependencies of its own.
    m_collisionSystem = std::make_unique<Physics::CollisionSystemUVE>();

    // PhysicsSystem twenty-sixth: needs only CollisionSystem (composed by reference) and
    // EngineConfigUVE::gravity, both already available.
    m_physicsSystem = std::make_unique<Physics::PhysicsSystemUVE>(*m_collisionSystem, m_config.gravity);

    // RaycastSystem twenty-seventh: stateless, no dependencies of its own.
    m_raycastSystem = std::make_unique<Physics::RaycastSystemUVE>();

    // InputSystem twenty-eighth: needs only EventSystem (composed by reference, to queue
    // InputActionTriggeredEventUVE), already available.
    m_inputSystem = std::make_unique<Input::InputSystemUVE>(*m_eventSystem);

    // AudioDevice twenty-ninth: no dependencies of its own (a NullAudioDeviceUVE — no real audio
    // hardware/SDK is buildable in this sandbox).
    m_audioDevice = std::make_unique<Audio::NullAudioDeviceUVE>();

    // AudioSystem thirtieth: needs AudioDevice (it pushes computed gain/position through it).
    m_audioSystem = std::make_unique<Audio::AudioSystemUVE>(*m_audioDevice);

    // AudioSourceSystem thirty-first: stateful but takes no constructor dependencies —
    // EntityManager and AudioSystem are passed to SyncUVE() per call, like
    // MeshRendererUVE::ExtractRenderQueueUVE().
    m_audioSourceSystem = std::make_unique<Audio::AudioSourceSystemUVE>();

    // SaveGameSystem thirty-second: needs SceneSerializer (composed by reference) and
    // EngineConfigUVE::saveDirectoryPath.
    m_saveGameSystem = std::make_unique<Save::SaveGameSystemUVE>(*m_sceneSerializer, m_config.saveDirectoryPath);

    // CheckpointManager thirty-third: needs SaveGameSystem (composed by reference) and
    // EngineConfigUVE::autoSaveIntervalSecondsUVE.
    m_checkpointManager =
        std::make_unique<Save::CheckpointManagerUVE>(*m_saveGameSystem, m_config.autoSaveIntervalSecondsUVE);

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
                        *m_renderDevice, *m_shaderManager, *m_renderSystem, *m_cameraSystem,
                        *m_meshRenderer, *m_lightSystem, *m_renderer3D, *m_collisionSystem, *m_physicsSystem,
                        *m_raycastSystem, *m_inputSystem, *m_audioDevice, *m_audioSystem,
                        *m_audioSourceSystem, *m_saveGameSystem, *m_checkpointManager,
                        *m_windowManager);

    TransitionStateUVE(EngineStateUVE::Running);
    UVE_INFO("EngineCoreUVE: initialized");
}

bool EngineCoreUVE::Load() {
    if (m_windowCreationFailedUVE) {
        UVE_FATAL("EngineCoreUVE: Load() aborting - window/GL context creation failed during Init()");
        return false;
    }
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
    m_inputSystem->UpdateUVE();

    m_windowManager->PollEventsUVE();
    if (m_windowManager->IsCloseRequestedUVE()) {
        RequestQuitUVE();
    }

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

    m_shaderManager->UpdateUVE(m_timer->GetDeltaTimeUVE());

    m_checkpointManager->UpdateUVE(m_timer->GetDeltaTimeUVE(), *m_entityManager,
                                    m_sceneGraph->GetChildrenUVE(*m_entityManager, Scene::kInvalidEntityUVE));
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

    if (m_activeCamera != Scene::kInvalidEntityUVE) {
        const auto& worldTransform =
            m_entityManager->GetComponentUVE<Scene::WorldTransformComponentUVE>(m_activeCamera);
        m_audioSystem->SetListenerPositionUVE(worldTransform.worldPosition);
        m_audioSystem->SetListenerOrientationUVE(
            Math::RotateVectorUVE(worldTransform.worldRotation, Math::Vector3UVE{0.0F, 0.0F, -1.0F}),
            Math::RotateVectorUVE(worldTransform.worldRotation, Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    }
    m_audioSourceSystem->SyncUVE(*m_entityManager, *m_audioSystem);
    m_audioSystem->UpdateUVE();
}

void EngineCoreUVE::Render() {
    if (m_activeCamera != Scene::kInvalidEntityUVE) {
        m_renderer3D->RenderFrameUVE(*m_entityManager, m_activeCamera);
    } else {
        UVE_TRACE("Render (no-op)");
    }

    if (m_windowedRenderingActiveUVE) {
        RenderDemoTriangleUVE();
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
    // CheckpointManager, then SaveGameSystem, then AudioSourceSystem, then AudioSystem, then AudioDevice, then
    // InputSystem, then RaycastSystem, then PhysicsSystem, then CollisionSystem, then Renderer3D, then LightSystem, then MeshRenderer, then CameraSystem, then RenderSystem, then ShaderManager, then RenderDevice
    // (destroying the demo triangle's shader program and vertex buffer first, if windowed rendering
    // was active), then WindowManager,
    // then FileSystem, then AssetBundle, then AssetImporter, then
    // AssetManager (its destructor blocks until every in-flight load job
    // finishes), then HotReload, then PrefabSystem, then SceneSerializer,
    // then AssetDatabase, then SceneGraph, then EntityManager, then
    // EventSystem, then Timer, then ThreadPool, then MemoryManager, then
    // Logger, then CommandLine. The final log message is emitted before the
    // logger itself is torn down, so it is guaranteed to be recorded.
    m_services.reset();
    m_configManager.reset();
    m_checkpointManager.reset();
    m_saveGameSystem.reset();
    m_audioSourceSystem.reset();
    m_audioSystem.reset();
    m_audioDevice.reset();
    m_inputSystem.reset();
    m_raycastSystem.reset();
    m_physicsSystem.reset();
    m_collisionSystem.reset();
    m_renderer3D.reset();
    m_lightSystem.reset();
    m_meshRenderer.reset();
    m_cameraSystem.reset();
    m_renderSystem.reset();

    // The demo triangle's shader program (owned via a ShaderManagerUVE-supplied shared_ptr
    // deleter) and vertex buffer must be destroyed while m_renderDevice (and the GL context
    // m_windowManager owns) is still valid — before either is reset below.
    if (m_windowedRenderingActiveUVE) {
        m_demoTriangleProgram.reset();
        m_renderDevice->DestroyBufferUVE(m_demoTriangleVertexBuffer);
    }
    m_shaderManager.reset();
    m_renderDevice.reset();

    // WindowManager right after RenderDevice — every GL object RenderDevice owned is already
    // destroyed above, so it's safe for WindowManager's destructor to tear down the GL context
    // (and, for the real backend, terminate GLFW) now.
    m_windowManager.reset();

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
