// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/core/engine_core_uve.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <GL/gl.h>
#include <gtest/gtest.h>

#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/blob_asset_uve.h"
#include "uve/asset/data_table_importer_uve.h"
#include "uve/asset/data_table_uve.h"
#include "uve/audio/i_audio_system_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/input/i_input_system_uve.h"
#include "uve/math/aabb_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/physics/area_overlap_events_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/physics/i_raycast_system_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_light_system_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/save/i_checkpoint_manager_uve.h"
#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/window/i_window_manager_uve.h"

namespace UVE::Core::Tests {
namespace {

EngineConfigUVE MakeTestConfigUVE() {
    EngineConfigUVE config{};
    config.enableConsoleLogging = false;
    config.logFilePath = "uve_engine_core_tests.log";
    config.threadPoolWorkerCount = 2; // keep the whole suite's thread churn small and fast
    config.settingsFilePath = "uve_engine_core_tests.uvesettings"; // never touch a real settings file
    config.assetDatabaseFilePath = "uve_engine_core_tests.uveassetdb"; // never touch a real asset db
    config.headlessUVE = true; // NullWindowManagerUVE/NullRenderDeviceUVE - no display required;
                                // every pre-Increment-20 test opts into this by default, matching
                                // its exact prior (headless-only) behavior. Tests that specifically
                                // exercise the real window/GL backend override this explicitly.
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

TEST(EngineCoreUVETest, AreaOverlapLifecycle_QueuesEnteredAndExitedEvents) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    auto& services = engine.GetServicesUVE();
    auto& entityManager = services.GetEntityManagerUVE();
    auto& sceneGraph = services.GetSceneGraphUVE();

    const Scene::EntityUVE area = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE areaTransform;
    areaTransform.localPosition = Math::Vector3UVE{0.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, area, areaTransform);
    entityManager.AddComponentUVE<Scene::AreaComponentUVE>(
        area, Scene::AreaComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 0xFFFFFFFFU});

    const Scene::EntityUVE collider = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE colliderTransform;
    colliderTransform.localPosition = Math::Vector3UVE{0.5F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, collider, colliderTransform);
    entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
        collider, Scene::ColliderComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 0xFFFFFFFFU});
    sceneGraph.UpdateUVE(entityManager);

    std::vector<Physics::AreaOverlapPairUVE> entered;
    std::vector<Physics::AreaOverlapPairUVE> exited;
    services.GetEventSystemUVE().Subscribe<Physics::AreaOverlapEnteredEventUVE>(
        [&entered](const Physics::AreaOverlapEnteredEventUVE& event) { entered.push_back(event.pair); });
    services.GetEventSystemUVE().Subscribe<Physics::AreaOverlapExitedEventUVE>(
        [&exited](const Physics::AreaOverlapExitedEventUVE& event) { exited.push_back(event.pair); });

    engine.TickFrameUVE();
    EXPECT_TRUE(entered.empty());
    EXPECT_TRUE(exited.empty());

    engine.TickFrameUVE();
    ASSERT_EQ(entered.size(), 1U);
    EXPECT_EQ(entered.front().area, area);
    EXPECT_EQ(entered.front().other, collider);
    EXPECT_TRUE(exited.empty());

    entityManager.DestroyEntityUVE(collider);
    engine.TickFrameUVE();
    EXPECT_TRUE(exited.empty());

    engine.TickFrameUVE();
    ASSERT_EQ(exited.size(), 1U);
    EXPECT_EQ(exited.front().area, area);
    EXPECT_EQ(exited.front().other, collider);

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
    const Asset::AssetImportSourceClassificationUVE rawModelClassification =
        importer.ClassifySourceUVE("engine_core_tests_character.fbx");
    EXPECT_EQ(rawModelClassification.kind, Asset::AssetImportSourceKindUVE::RawModel);
    EXPECT_EQ(rawModelClassification.normalizedExtension, "fbx");
    EXPECT_FALSE(rawModelClassification.importerRegistered);
    EXPECT_TRUE(rawModelClassification.requiresFormatSpecificParser);
    EXPECT_EQ(rawModelClassification.diagnostic, "format-specific parser is not registered");
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

