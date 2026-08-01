//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_core_uve.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/blob_asset_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/math/aabb_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/physics/i_raycast_system_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Core::Tests {
namespace {

EngineConfigUVE MakeTestConfigUVE() {
    EngineConfigUVE config{};
    config.enableConsoleLogging = false;
    config.logFilePath = "uve_engine_core_tests.log";
    config.threadPoolWorkerCount = 2; // keep the whole suite's thread churn small and fast
    config.settingsFilePath = "uve_engine_core_tests.uvesettings"; // never touch a real settings file
    config.assetDatabaseFilePath = "uve_engine_core_tests.uveassetdb"; // never touch a real asset db
    return config;
}

TEST(EngineCoreUVETest, InitialState_IsUninitialized) {
    const EngineCoreUVE engine(MakeTestConfigUVE());
    EXPECT_EQ(engine.GetStateUVE(), EngineStateUVE::Uninitialized);
}

TEST(EngineCoreUVETest, RunUVE_BoundedFrames_ReachesShutdownWithCorrectFrameCount) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    const int exitCode = engine.RunUVE(10);

    EXPECT_EQ(exitCode, 0);
    EXPECT_EQ(engine.GetStateUVE(), EngineStateUVE::Shutdown);
    EXPECT_EQ(engine.GetFrameStatsUVE().frameNumber, 10U);
}

TEST(EngineCoreUVETest, RunUVE_ZeroFrames_StillInitsAndShutsDownCleanly) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    const int exitCode = engine.RunUVE(0);

    EXPECT_EQ(exitCode, 0);
    EXPECT_EQ(engine.GetStateUVE(), EngineStateUVE::Shutdown);
    EXPECT_EQ(engine.GetFrameStatsUVE().frameNumber, 0U);
}

TEST(EngineCoreUVETest, RequestQuitUVE_BeforeRun_PreventsAnyFramesFromRunning) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.RequestQuitUVE();

    const int exitCode = engine.RunUVE(50);

    EXPECT_EQ(exitCode, 0);
    EXPECT_EQ(engine.GetStateUVE(), EngineStateUVE::Shutdown);
    EXPECT_EQ(engine.GetFrameStatsUVE().frameNumber, 0U);
}

TEST(EngineCoreUVETest, Shutdown_ClearsLoggerActiveInstance) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.RunUVE(1);

    EXPECT_EQ(Debug::LoggerUVE::GetActiveInstanceUVE(), nullptr);
}

TEST(EngineCoreUVETest, Timer_TotalTimeStrictlyIncreasesAcrossFrames) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    engine.TickFrameUVE();
    const double firstTotal = engine.GetFrameStatsUVE().totalTimeSeconds;
    engine.TickFrameUVE();
    const double secondTotal = engine.GetFrameStatsUVE().totalTimeSeconds;

    EXPECT_GT(secondTotal, firstTotal);
    engine.Shutdown();
}

TEST(EngineCoreUVETest, QueuedEvent_DeliveredDuringTickFrame) {
    struct PingEventUVE {
        int value = 0;
    };

    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    int received = -1;
    engine.GetServicesUVE().GetEventSystemUVE().Subscribe<PingEventUVE>(
        [&received](const PingEventUVE& event) { received = event.value; });
    engine.GetServicesUVE().GetEventSystemUVE().QueueEvent(PingEventUVE{99});

    ASSERT_EQ(received, -1);
    engine.TickFrameUVE();
    EXPECT_EQ(received, 99);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, FrameStats_PopulatedAfterFrames) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    engine.TickFrameUVE();
    engine.TickFrameUVE();

    const FrameStatsUVE& stats = engine.GetFrameStatsUVE();
    EXPECT_EQ(stats.frameNumber, 2U);
    EXPECT_GE(stats.frameTimeSeconds, 0.0);
    EXPECT_GE(stats.deltaTimeSeconds, 0.0);
    EXPECT_GT(stats.fps, 0.0);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, LoopStages_ExecuteInDocumentedOrder) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    engine.GetServicesUVE().GetLoggerUVE().AddSink(std::move(memorySink));

    engine.TickFrameUVE();

    std::vector<std::string> stageOrder;
    for (const Debug::LogMessageUVE& message : memorySinkPtr->GetMessagesUVE()) {
        if (message.message.starts_with("BeginFrame")) {
            stageOrder.emplace_back("BeginFrame");
        } else if (message.message.starts_with("Update:")) {
            stageOrder.emplace_back("Update");
        } else if (message.message.starts_with("LateUpdate:")) {
            stageOrder.emplace_back("LateUpdate");
        } else if (message.message.starts_with("Render")) {
            stageOrder.emplace_back("Render");
        } else if (message.message.starts_with("EndFrame")) {
            stageOrder.emplace_back("EndFrame");
        }
    }

    const std::vector<std::string> expectedOrder{"BeginFrame", "Update", "LateUpdate", "Render", "EndFrame"};
    EXPECT_EQ(stageOrder, expectedOrder);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, MemoryManager_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Memory::IMemoryManagerUVE& memoryManager = engine.GetServicesUVE().GetMemoryManagerUVE();
    Memory::IAllocatorUVE& allocator = memoryManager.GetDefaultAllocatorUVE();

    void* const pointer = allocator.AllocateUVE(32, 8, __FILE__, __LINE__);
    ASSERT_NE(pointer, nullptr);
    EXPECT_TRUE(memoryManager.HasLeaksUVE());
    allocator.DeallocateUVE(pointer);
    EXPECT_FALSE(memoryManager.HasLeaksUVE());

    engine.Shutdown();
}