TEST(EngineCoreUVETest, TypedUVEEnvelopeImporters_ComposedAndReachableAfterInit) {
    EngineConfigUVE config = MakeTestConfigUVE();
    const std::filesystem::path root = std::filesystem::temp_directory_path();
    config.assetDatabaseFilePath = root / "uve_engine_core_typed_envelope_tests.uveassetdb";

    constexpr std::array<std::string_view, 4> kTypedEnvelopeExtensions = {
        ".uvemodel", ".uvetex", ".uveshader", ".uvemat"};
    struct CleanupUVE final {
        std::filesystem::path database;
        std::array<std::filesystem::path, 4> sources;
        std::array<std::filesystem::path, 4> destinations;
        ~CleanupUVE() {
            std::filesystem::remove(database);
            for (const auto& path : sources) {
                std::filesystem::remove(path);
            }
            for (const auto& path : destinations) {
                std::filesystem::remove(path);
            }
        }
    } cleanup{config.assetDatabaseFilePath, {}, {}};

    for (std::size_t index = 0; index < kTypedEnvelopeExtensions.size(); ++index) {
        const std::string suffix(kTypedEnvelopeExtensions[index]);
        cleanup.sources[index] = root / ("uve_engine_core_typed_source_" + std::to_string(index) + suffix);
        cleanup.destinations[index] = root / ("uve_engine_core_typed_dest_" + std::to_string(index) + suffix);
        std::ofstream source(cleanup.sources[index], std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(source.is_open());
        source << "typed envelope composition proof";
    }

    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Asset::IAssetDatabaseUVE& assetDatabase = engine.GetServicesUVE().GetAssetDatabaseUVE();
    Asset::IAssetImporterUVE& importer = engine.GetServicesUVE().GetAssetImporterUVE();
    for (std::size_t index = 0; index < kTypedEnvelopeExtensions.size(); ++index) {
        const Asset::AssetGuidUVE guid =
            importer.ImportUVE(cleanup.sources[index], cleanup.destinations[index], assetDatabase);
        ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE) << kTypedEnvelopeExtensions[index];
        EXPECT_EQ(assetDatabase.ResolveUVE(guid), cleanup.destinations[index]);
    }

    engine.Shutdown();
}

TEST(EngineCoreUVETest, DataTablePipeline_RegisteredAndReachableThroughServicesAfterInit) {
    EngineConfigUVE config = MakeTestConfigUVE();
    const std::filesystem::path root = std::filesystem::temp_directory_path();
    config.assetDatabaseFilePath = root / "uve_engine_core_data_table_tests.uveassetdb";
    const std::filesystem::path sourcePath = root / "uve_engine_core_data_table_tests.csv";
    const std::filesystem::path destinationPath = root / "uve_engine_core_data_table_tests.uvetable";
    std::filesystem::remove(config.assetDatabaseFilePath);
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);

    struct CleanupUVE final {
        std::filesystem::path database;
        std::filesystem::path source;
        std::filesystem::path destination;
        ~CleanupUVE() {
            std::filesystem::remove(database);
            std::filesystem::remove(source);
            std::filesystem::remove(destination);
        }
    } cleanup{config.assetDatabaseFilePath, sourcePath, destinationPath};

    {
        std::ofstream output(sourcePath, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(output.is_open());
        output << "id,damage\npistol,25\n";
    }

    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Asset::IAssetDatabaseUVE& assetDatabase = engine.GetServicesUVE().GetAssetDatabaseUVE();
    Asset::IAssetImporterUVE& importer = engine.GetServicesUVE().GetAssetImporterUVE();
    Asset::IAssetManagerUVE& assetManager = engine.GetServicesUVE().GetAssetManagerUVE();

    Asset::DataTableImportSettingsUVE settings;
    settings.tableName = "weapons";
    settings.columns = {Asset::DataTableColumnUVE{"damage", Asset::DataTableColumnTypeUVE::Integer}};
    const Asset::AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase, settings);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    {
        const Asset::AssetHandleUVE<Asset::DataTableUVE> handle =
            assetManager.LoadUVE<Asset::DataTableUVE>(guid, assetDatabase);
        bool terminal = false;
        for (int iteration = 0; iteration < 200000 && !terminal; ++iteration) {
            terminal = handle.IsReadyUVE() || handle.HasFailedUVE();
            if (!terminal) {
                std::this_thread::yield();
            }
        }
        ASSERT_TRUE(terminal);
        ASSERT_TRUE(handle.IsReadyUVE());
        const Asset::DataTableUVE* const table = handle.TryGetUVE();
        ASSERT_NE(table, nullptr);
        const Asset::DataTableSnapshotUVE snapshot = table->GetSnapshotUVE();
        EXPECT_EQ(snapshot.name, "weapons");
        ASSERT_EQ(snapshot.rows.size(), 1U);
        ASSERT_EQ(snapshot.rows.front().values.size(), 1U);
        EXPECT_EQ(std::get<std::int64_t>(snapshot.rows.front().values.front()), 25);
    }

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

TEST(EngineCoreUVETest, LightSystem_ReachableAndReturnsSentinelWithNoLightEntityAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Render::ILightSystemUVE& lightSystem = engine.GetServicesUVE().GetLightSystemUVE();
    const Render::LightListUVE lights =
        lightSystem.ExtractActiveLightsUVE(engine.GetServicesUVE().GetEntityManagerUVE());
    for (const Render::LightDataUVE& slot : lights) {
        EXPECT_FLOAT_EQ(slot.intensity, 0.0F);
    }

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

TEST(EngineCoreUVETest, InputSystem_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Input::IInputSystemUVE& inputSystem = engine.GetServicesUVE().GetInputSystemUVE();

    inputSystem.SetKeyStateUVE(Input::KeyCodeUVE::Space, true);
    engine.TickFrameUVE();

    EXPECT_TRUE(inputSystem.IsKeyDownUVE(Input::KeyCodeUVE::Space));
    EXPECT_TRUE(inputSystem.WasKeyPressedThisFrameUVE(Input::KeyCodeUVE::Space));

    engine.TickFrameUVE();
    EXPECT_TRUE(inputSystem.IsKeyDownUVE(Input::KeyCodeUVE::Space));
    EXPECT_FALSE(inputSystem.WasKeyPressedThisFrameUVE(Input::KeyCodeUVE::Space));

    inputSystem.SetKeyStateUVE(Input::KeyCodeUVE::Space, false);
    engine.TickFrameUVE();
    EXPECT_FALSE(inputSystem.IsKeyDownUVE(Input::KeyCodeUVE::Space));
    EXPECT_TRUE(inputSystem.WasKeyReleasedThisFrameUVE(Input::KeyCodeUVE::Space));

    engine.Shutdown();
}

TEST(EngineCoreUVETest, AudioSystem_ReachableAndFunctionalAfterInit) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Audio::IAudioSystemUVE& audioSystem = engine.GetServicesUVE().GetAudioSystemUVE();

    Audio::AudioSourceDescUVE desc;
    desc.spatial = false;
    const Audio::VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);
    ASSERT_NE(source, Audio::kInvalidVoiceHandleUVE);
    ASSERT_TRUE(audioSystem.PlayUVE(source));

    engine.TickFrameUVE();

    EXPECT_EQ(audioSystem.GetSourceStateUVE(source), Audio::VoicePlaybackStateUVE::Playing);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, AudioListener_TracksActiveCameraWorldPosition) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = engine.GetServicesUVE().GetSceneGraphUVE();
    const Scene::EntityUVE cameraEntity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{3.0F, 4.0F, 5.0F};
    sceneGraph.AttachTransformUVE(entityManager, cameraEntity, local);
    sceneGraph.UpdateUVE(entityManager);
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(cameraEntity);

    engine.SetActiveCameraUVE(cameraEntity);
    engine.TickFrameUVE();

    Audio::IAudioSystemUVE& audioSystem = engine.GetServicesUVE().GetAudioSystemUVE();
    EXPECT_EQ(audioSystem.GetListenerPositionUVE(), (Math::Vector3UVE{3.0F, 4.0F, 5.0F}));

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

TEST(EngineCoreUVETest, SaveGameSystemAndCheckpointManager_ReachableAndFunctionalAfterInit) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.saveDirectoryPath = "uve_engine_core_tests_saves";
    std::filesystem::remove_all(config.saveDirectoryPath);

    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Save::ISaveGameSystemUVE& saveGameSystem = engine.GetServicesUVE().GetSaveGameSystemUVE();
    Save::ICheckpointManagerUVE& checkpointManager = engine.GetServicesUVE().GetCheckpointManagerUVE();

    Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(0, entityManager, {entity}, Save::GameStateMetadataUVE{}));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(0));

    EXPECT_TRUE(checkpointManager.CheckpointUVE(entityManager, {entity}));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(Save::kManualCheckpointSlotIndexUVE));
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(Save::kAutoSaveSlotIndexUVE));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(0));

    engine.Shutdown();
    std::filesystem::remove_all(config.saveDirectoryPath);
}