TEST(EngineCoreUVETest, NormalRun_ReportsNoLeaks) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    engine.TickFrameUVE();
    engine.TickFrameUVE();

    // Nothing allocates through MemoryManagerUVE on Increment 1's behalf yet, so a normal run
    // must report zero leaks — this also implicitly exercises the debug-only shutdown leak
    // assertion on the happy path (zero leaks, assertion never fires).
    EXPECT_FALSE(engine.GetServicesUVE().GetMemoryManagerUVE().HasLeaksUVE());

    engine.Shutdown();
}

TEST(EngineCoreUVETest, ThreadPool_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Threading::IThreadPoolUVE& threadPool = engine.GetServicesUVE().GetThreadPoolUVE();
    EXPECT_GE(threadPool.GetWorkerCountUVE(), 1U);

    Threading::JobCounterUVE counter;
    bool jobRan = false;
    threadPool.SubmitUVE([&jobRan] { jobRan = true; }, counter);
    counter.WaitUVE();
    EXPECT_TRUE(jobRan);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, CommandLineAndConfigManager_ReachableAndFunctionalAfterInit) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.commandLineArgs = {"--server"};
    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    EXPECT_TRUE(engine.GetServicesUVE().GetCommandLineUVE().HasFlagUVE("server"));

    Config::IConfigManagerUVE& configManager = engine.GetServicesUVE().GetConfigManagerUVE();
    configManager.SetStringUVE("editor.theme", "dark");
    EXPECT_EQ(configManager.GetStringUVE("editor.theme", ""), "dark");

    engine.Shutdown();
}