TEST(EngineCoreUVETest, CheckpointManager_AutoSavesAfterConfiguredInterval_TickFrameUVEDriven) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.saveDirectoryPath = "uve_engine_core_tests_autosave_saves";
    config.autoSaveIntervalSecondsUVE = std::numeric_limits<double>::min(); // smallest positive interval
    std::filesystem::remove_all(config.saveDirectoryPath);

    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Save::ISaveGameSystemUVE& saveGameSystem = engine.GetServicesUVE().GetSaveGameSystemUVE();

    for (int frame = 0; frame < 3; ++frame) {
        engine.TickFrameUVE();
    }

    EXPECT_TRUE(saveGameSystem.HasSaveUVE(Save::kAutoSaveSlotIndexUVE));
    EXPECT_TRUE(saveGameSystem.ListUsedSlotsUVE().empty());

    engine.Shutdown();
    std::filesystem::remove_all(config.saveDirectoryPath);
}

TEST(EngineCoreUVETest, SimulationControl_PausesStepsQueuesOneStepAndSuppressesTransientCheckpoints) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.saveDirectoryPath = "uve_engine_core_tests_transient_saves";
    config.autoSaveIntervalSecondsUVE = std::numeric_limits<double>::min(); // smallest positive interval
    std::filesystem::remove_all(config.saveDirectoryPath);

    EngineCoreUVE engine(config);
    EXPECT_FALSE(engine.SetSimulationExecutionModeUVE(SimulationExecutionModeUVE::Paused));
    engine.Init();
    ASSERT_TRUE(engine.Load());

    Save::ISaveGameSystemUVE& saveGameSystem = engine.GetServicesUVE().GetSaveGameSystemUVE();
    Save::ICheckpointManagerUVE& checkpointManager = engine.GetServicesUVE().GetCheckpointManagerUVE();
    ASSERT_TRUE(engine.SetTransientSimulationSessionActiveUVE(true));
    ASSERT_TRUE(engine.SetSimulationExecutionModeUVE(SimulationExecutionModeUVE::Paused));
    EXPECT_TRUE(engine.RequestSingleSimulationStepUVE());
    EXPECT_FALSE(engine.RequestSingleSimulationStepUVE());
    engine.TickFrameUVE();
    EXPECT_EQ(engine.GetSimulationExecutionModeUVE(), SimulationExecutionModeUVE::Paused);
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(Save::kAutoSaveSlotIndexUVE));
    EXPECT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.0);
    EXPECT_DOUBLE_EQ(checkpointManager.GetTotalPlaytimeSecondsUVE(), 0.0);

    ASSERT_TRUE(engine.SetSimulationExecutionModeUVE(SimulationExecutionModeUVE::Running));
    ASSERT_TRUE(engine.SetTransientSimulationSessionActiveUVE(false));
    // A positive interval must observe a nonzero timer delta; allow the same three-frame cadence
    // used by the neighboring autosave integration test after the paused frame.
    for (int frame = 0; frame < 3; ++frame) {
        engine.TickFrameUVE();
    }
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(Save::kAutoSaveSlotIndexUVE));

    engine.Shutdown();
    std::filesystem::remove_all(config.saveDirectoryPath);
}

TEST(EngineCoreUVETest, HeadlessCommandLineFlag_ForcesHeadlessAndUsesNullWindowManager) {
    // headlessUVE starts false here specifically to prove the --headless CLI flag itself forces
    // it (Init() reads CommandLineUVE before anything else consults the flag), not that the
    // config's own default already happened to be headless.
    EngineConfigUVE config = MakeTestConfigUVE();
    config.headlessUVE = false;
    config.commandLineArgs = {"--headless"};

    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    EXPECT_EQ(engine.GetServicesUVE().GetWindowManagerUVE().GetBackendNameUVE(), "Null");

    for (int frame = 0; frame < 3; ++frame) {
        engine.TickFrameUVE();
    }
    EXPECT_EQ(engine.GetFrameStatsUVE().frameNumber, 3U);

    engine.Shutdown();
}

// Needs a real (possibly virtual, e.g. Xvfb) X display and GL context. Skips cleanly with a clear
// message when unavailable, so the same uve_tests binary runs cleanly with or without a display -
// same GTEST_SKIP() pattern as WindowManagerUVETest/GlRenderDeviceUVETest.
TEST(EngineCoreUVETest, WindowedMode_ReachesRunningAndPresentsEmptyRendererScene) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.headlessUVE = false;
    config.windowWidth = 64;
    config.windowHeight = 64;
    config.vsyncEnabledUVE = false;
    // This sandbox's Mesa/llvmpipe GLX stack caps at OpenGL 4.5 Core (confirmed by direct
    // testing - 4.6 fails with GLXBadFBConfig); see the identical override + rationale in
    // tests/window/window_manager_uve_tests.cpp. The shipped production default (4, 6) is
    // untouched.
    config.windowGlVersionMajor = 4;
    config.windowGlVersionMinor = 5;

    EngineCoreUVE engine(config);
    engine.Init();
    if (!engine.GetServicesUVE().GetWindowManagerUVE().IsValidUVE()) {
        GTEST_SKIP() << "No display available for windowed EngineCoreUVE - skipping (run under "
                        "xvfb-run to exercise this test)";
    }
    ASSERT_TRUE(engine.Load());
    EXPECT_EQ(engine.GetServicesUVE().GetWindowManagerUVE().GetBackendNameUVE(), "GLFW3");

    // An active identity camera ensures this is an empty *renderer scene* test rather than a
    // no-camera EngineCore no-op test.
    EngineServicesUVE& services = engine.GetServicesUVE();
    Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = services.GetSceneGraphUVE();
    const Scene::EntityUVE camera = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, camera, Scene::TransformComponentUVE{});
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(camera);
    engine.SetActiveCameraUVE(camera);

    std::array<unsigned char, 3> scenePixel{};
    GLenum postRenderGlError = GL_NO_ERROR;
    engine.SetPostRenderCallbackUVE([&scenePixel, &postRenderGlError] {
        glFinish();
        glReadBuffer(GL_BACK);
        glReadPixels(32, 32, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, scenePixel.data());
        postRenderGlError = glGetError();
    });

    // A few frames let Renderer3DUVE complete scene, tone-mapping, and presentation work.
    for (int frame = 0; frame < 3; ++frame) {
        engine.TickFrameUVE();
    }
    engine.SetPostRenderCallbackUVE({});
    EXPECT_EQ(engine.GetStateUVE(), EngineStateUVE::Running);

    // With no document render items, Renderer3DUVE presents its deterministic blue-gray
    // tone-mapped environment. This is intentionally not a demo-geometry assertion: visible
    // content enters only through ECS extraction into the renderer.
    EXPECT_EQ(scenePixel[0], 6U);
    EXPECT_EQ(scenePixel[1], 13U);
    EXPECT_EQ(scenePixel[2], 25U);
    EXPECT_EQ(postRenderGlError, GL_NO_ERROR);

    engine.Shutdown();
}