TEST(EngineCoreUVETest, EntityManagerAndSceneGraph_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();

    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);

    engine.TickFrameUVE();

    const Scene::WorldTransformComponentUVE& world =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    EXPECT_FALSE(world.dirty);
    EXPECT_EQ(world.worldPosition, local.localPosition);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, AssetDatabaseSceneSerializerPrefabSystem_ReachableAndRoundTripAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    Asset::IAssetDatabaseUVE& assetDatabase = engine.GetServicesUVE().GetAssetDatabaseUVE();
    Scene::IPrefabSystemUVE& prefabSystem = engine.GetServicesUVE().GetPrefabSystemUVE();

    const Scene::EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::MeshComponentUVE>(
        source, Scene::MeshComponentUVE{Asset::AssetGuidUVE{51}, Asset::AssetGuidUVE{52}});

    const std::filesystem::path prefabPath = "uve_engine_core_tests.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    const Scene::EntityUVE instance =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, Scene::kInvalidEntityUVE);
    ASSERT_NE(instance, Scene::kInvalidEntityUVE);
    EXPECT_EQ(entityManager.GetComponentUVE<Scene::MeshComponentUVE>(instance).meshGuid, Asset::AssetGuidUVE{51});

    std::filesystem::remove(prefabPath);
    std::filesystem::remove(MakeTestConfigUVE().assetDatabaseFilePath);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, AssetManagerImporterHotReloadBundle_ReachableAndRoundTripAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Asset::IAssetDatabaseUVE& assetDatabase = engine.GetServicesUVE().GetAssetDatabaseUVE();
    Asset::IAssetImporterUVE& importer = engine.GetServicesUVE().GetAssetImporterUVE();
    Asset::IAssetManagerUVE& assetManager = engine.GetServicesUVE().GetAssetManagerUVE();
    static_cast<void>(engine.GetServicesUVE().GetHotReloadUVE());
    static_cast<void>(engine.GetServicesUVE().GetAssetBundleUVE());

    const std::filesystem::path sourcePath = "uve_engine_core_tests_source.txt";
    const std::filesystem::path destinationPath = "uve_engine_core_tests_dest.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    {
        std::ofstream file(sourcePath);
        file << "engine core asset pipeline round trip";
    }

    const Asset::AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    assetManager.RegisterLoaderUVE<Asset::BlobAssetUVE>(
        [](const std::filesystem::path& path, Asset::BlobAssetUVE& outValue) {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            const std::vector<char> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            outValue.assign(reinterpret_cast<const std::byte*>(raw.data()),
                             reinterpret_cast<const std::byte*>(raw.data()) + raw.size());
            return true;
        });

    {
        // Scoped so the handle releases its reference (and is destroyed) before engine.Shutdown()
        // tears down the AssetManagerUVE it points into — a handle must never outlive the
        // manager that produced it, exactly like an IEntityManagerUVE& argument never outliving
        // its EntityManagerUVE.
        const Asset::AssetHandleUVE<Asset::BlobAssetUVE> handle =
            assetManager.LoadUVE<Asset::BlobAssetUVE>(guid, assetDatabase);

        // Ticking a couple of frames exercises HotReloadUVE::PollUVE()/AssetManagerUVE::
        // CollectGarbageUVE() running from within EngineCoreUVE::Update() while a load is in
        // flight, proving neither crashes nor prematurely collects the still-referenced asset.
        engine.TickFrameUVE();
        engine.TickFrameUVE();

        bool ready = false;
        for (int poll = 0; poll < 200000 && !ready; ++poll) {
            ready = handle.IsReadyUVE() || handle.HasFailedUVE();
            if (!ready) {
                std::this_thread::yield();
            }
        }
        ASSERT_TRUE(ready);
        ASSERT_TRUE(handle.IsReadyUVE());
        const Asset::BlobAssetUVE* const blob = handle.TryGetUVE();
        ASSERT_NE(blob, nullptr);
        const std::string content(reinterpret_cast<const char*>(blob->data()), blob->size());
        EXPECT_EQ(content, "engine core asset pipeline round trip");
    }

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    engine.Shutdown();
}

TEST(EngineCoreUVETest, FileSystem_ReachableAndReadWriteRoundTripAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Asset::IFileSystemUVE& fileSystem = engine.GetServicesUVE().GetFileSystemUVE();

    const std::filesystem::path mountDirectory = "uve_engine_core_tests_vfs_mount";
    std::filesystem::remove_all(mountDirectory);
    std::filesystem::create_directories(mountDirectory);
    fileSystem.MountDirectoryUVE("", mountDirectory, 0);

    const std::string text = "engine core vfs round trip";
    const auto* const textBytes = reinterpret_cast<const std::byte*>(text.data());
    const std::vector<std::byte> data(textBytes, textBytes + text.size());
    ASSERT_TRUE(fileSystem.WriteFileUVE("notes.txt", data));

    const std::optional<std::vector<std::byte>> readBack = fileSystem.ReadFileUVE("notes.txt");
    ASSERT_TRUE(readBack.has_value());
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(readBack->data()), readBack->size()), text);

    std::filesystem::remove_all(mountDirectory);
    engine.Shutdown();
}

TEST(EngineCoreUVETest, RenderSystem_ReachableAndFrameLifecycleWorksAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Render::IRenderSystemUVE& renderSystem = engine.GetServicesUVE().GetRenderSystemUVE();
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 0U);

    renderSystem.BeginFrameUVE();
    static_cast<void>(renderSystem.GetFrameCommandBufferUVE());
    renderSystem.EndFrameUVE();

    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 1U);

    Render::IRenderDeviceUVE& renderDevice = engine.GetServicesUVE().GetRenderDeviceUVE();
    EXPECT_EQ(renderDevice.GetBackendNameUVE(), "Null");

    engine.Shutdown();
}