// Exercises the actual EngineCore -> Renderer3DUVE -> tone-mapping -> GLFW default-framebuffer
// path. The fixed positions are intentionally far from geometry edges so a future expected-pixel
// assertion cannot be satisfied by the background alone.
TEST(EngineCoreUVETest, WindowedMode_PresentsDeterministicPrimitiveFixtureToDefaultFramebuffer) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.headlessUVE = false;
    config.windowWidth = 128U;
    config.windowHeight = 96U;
    config.renderTargetWidth = 128U;
    config.renderTargetHeight = 96U;
    config.vsyncEnabledUVE = false;
    config.windowGlVersionMajor = 4U;
    config.windowGlVersionMinor = 5U;
    config.ambientColor = Math::Vector3UVE{0.45F, 0.45F, 0.45F};

    EngineCoreUVE engine(config);
    engine.Init();
    if (!engine.GetServicesUVE().GetWindowManagerUVE().IsValidUVE()) {
        GTEST_SKIP() << "No display available for windowed EngineCoreUVE - skipping (run under "
                        "xvfb-run to exercise this test)";
    }
    ASSERT_TRUE(engine.Load());

    EngineServicesUVE& services = engine.GetServicesUVE();
    Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = services.GetSceneGraphUVE();
    const Scene::EntityUVE camera = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, camera, Scene::TransformComponentUVE{});
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(camera);
    engine.SetActiveCameraUVE(camera);

    Math::QuaternionUVE planeFacingCamera{};
    ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                          std::numbers::pi_v<float> * 0.5F, planeFacingCamera));
    const auto makePrimitive = [&entityManager, &sceneGraph](const Math::Vector3UVE position,
                                                               const Math::QuaternionUVE rotation,
                                                               const Math::Vector3UVE scale,
                                                               const Scene::PrimitiveMeshKindUVE kind,
                                                               const Math::Vector3UVE color) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform{};
        transform.localPosition = position;
        transform.localRotation = rotation;
        transform.localScale = scale;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(
            entity, Scene::PrimitiveMeshComponentUVE{kind, color});
    };
    makePrimitive(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, planeFacingCamera, Math::Vector3UVE{6.0F, 6.0F, 1.0F},
                  Scene::PrimitiveMeshKindUVE::Plane, Math::Vector3UVE{0.10F, 0.72F, 0.20F});
    makePrimitive(Math::Vector3UVE{-3.5F, 0.0F, -8.0F}, Math::QuaternionUVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F},
                  Scene::PrimitiveMeshKindUVE::Cube, Math::Vector3UVE{0.88F, 0.10F, 0.06F});
    makePrimitive(Math::Vector3UVE{-1.8F, 0.0F, -7.0F}, Math::QuaternionUVE{}, Math::Vector3UVE{1.4F, 1.4F, 1.4F},
                  Scene::PrimitiveMeshKindUVE::UVSphere, Math::Vector3UVE{0.06F, 0.20F, 0.90F});
    sceneGraph.UpdateUVE(entityManager);

    std::array<unsigned char, 3> cube{};
    std::array<unsigned char, 3> plane{};
    std::array<unsigned char, 3> sphere{};
    GLenum postRenderGlError = GL_NO_ERROR;
    engine.SetPostRenderCallbackUVE([&cube, &plane, &sphere, &postRenderGlError] {
        // The callback runs after Renderer3DUVE’s default-framebuffer tone-map pass and before
        // EngineCoreUVE requests SwapBuffersUVE(). GL_BACK is therefore the exact presentation
        // surface being handed to the window manager, unlike post-swap front/back reads.
        glFinish();
        glReadBuffer(GL_BACK);
        // These center-of-raster samples are intentionally well inside the fixed fixture's
        // projected geometry: red cube, green plane, then blue UV sphere.
        glReadPixels(41, 85, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, cube.data());
        glReadPixels(103, 89, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, plane.data());
        glReadPixels(79, 80, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, sphere.data());
        postRenderGlError = glGetError();
    });
    for (int frame = 0; frame < 12; ++frame) {
        engine.TickFrameUVE();
    }
    engine.SetPostRenderCallbackUVE({});

    const Render::Renderer3DFrameDiagnosticsUVE diagnostics = services.GetRenderer3DUVE().GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(diagnostics.primitiveCandidates, 3U);
    EXPECT_EQ(diagnostics.primitiveItemsExtracted, 3U);
    EXPECT_EQ(diagnostics.primitiveDrawCallsRecorded, 3U);
    EXPECT_EQ(diagnostics.glDrawCallsIssued, 3U);
    EXPECT_TRUE(diagnostics.toneMappingPassRecorded);

    EXPECT_GT(cube[0], static_cast<unsigned char>(cube[1] + 12U)) << "cube RGB=" << static_cast<int>(cube[0])
                                                                     << ',' << static_cast<int>(cube[1]) << ','
                                                                     << static_cast<int>(cube[2]);
    EXPECT_GT(plane[1], static_cast<unsigned char>(plane[0] + 12U)) << "plane RGB=" << static_cast<int>(plane[0])
                                                                      << ',' << static_cast<int>(plane[1]) << ','
                                                                      << static_cast<int>(plane[2]);
    EXPECT_GT(sphere[2], static_cast<unsigned char>(sphere[0] + 12U)) << "sphere RGB=" << static_cast<int>(sphere[0])
                                                                        << ',' << static_cast<int>(sphere[1]) << ','
                                                                        << static_cast<int>(sphere[2]);
    EXPECT_EQ(postRenderGlError, GL_NO_ERROR);

    engine.Shutdown();
}

TEST(EngineCoreUVETest, PostRenderCallback_HeadlessModeDoesNotInvokeOverlay) {
    EngineConfigUVE config = MakeTestConfigUVE();
    config.headlessUVE = true;
    EngineCoreUVE engine(config);
    engine.Init();
    ASSERT_TRUE(engine.Load());

    int callbackCount = 0;
    engine.SetPostRenderCallbackUVE([&callbackCount] { ++callbackCount; });
    engine.TickFrameUVE();
    EXPECT_EQ(callbackCount, 0);

    engine.SetPostRenderCallbackUVE({});
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