TEST(EngineCoreUVETest, CameraSystem_ReachableAndComputesViewProjectionAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    const Scene::EntityUVE cameraEntity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{0.0F, 0.0F, 5.0F};
    sceneGraph.AttachTransformUVE(entityManager, cameraEntity, local);
    sceneGraph.UpdateUVE(entityManager);
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(cameraEntity);

    Render::ICameraSystemUVE& cameraSystem = engine.GetServicesUVE().GetCameraSystemUVE();
    const Math::Matrix4x4UVE viewProjection =
        cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, 16.0F / 9.0F);
    const Math::FrustumUVE frustum = cameraSystem.ExtractFrustumUVE(viewProjection);

    EXPECT_TRUE(
        frustum.IntersectsUVE(Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F})));

    engine.Shutdown();
}

TEST(EngineCoreUVETest, Renderer3D_ReachableAfterInit_NoActiveCameraStillNoOps) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    static_cast<void>(engine.GetServicesUVE().GetRenderer3DUVE());
    EXPECT_EQ(engine.GetActiveCameraUVE(), Scene::kInvalidEntityUVE);

    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    engine.GetServicesUVE().GetLoggerUVE().AddSink(std::move(memorySink));

    engine.TickFrameUVE();

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundNoOpTrace =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.message.starts_with("Render (no-op)");
        });
    EXPECT_TRUE(foundNoOpTrace);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, Renderer3D_ActiveCameraSet_RendersWithoutCrashing) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    const Scene::EntityUVE cameraEntity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{0.0F, 0.0F, 5.0F};
    sceneGraph.AttachTransformUVE(entityManager, cameraEntity, local);
    sceneGraph.UpdateUVE(entityManager);
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(cameraEntity);

    engine.SetActiveCameraUVE(cameraEntity);
    EXPECT_EQ(engine.GetActiveCameraUVE(), cameraEntity);

    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    engine.GetServicesUVE().GetLoggerUVE().AddSink(std::move(memorySink));

    engine.TickFrameUVE();

    // With an active camera set, Render() no longer takes the no-op trace path — proves
    // RenderFrameUVE() actually ran instead (an empty scene still begins+ends a render pass).
    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundNoOpTrace =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.message.starts_with("Render (no-op)");
        });
    EXPECT_FALSE(foundNoOpTrace);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, PhysicsSystemAndCollisionSystem_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Physics::ICollisionSystemUVE& collisionSystem = engine.GetServicesUVE().GetCollisionSystemUVE();
    Physics::IPhysicsSystemUVE& physicsSystem = engine.GetServicesUVE().GetPhysicsSystemUVE();

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());

    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{0.0F, 10.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity);
    sceneGraph.UpdateUVE(entityManager);

    physicsSystem.StepUVE(entityManager, sceneGraph, 1.0F / 60.0F);

    const Scene::WorldTransformComponentUVE& world =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    EXPECT_LT(world.worldPosition.y, 10.0F);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, RaycastSystem_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    Physics::IRaycastSystemUVE& raycastSystem = engine.GetServicesUVE().GetRaycastSystemUVE();

    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{5.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);
    sceneGraph.UpdateUVE(entityManager);
    entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, Scene::ColliderComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}});

    Physics::RaycastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.maxDistance = 100.0F;
    const std::optional<Physics::RaycastHitUVE> hit = raycastSystem.RaycastUVE(entityManager, query);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, FallingRigidBody_TickFrameUVEDrivenPhysicsStep_MovesEntityDownward) {
    // A 1kHz fixed-update rate (1ms fixed step) paired with a short real sleep before each
    // TickFrameUVE() call guarantees the ITimerUVE accumulator crosses at least one fixed step
    // almost every frame — an excessively high fixedUpdateFps would trigger steps just as
    // reliably but make each step's position delta too small to be representable in float at
    // this entity's starting magnitude (10.0), silently rounding to no visible movement. This
    // test only proves EngineCoreUVE::Update()'s physics-step wiring moves an entity end-to-end;
    // PhysicsSystemUVE's exact per-step math is already covered by
    // tests/physics/physics_system_uve_tests.cpp.
    EngineConfigUVE config = MakeTestConfigUVE();
    config.fixedUpdateFps = 1000.0;
    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();

    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{0.0F, 10.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity);

    for (int frame = 0; frame < 30; ++frame) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        engine.TickFrameUVE();
    }

    const Scene::WorldTransformComponentUVE& world =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    EXPECT_LT(world.worldPosition.y, 10.0F);

    engine.Shutdown();
}

#if UVE_DEBUG
TEST(EngineCoreUVEDeathTest, ShutdownBeforeInit_TriggersInvalidTransitionAssert) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    EXPECT_DEATH({ engine.Shutdown(); }, "");
}
#endif

} // namespace
} // namespace UVE::Core::Tests
