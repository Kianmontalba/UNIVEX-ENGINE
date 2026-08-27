// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/core/engine_services_uve.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/i_asset_bundle_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_importer_uve.h"
#include "uve/asset/i_asset_import_queue_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/asset/i_derived_artifact_cache_uve.h"
#include "uve/asset/i_file_system_uve.h"
#include "uve/asset/i_hot_reload_uve.h"
#include "uve/asset/i_project_file_index_uve.h"
#include "uve/audio/i_audio_device_uve.h"
#include "uve/audio/i_audio_source_system_uve.h"
#include "uve/audio/i_audio_system_uve.h"
#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/input/gamepad_input_system_uve.h"
#include "uve/input/i_input_system_uve.h"
#include "uve/input/mobile_gesture_system_uve.h"
#include "uve/input/mobile_input_system_uve.h"
#include "uve/math/frustum_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/physics/physics_constraint_system_uve.h"
#include "uve/physics/physics_query_system_uve.h"
#include "uve/physics/i_raycast_system_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_light_system_uve.h"
#include "uve/render/i_mesh_renderer_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/render/i_renderer_3d_uve.h"
#include "uve/render/shader/i_shader_manager_uve.h"
#include "uve/save/i_checkpoint_manager_uve.h"
#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_particle_runtime_uve.h"
#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"
#include "uve/window/i_window_manager_uve.h"

// These hand-written fakes exist to prove that EngineServicesUVE (and, by
// extension, anything that consumes ILoggerUVE/ITimerUVE/IEventSystemUVE/
// IMemoryManagerUVE/IThreadPoolUVE/ICommandLineUVE/IConfigManagerUVE/
// IEntityManagerUVE/ISceneGraphUVE/IAssetDatabaseUVE/ISceneSerializerUVE/
// IPrefabSystemUVE/IHotReloadUVE/IAssetManagerUVE/IAssetImporterUVE/
// IAssetBundleUVE/IFileSystemUVE/IRenderDeviceUVE/Shader::IShaderManagerUVE/IRenderSystemUVE/
// ICameraSystemUVE/IMeshRendererUVE) works against ANY conforming
// implementation, independent of the concrete LoggerUVE/TimerUVE/
// EventSystemUVE/MemoryManagerUVE/ThreadPoolUVE/CommandLineUVE/
// ConfigManagerUVE/EntityManagerUVE/SceneGraphUVE/AssetDatabaseUVE/
// SceneSerializerUVE/PrefabSystemUVE/HotReloadUVE/AssetManagerUVE/
// AssetImporterUVE/AssetBundleUVE/FileSystemUVE/NullRenderDeviceUVE/
// RenderSystemUVE/CameraSystemUVE/MeshRendererUVE classes used by
// EngineCoreUVE — this is the whole point of introducing the interfaces.

namespace UVE::Core::Tests {
namespace {

class FakeLoggerUVE final : public Debug::ILoggerUVE {
public:
    void Init(Debug::LogLevelUVE) override {}
    void Shutdown() override {}
    void AddSink(std::unique_ptr<Debug::ILogSinkUVE>) override {}
    void SetMinLevel(Debug::LogLevelUVE level) override { minLevel = level; }
    [[nodiscard]] Debug::LogLevelUVE GetMinLevel() const override { return minLevel; }
    void LogFormatted(Debug::LogLevelUVE, std::string_view, const char*, int, std::string) override {}

    Debug::LogLevelUVE minLevel = Debug::LogLevelUVE::Trace;
};

class FakeTimerUVE final : public Utilities::ITimerUVE {
public:
    void Tick() override { ++tickCount; }
    [[nodiscard]] double GetDeltaTimeUVE() const override { return 0.0; }
    [[nodiscard]] float GetDeltaTimeFloatUVE() const override { return 0.0F; }
    [[nodiscard]] double GetTotalTimeUVE() const override { return 0.0; }
    void Reset() override {}
    void SetMaxDeltaTimeUVE(double) override {}
    void SetFixedTimestepUVE(double) override {}
    Utilities::FixedStepResultUVE AdvanceFixedStepUVE() override { return {}; }
    void DiscardFixedStepAccumulatorUVE() noexcept override { ++discardCount; }

    int tickCount = 0;
    int discardCount = 0;
};

class FakeEventSystemUVE final : public Events::IEventSystemUVE {
public:
    void Unsubscribe(const Events::EventSubscriptionUVE&) override {}
    void DispatchQueuedUVE() override { ++dispatchCount; }
    void Clear() override {}

    int dispatchCount = 0;

protected:
    std::uint64_t SubscribeErased(std::type_index, std::function<void(const void*)>) override { return 1; }
    void PublishErased(std::type_index, const void*) override {}
    void QueueErased(std::type_index, Events::EventPriorityUVE, std::function<void()>) override {}
};

class FakeAllocatorUVE final : public Memory::IAllocatorUVE {
public:
    [[nodiscard]] void* AllocateUVE(std::size_t, std::size_t, const char*, int) override {
        return nullptr;
    }
    void DeallocateUVE(void*) override {}
    [[nodiscard]] std::size_t GetAllocatedBytesUVE() const override { return 0; }
};

class FakeMemoryManagerUVE final : public Memory::IMemoryManagerUVE {
public:
    void RecordAllocationUVE(void*, std::size_t, std::size_t, std::string_view, const char*,
                              int) override {
        ++recordAllocationCount;
    }
    void RecordDeallocationUVE(void*) override { ++recordDeallocationCount; }
    [[nodiscard]] Memory::IAllocatorUVE& GetDefaultAllocatorUVE() override { return allocator; }
    [[nodiscard]] std::size_t GetActiveAllocationCountUVE() const override { return 0; }
    [[nodiscard]] std::size_t GetActiveBytesUVE() const override { return 0; }
    [[nodiscard]] std::size_t GetPeakBytesUVE() const override { return 0; }
    [[nodiscard]] std::uint64_t GetTotalAllocationsEverUVE() const override { return 0; }
    [[nodiscard]] bool HasLeaksUVE() const override { return false; }
    [[nodiscard]] std::vector<Memory::AllocationRecordUVE> GetLeakedAllocationsUVE() const override {
        return {};
    }
    void LogLeakReportUVE() override { ++logLeakReportCallCount; }

    FakeAllocatorUVE allocator;
    int recordAllocationCount = 0;
    int recordDeallocationCount = 0;
    int logLeakReportCallCount = 0;
};

class FakeThreadPoolUVE final : public Threading::IThreadPoolUVE {
public:
    void SubmitUVE(Threading::JobUVE job) override {
        ++submitCount;
        job();
    }
    void SubmitUVE(Threading::JobUVE job, Threading::JobCounterUVE& counter) override {
        ++submitCount;
        counter.IncrementUVE();
        job();
        counter.DecrementAndNotifyUVE();
    }
    [[nodiscard]] std::size_t GetWorkerCountUVE() const noexcept override { return 1; }
    [[nodiscard]] std::size_t GetPendingJobCountUVE() const noexcept override { return 0; }
    [[nodiscard]] std::size_t GetActiveWorkerCountUVE() const noexcept override { return 0; }
    [[nodiscard]] std::uint64_t GetStolenJobCountUVE() const noexcept override { return 0; }
    [[nodiscard]] bool IsWorkerThreadUVE() const noexcept override { return false; }

    int submitCount = 0;
};

class FakeCommandLineUVE final : public CommandLine::ICommandLineUVE {
public:
    [[nodiscard]] bool HasFlagUVE(std::string_view name) const noexcept override {
        ++hasFlagCallCount;
        return name == "server";
    }
    [[nodiscard]] std::string GetValueUVE(std::string_view /*name*/,
                                           std::string_view defaultValue) const override {
        return std::string(defaultValue);
    }

    mutable int hasFlagCallCount = 0;
};

class FakeConfigManagerUVE final : public Config::IConfigManagerUVE {
public:
    bool LoadUVE(const std::filesystem::path&) override { return true; }
    bool SaveUVE() override {
        ++saveCallCount;
        return true;
    }
    bool SaveUVE(const std::filesystem::path&) override {
        ++saveCallCount;
        return true;
    }
    [[nodiscard]] std::string GetStringUVE(std::string_view,
                                            std::string_view defaultValue) const override {
        return std::string(defaultValue);
    }
    [[nodiscard]] std::int64_t GetIntUVE(std::string_view, std::int64_t defaultValue) const override {
        return defaultValue;
    }
    [[nodiscard]] double GetDoubleUVE(std::string_view, double defaultValue) const override {
        return defaultValue;
    }
    [[nodiscard]] bool GetBoolUVE(std::string_view, bool defaultValue) const override {
        return defaultValue;
    }
    void SetStringUVE(std::string_view, std::string) override {}
    void SetIntUVE(std::string_view, std::int64_t) override {}
    void SetDoubleUVE(std::string_view, double) override {}
    void SetBoolUVE(std::string_view, bool) override {}
    [[nodiscard]] bool HasKeyUVE(std::string_view) const override { return false; }

    int saveCallCount = 0;
};

class FakeEntityManagerUVE final : public Scene::IEntityManagerUVE {
public:
    [[nodiscard]] Scene::EntityUVE CreateEntityUVE() override {
        ++createEntityCallCount;
        return Scene::EntityUVE{0, 0};
    }
    void DestroyEntityUVE(Scene::EntityUVE) override { ++destroyEntityCallCount; }
    [[nodiscard]] bool IsAliveUVE(Scene::EntityUVE) const noexcept override { return true; }
    [[nodiscard]] std::size_t GetEntityCountUVE() const noexcept override { return 0; }
    [[nodiscard]] std::vector<std::type_index> GetComponentTypesUVE(Scene::EntityUVE) const override {
        return {};
    }

    int createEntityCallCount = 0;
    int destroyEntityCallCount = 0;

protected:
    [[nodiscard]] void* AddComponentErased(Scene::EntityUVE, std::type_index,
                                            const Scene::ComponentTypeInfoUVE&) override {
        return nullptr;
    }
    void RemoveComponentErased(Scene::EntityUVE, std::type_index) override {}
    [[nodiscard]] bool HasComponentErased(Scene::EntityUVE, std::type_index) const override { return false; }
    [[nodiscard]] void* GetComponentErased(Scene::EntityUVE, std::type_index) override { return nullptr; }
    void ForEachErased(const std::vector<std::type_index>&,
                        const std::function<void(Scene::EntityUVE, const std::vector<void*>&)>&) override {}
};

class FakeSceneGraphUVE final : public Scene::ISceneGraphUVE {
public:
    void AttachTransformUVE(Scene::IEntityManagerUVE&, Scene::EntityUVE,
                             const Scene::TransformComponentUVE&) override {}
    void SetLocalTransformUVE(Scene::IEntityManagerUVE&, Scene::EntityUVE,
                               const Scene::TransformComponentUVE&) override {}
    void SetParentUVE(Scene::IEntityManagerUVE&, Scene::EntityUVE, Scene::EntityUVE) override {}
    void UpdateUVE(Scene::IEntityManagerUVE&) override { ++updateCallCount; }
    [[nodiscard]] std::vector<Scene::EntityUVE> GetChildrenUVE(Scene::IEntityManagerUVE&,
                                                                Scene::EntityUVE) override {
        return {};
    }

    int updateCallCount = 0;
};

class FakeParticleRuntimeUVE final : public Scene::IParticleRuntimeUVE {
public:
    [[nodiscard]] Scene::ParticleRuntimeResultUVE AttachDetailedUVE(
        Scene::EntityUVE, const Scene::ParticleEmitterComponentUVE&) override {
        ++attachCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "attached"};
    }
    [[nodiscard]] Scene::ParticleRuntimeResultUVE DetachDetailedUVE(Scene::EntityUVE) noexcept override {
        ++detachCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "detached"};
    }
    [[nodiscard]] Scene::ParticleRuntimeResultUVE SetEnabledDetailedUVE(Scene::EntityUVE, bool) noexcept override {
        ++setEnabledCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "enabled"};
    }
    [[nodiscard]] Scene::ParticleRuntimeResultUVE SetLiveParticleCountDetailedUVE(
        Scene::EntityUVE, std::uint32_t) noexcept override {
        ++setLiveParticleCountCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "live count"};
    }
    [[nodiscard]] Scene::ParticleRuntimeResultUVE EmitDetailedUVE(
        Scene::EntityUVE, const Scene::ParticleEmissionUVE&) override {
        ++emitCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "emitted"};
    }
    [[nodiscard]] Scene::ParticleRuntimeResultUVE SimulateDetailedUVE(
        float, const Math::Vector3UVE&) noexcept override {
        ++simulateCallCount;
        return {Scene::ParticleRuntimeCodeUVE::Applied, "simulated"};
    }
    [[nodiscard]] Scene::ParticleRuntimeSnapshotUVE GetSnapshotUVE() const override {
        ++snapshotCallCount;
        return {};
    }
    [[nodiscard]] std::optional<Scene::ParticleStateSnapshotUVE> GetParticleSnapshotUVE(
        Scene::EntityUVE) const override {
        ++particleSnapshotCallCount;
        return std::nullopt;
    }
    [[nodiscard]] bool HasInstanceUVE(Scene::EntityUVE) const noexcept override { return false; }
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept override { return 0U; }
    [[nodiscard]] std::uint64_t GetTotalBudgetUVE() const noexcept override { return 0U; }

    mutable int snapshotCallCount = 0;
    mutable int particleSnapshotCallCount = 0;
    int attachCallCount = 0;
    int detachCallCount = 0;
    int setEnabledCallCount = 0;
    int setLiveParticleCountCallCount = 0;
    int emitCallCount = 0;
    int simulateCallCount = 0;
};

class FakeAssetDatabaseUVE final : public Asset::IAssetDatabaseUVE {
public:
    bool LoadUVE(const std::filesystem::path&) override { return true; }
    bool SaveUVE() override {
        ++saveCallCount;
        return true;
    }
    bool SaveUVE(const std::filesystem::path&) override {
        ++saveCallCount;
        return true;
    }
    [[nodiscard]] Asset::AssetGuidUVE RegisterUVE(const std::filesystem::path&) override {
        return Asset::AssetGuidUVE{1};
    }
    [[nodiscard]] std::filesystem::path ResolveUVE(Asset::AssetGuidUVE) const override { return {}; }
    [[nodiscard]] bool HasGuidUVE(Asset::AssetGuidUVE) const override { return false; }
    [[nodiscard]] std::vector<Asset::AssetRecordUVE> GetRegisteredAssetsUVE() const override { return {}; }

    int saveCallCount = 0;
};

class FakeProjectFileIndexUVE final : public Asset::IProjectFileIndexUVE {
public:
    bool RefreshUVE(const Asset::IAssetDatabaseUVE&) override {
        ++refreshCallCount;
        ++snapshot.refreshGeneration;
        return true;
    }

    [[nodiscard]] Asset::ProjectFileSnapshotUVE GetSnapshotUVE() const override { return snapshot; }

    int refreshCallCount = 0;
    Asset::ProjectFileSnapshotUVE snapshot;
};

class FakeProjectChangeWatcherUVE final : public Asset::IProjectChangeWatcherUVE {
public:
    [[nodiscard]] bool PollUVE(double, const Asset::IAssetDatabaseUVE&,
                               Asset::IDerivedArtifactCacheUVE&) override {
        ++pollCallCount;
        return true;
    }

    [[nodiscard]] bool PollNowUVE(const Asset::IAssetDatabaseUVE&,
                                  Asset::IDerivedArtifactCacheUVE&) override {
        ++pollNowCallCount;
        return true;
    }

    [[nodiscard]] Asset::ProjectChangeSnapshotUVE GetSnapshotUVE() const override { return snapshot; }

    void AcknowledgeThroughUVE(std::uint64_t sequence) override { acknowledgedThrough = sequence; }

    void AcknowledgeRescanUVE() override { snapshot.rescanRequired = false; }

    int pollCallCount = 0;
    int pollNowCallCount = 0;
    std::uint64_t acknowledgedThrough = 0U;
    Asset::ProjectChangeSnapshotUVE snapshot;
};

class FakeDerivedArtifactCacheUVE final : public Asset::IDerivedArtifactCacheUVE {
public:
    [[nodiscard]] std::optional<Asset::DerivedArtifactCacheRecordUVE>
    LoadImportRecordUVE(const std::filesystem::path&) const override {
        ++loadCallCount;
        return std::nullopt;
    }

    [[nodiscard]] bool StoreImportRecordUVE(const std::filesystem::path&,
                                             const Asset::DerivedArtifactCacheRecordUVE&) override {
        ++storeCallCount;
        return true;
    }

    [[nodiscard]] std::size_t MarkStaleForSourceUVE(const std::filesystem::path&) override {
        ++markStaleCallCount;
        return 0U;
    }

    [[nodiscard]] std::filesystem::path GetCacheRootUVE() const override { return "fake-derived-cache"; }

    mutable int loadCallCount = 0;
    int storeCallCount = 0;
    int markStaleCallCount = 0;
};

class FakeAssetImportQueueUVE final : public Asset::IAssetImportQueueUVE {
public:
    [[nodiscard]] std::optional<Asset::AssetImportJobIdUVE> EnqueueUVE(Asset::AssetImportRequestUVE) override {
        ++enqueueCallCount;
        return Asset::AssetImportJobIdUVE{1U};
    }

    [[nodiscard]] bool TickUVE() override {
        ++tickCallCount;
        return false;
    }

    [[nodiscard]] bool RetryUVE(Asset::AssetImportJobIdUVE) override {
        ++retryCallCount;
        return false;
    }

    [[nodiscard]] std::vector<Asset::AssetImportJobUVE> GetJobsUVE() const override { return {}; }

    int enqueueCallCount = 0;
    int tickCallCount = 0;
    int retryCallCount = 0;
};

class FakeSceneSerializerUVE final : public Scene::ISceneSerializerUVE {
public:
    [[nodiscard]] std::optional<Scene::SceneSnapshotUVE> CaptureUVE(
        Scene::IEntityManagerUVE&, const std::vector<Scene::EntityUVE>&, Scene::SceneAssetTypeUVE assetType) const override {
        return Scene::SceneSnapshotUVE{{}, assetType};
    }
    [[nodiscard]] std::vector<Scene::EntityUVE> RestoreUVE(Scene::IEntityManagerUVE&,
                                                            const Scene::SceneSnapshotUVE&) const override {
        return {};
    }
    [[nodiscard]] bool SaveUVE(Scene::IEntityManagerUVE&, const std::vector<Scene::EntityUVE>&,
                                const std::filesystem::path&, Scene::SceneAssetTypeUVE) override {
        ++saveCallCount;
        return true;
    }
    [[nodiscard]] std::vector<Scene::EntityUVE> LoadUVE(Scene::IEntityManagerUVE&,
                                                          const std::filesystem::path&) override {
        return {};
    }

    int saveCallCount = 0;
};

class FakePrefabSystemUVE final : public Scene::IPrefabSystemUVE {
public:
    [[nodiscard]] Asset::AssetGuidUVE SavePrefabUVE(Scene::IEntityManagerUVE&,
                                                     Asset::IAssetDatabaseUVE&, Scene::EntityUVE,
                                                     const std::filesystem::path&) override {
        ++savePrefabCallCount;
        return Asset::AssetGuidUVE{1};
    }
    [[nodiscard]] Scene::EntityUVE InstantiateUVE(Scene::IEntityManagerUVE&, Scene::ISceneGraphUVE&,
                                                   Asset::IAssetDatabaseUVE&, Asset::AssetGuidUVE,
                                                   Scene::EntityUVE) override {
        return Scene::kInvalidEntityUVE;
    }
    [[nodiscard]] Scene::PrefabRefreshResultUVE RefreshInstanceUVE(
        Scene::IEntityManagerUVE&, Scene::ISceneGraphUVE&, Asset::IAssetDatabaseUVE&,
        Scene::EntityUVE instanceRoot, bool) override {
        return {Scene::PrefabRefreshCodeUVE::InvalidInstance, instanceRoot, 0U, "fake"};
    }

    int savePrefabCallCount = 0;
};

class FakeHotReloadUVE final : public Asset::IHotReloadUVE {
public:
    void TrackUVE(Asset::AssetGuidUVE) override {}
    void UntrackUVE(Asset::AssetGuidUVE) override {}
    void PollUVE(Asset::IAssetManagerUVE&, Asset::IAssetDatabaseUVE&, double) override { ++pollCallCount; }

    int pollCallCount = 0;
};

class FakeAssetManagerUVE final : public Asset::IAssetManagerUVE {
public:
    void CollectGarbageUVE() override { ++collectGarbageCallCount; }
    void ReloadUVE(Asset::AssetGuidUVE, Asset::IAssetDatabaseUVE&) override { ++reloadCallCount; }
    [[nodiscard]] std::size_t GetLoadedAssetCountUVE() const override { return 0; }

    int collectGarbageCallCount = 0;
    int reloadCallCount = 0;

protected:
    void RegisterLoaderErased(std::type_index, Asset::AssetLoaderInfoUVE) override {}
    void LoadErased(Asset::AssetGuidUVE, std::type_index, Asset::IAssetDatabaseUVE&) override {}
    [[nodiscard]] Asset::AssetLoadStateUVE GetStateErased(Asset::AssetGuidUVE) const override {
        return Asset::AssetLoadStateUVE::NotLoaded;
    }
    [[nodiscard]] std::string GetFailureReasonErased(Asset::AssetGuidUVE) const override { return {}; }
    [[nodiscard]] void* TryGetErased(Asset::AssetGuidUVE) override { return nullptr; }
    void AddRefErased(Asset::AssetGuidUVE) override {}
    void ReleaseErased(Asset::AssetGuidUVE) override {}
};

class FakeAssetImporterUVE final : public Asset::IAssetImporterUVE {
public:
    void RegisterImporterUVE(std::string,
                              std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                                                  const Asset::AssetImportSettingsUVE&)>) override {}
    [[nodiscard]] Asset::AssetGuidUVE ImportUVE(const std::filesystem::path&, const std::filesystem::path&,
                                                 Asset::IAssetDatabaseUVE&,
                                                 const Asset::AssetImportSettingsUVE&) override {
        ++importCallCount;
        return Asset::AssetGuidUVE{1};
    }

    int importCallCount = 0;
};

class FakeAssetBundleUVE final : public Asset::IAssetBundleUVE {
public:
    [[nodiscard]] bool PackUVE(const std::vector<Asset::AssetBundleEntryUVE>&,
                                const std::filesystem::path&) override {
        ++packCallCount;
        return true;
    }
    [[nodiscard]] bool UnpackUVE(const std::filesystem::path&, const std::filesystem::path&) override {
        return true;
    }
    [[nodiscard]] bool HasEntryUVE(const std::filesystem::path&, std::string_view) const override {
        return false;
    }
    [[nodiscard]] std::optional<std::vector<std::byte>> ReadEntryUVE(const std::filesystem::path&,
                                                                       std::string_view) const override {
        return std::nullopt;
    }

    int packCallCount = 0;
};

class FakeFileSystemUVE final : public Asset::IFileSystemUVE {
public:
    [[nodiscard]] Asset::MountHandleUVE MountDirectoryUVE(std::string, std::filesystem::path, int) override {
        return 1;
    }
    [[nodiscard]] Asset::MountHandleUVE MountBundleUVE(std::string, std::filesystem::path, int) override {
        return 1;
    }
    void UnmountUVE(Asset::MountHandleUVE) override {}
    [[nodiscard]] bool HasFileUVE(std::string_view) const override { return false; }
    [[nodiscard]] std::optional<std::vector<std::byte>> ReadFileUVE(std::string_view) const override {
        return std::nullopt;
    }
    [[nodiscard]] bool WriteFileUVE(std::string_view, const std::vector<std::byte>&) override {
        ++writeCallCount;
        return true;
    }
    [[nodiscard]] std::filesystem::path ResolveRealPathUVE(std::string_view) const override { return {}; }

    int writeCallCount = 0;
};

class FakeCommandBufferUVE final : public Render::ICommandBufferUVE {
public:
    void BeginRenderPassUVE(const Render::RenderPassDescUVE&) override {}
    void EndRenderPassUVE() override {}
    void BindPipelineUVE(Render::PipelineHandleUVE) override {}
    void BindVertexBufferUVE(Render::BufferHandleUVE, std::uint32_t) override {}
    void BindIndexBufferUVE(Render::BufferHandleUVE) override {}
    void BindTextureUVE(Render::TextureHandleUVE, std::uint32_t) override {}
    void BindUniformBufferUVE(Render::BufferHandleUVE, std::uint32_t) override {}
    void DrawIndexedUVE(std::uint32_t, std::uint32_t) override {}
    void DrawUVE(std::uint32_t, std::uint32_t) override {}
    void SetUniformFloatUVE(std::string_view, float) override {}
    void SetUniformIntUVE(std::string_view, std::int32_t) override {}
    void SetUniformBoolUVE(std::string_view, bool) override {}
    void SetUniformVector3UVE(std::string_view, const Math::Vector3UVE&) override {}
    void SetUniformMatrix4x4UVE(std::string_view, const Math::Matrix4x4UVE&) override {}
};

class FakeRenderDeviceUVE final : public Render::IRenderDeviceUVE {
public:
    [[nodiscard]] Render::BufferHandleUVE CreateBufferUVE(const Render::BufferDescUVE&,
                                                            std::span<const std::byte>) override {
        ++createBufferCallCount;
        return Render::BufferHandleUVE{1};
    }
    void DestroyBufferUVE(Render::BufferHandleUVE) override {}
    [[nodiscard]] bool UpdateBufferUVE(Render::BufferHandleUVE, std::span<const std::byte>, std::uint64_t) override {
        return true;
    }
    [[nodiscard]] Render::TextureHandleUVE CreateTextureUVE(const Render::TextureDescUVE&,
                                                              std::span<const std::byte>) override {
        return Render::TextureHandleUVE{1};
    }
    void DestroyTextureUVE(Render::TextureHandleUVE) override {}
    [[nodiscard]] Render::ShaderHandleUVE CreateShaderUVE(const Render::ShaderDescUVE&, std::string*) override {
        return Render::ShaderHandleUVE{1};
    }
    void DestroyShaderUVE(Render::ShaderHandleUVE) override {}
    [[nodiscard]] Render::PipelineHandleUVE CreatePipelineUVE(const Render::PipelineDescUVE&, std::string*) override {
        return Render::PipelineHandleUVE{1};
    }
    void DestroyPipelineUVE(Render::PipelineHandleUVE) override {}
    [[nodiscard]] std::vector<Render::UniformReflectionUVE> GetPipelineUniformsUVE(
        Render::PipelineHandleUVE) const override {
        return {};
    }
    [[nodiscard]] bool GetPipelineBinaryUVE(Render::PipelineHandleUVE, std::vector<std::byte>&,
                                             std::uint32_t&) const override {
        return false;
    }
    [[nodiscard]] Render::PipelineHandleUVE CreatePipelineFromBinaryUVE(
        std::span<const std::byte>, std::uint32_t, const Render::PipelineBinaryDescUVE&) override {
        return Render::PipelineHandleUVE{1};
    }
    [[nodiscard]] std::unique_ptr<Render::ICommandBufferUVE> CreateCommandBufferUVE() override { return nullptr; }
    void SubmitUVE(std::unique_ptr<Render::ICommandBufferUVE>) override {}
    void PresentUVE() override { ++presentCallCount; }
    [[nodiscard]] bool IsUsableUVE() const noexcept override { return true; }
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override { return "Fake"; }

    int createBufferCallCount = 0;
    int presentCallCount = 0;
};

class FakeShaderManagerUVE final : public Render::Shader::IShaderManagerUVE {
public:
    [[nodiscard]] std::shared_ptr<Render::Shader::ShaderSourceUVE> CreateSourceUVE(
        const Render::Shader::ShaderSourceCompileDescUVE&) override {
        return nullptr;
    }
    [[nodiscard]] std::shared_ptr<Render::Shader::ShaderProgramUVE> CreateProgramUVE(
        const Render::Shader::ShaderProgramDescUVE&) override {
        return nullptr;
    }
    [[nodiscard]] std::shared_ptr<Render::Shader::ShaderProgramUVE> CreateProgramFromStagesUVE(
        const Render::Shader::ShaderProgramStagesDescUVE&) override {
        return nullptr;
    }
    void UpdateUVE(double) override { ++updateCallCount; }

    int updateCallCount = 0;
};

class FakeRenderSystemUVE final : public Render::IRenderSystemUVE {
public:
    void BeginFrameUVE() override { ++beginFrameCallCount; }
    [[nodiscard]] Render::ICommandBufferUVE& GetFrameCommandBufferUVE() override { return commandBuffer; }
    void EndFrameUVE() override {}
    [[nodiscard]] std::uint64_t GetFrameIndexUVE() const noexcept override { return 0; }

    FakeCommandBufferUVE commandBuffer;
    int beginFrameCallCount = 0;
};

class FakeCameraSystemUVE final : public Render::ICameraSystemUVE {
public:
    [[nodiscard]] Math::Matrix4x4UVE ComputeViewMatrixUVE(const Scene::IEntityManagerUVE&,
                                                            Scene::EntityUVE) const override {
        ++computeViewCallCount;
        return Math::Matrix4x4UVE::IdentityUVE();
    }
    [[nodiscard]] Math::Matrix4x4UVE ComputeProjectionMatrixUVE(const Scene::IEntityManagerUVE&, Scene::EntityUVE,
                                                                 float) const override {
        return Math::Matrix4x4UVE::IdentityUVE();
    }
    [[nodiscard]] Math::Matrix4x4UVE ComputeViewProjectionUVE(const Scene::IEntityManagerUVE&, Scene::EntityUVE,
                                                               float) const override {
        return Math::Matrix4x4UVE::IdentityUVE();
    }
    [[nodiscard]] Math::FrustumUVE ExtractFrustumUVE(const Math::Matrix4x4UVE&) const override { return {}; }
    [[nodiscard]] Render::CameraFrustumCornersUVE ComputeFrustumCornersUVE(
        const Scene::IEntityManagerUVE&, Scene::EntityUVE, float) const override {
        return {};
    }
    [[nodiscard]] Math::Vector3UVE GetWorldPositionUVE(const Scene::IEntityManagerUVE&,
                                                          Scene::EntityUVE) const override {
        ++getWorldPositionCallCount;
        return {};
    }

    mutable int computeViewCallCount = 0;
    mutable int getWorldPositionCallCount = 0;
};

class FakeMeshRendererUVE final : public Render::IMeshRendererUVE {
public:
    [[nodiscard]] Render::RenderQueueUVE ExtractRenderQueueUVE(Scene::IEntityManagerUVE&, Asset::IAssetManagerUVE&,
                                                                 Asset::IAssetDatabaseUVE&,
                                                                 const Math::FrustumUVE&) const override {
        ++extractRenderQueueCallCount;
        return {};
    }

    mutable int extractRenderQueueCallCount = 0;
};

class FakeLightSystemUVE final : public Render::ILightSystemUVE {
public:
    [[nodiscard]] Render::LightListUVE ExtractActiveLightsUVE(Scene::IEntityManagerUVE&) const override {
        ++extractActiveLightCallCount;
        return {};
    }

    mutable int extractActiveLightCallCount = 0;
};

class FakeRenderer3DUVE final : public Render::IRenderer3DUVE {
public:
    void RenderFrameUVE(Scene::IEntityManagerUVE&, Scene::EntityUVE) override { ++renderFrameCallCount; }

    [[nodiscard]] Render::Renderer3DFrameDiagnosticsUVE GetLastFrameDiagnosticsUVE() const noexcept override {
        ++getDiagnosticsCallCount;
        return diagnostics;
    }

    int renderFrameCallCount = 0;
    mutable int getDiagnosticsCallCount = 0;
    Render::Renderer3DFrameDiagnosticsUVE diagnostics{};
};

class FakeCollisionSystemUVE final : public Physics::ICollisionSystemUVE {
public:
    [[nodiscard]] std::vector<Physics::CollisionPairUVE> DetectCollisionsUVE(
        Scene::IEntityManagerUVE&) const override {
        ++detectCollisionsCallCount;
        return {};
    }

    mutable int detectCollisionsCallCount = 0;
};

class FakePhysicsSystemUVE final : public Physics::IPhysicsSystemUVE {
public:
    void StepUVE(Scene::IEntityManagerUVE&, Scene::ISceneGraphUVE&, float) override { ++stepCallCount; }

    int stepCallCount = 0;
};

class FakeRaycastSystemUVE final : public Physics::IRaycastSystemUVE {
public:
    [[nodiscard]] std::optional<Physics::RaycastHitUVE> RaycastUVE(
        Scene::IEntityManagerUVE&, const Physics::RaycastQueryUVE&) const override {
        ++raycastCallCount;
        return std::nullopt;
    }

    mutable int raycastCallCount = 0;
};

class FakeInputSystemUVE final : public Input::IInputSystemUVE {
public:
    void SetKeyStateUVE(Input::KeyCodeUVE, bool) override {}
    void SetMouseButtonStateUVE(Input::MouseButtonUVE, bool) override {}
    void SetMousePositionUVE(Math::Vector2UVE) override {}
    void SetMouseScrollDeltaUVE(float) override {}
    void UpdateUVE() override { ++updateCallCount; }

    [[nodiscard]] bool IsKeyDownUVE(Input::KeyCodeUVE) const override { return false; }
    [[nodiscard]] bool WasKeyPressedThisFrameUVE(Input::KeyCodeUVE) const override { return false; }
    [[nodiscard]] bool WasKeyReleasedThisFrameUVE(Input::KeyCodeUVE) const override { return false; }
    [[nodiscard]] bool IsMouseButtonDownUVE(Input::MouseButtonUVE) const override { return false; }
    [[nodiscard]] bool WasMouseButtonPressedThisFrameUVE(Input::MouseButtonUVE) const override { return false; }
    [[nodiscard]] bool WasMouseButtonReleasedThisFrameUVE(Input::MouseButtonUVE) const override { return false; }
    [[nodiscard]] Math::Vector2UVE GetMousePositionUVE() const override { return {}; }
    [[nodiscard]] Math::Vector2UVE GetMouseDeltaUVE() const override { return {}; }
    [[nodiscard]] float GetMouseScrollDeltaUVE() const override { return 0.0F; }

    void RegisterActionUVE(Input::InputActionUVE&&) override {}
    bool UnregisterActionUVE(std::string_view) override { return false; }
    bool RemapActionUVE(std::string_view, std::vector<Input::InputBindingUVE>,
                       std::vector<Input::InputBindingUVE>) override {
        return false;
    }
    [[nodiscard]] bool IsActionTriggeredUVE(std::string_view) const override { return false; }
    [[nodiscard]] bool IsActionHeldUVE(std::string_view) const override { return false; }
    [[nodiscard]] bool IsActionReleasedUVE(std::string_view) const override { return false; }
    [[nodiscard]] float GetAxisValueUVE(std::string_view) const override { return 0.0F; }

    int updateCallCount = 0;
};

class FakeAudioDeviceUVE final : public Audio::IAudioDeviceUVE {
public:
    [[nodiscard]] Audio::VoiceHandleUVE CreateVoiceUVE(const Audio::AudioVoiceDescUVE&) override {
        return Audio::VoiceHandleUVE{1};
    }
    void DestroyVoiceUVE(Audio::VoiceHandleUVE) override {}
    [[nodiscard]] bool PlayUVE(Audio::VoiceHandleUVE) override { return true; }
    [[nodiscard]] bool StopUVE(Audio::VoiceHandleUVE) override { return true; }
    [[nodiscard]] bool SetVoiceParamsUVE(Audio::VoiceHandleUVE, const Audio::AudioVoiceParamsUVE&) override {
        return true;
    }
    [[nodiscard]] Audio::VoicePlaybackStateUVE GetVoiceStateUVE(Audio::VoiceHandleUVE) const override {
        return Audio::VoicePlaybackStateUVE::Stopped;
    }
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override { return "Fake"; }
};

class FakeAudioSystemUVE final : public Audio::IAudioSystemUVE {
public:
    void SetListenerPositionUVE(Math::Vector3UVE) override {}
    void SetListenerOrientationUVE(Math::Vector3UVE, Math::Vector3UVE) override {}
    [[nodiscard]] Math::Vector3UVE GetListenerPositionUVE() const noexcept override { return {}; }
    [[nodiscard]] Audio::VoiceHandleUVE CreateSourceUVE(const Audio::AudioSourceDescUVE&) override {
        return Audio::VoiceHandleUVE{1};
    }
    void DestroySourceUVE(Audio::VoiceHandleUVE) override {}
    [[nodiscard]] bool PlayUVE(Audio::VoiceHandleUVE) override { return true; }
    [[nodiscard]] bool StopUVE(Audio::VoiceHandleUVE) override { return true; }
    [[nodiscard]] Audio::VoicePlaybackStateUVE GetSourceStateUVE(Audio::VoiceHandleUVE) const override {
        return Audio::VoicePlaybackStateUVE::Stopped;
    }
    void SetSourcePositionUVE(Audio::VoiceHandleUVE, Math::Vector3UVE) override {}
    void SetSourceVolumeUVE(Audio::VoiceHandleUVE, float) override {}
    void SetSourcePitchUVE(Audio::VoiceHandleUVE, float) override {}
    bool RegisterMixerGroupUVE(std::string_view, float, float) override { return false; }
    bool RemoveMixerGroupUVE(std::string_view) override { return false; }
    bool SetMixerGroupVolumeUVE(std::string_view, float) override { return false; }
    bool SetMixerGroupPitchUVE(std::string_view, float) override { return false; }
    bool SetSourceMixerGroupUVE(Audio::VoiceHandleUVE, std::string_view) override { return false; }
    [[nodiscard]] Audio::AudioMixerDiagnosticsUVE GetMixerDiagnosticsUVE(std::size_t) const override { return {}; }
    [[nodiscard]] bool ResetSourceStreamUVE(Audio::VoiceHandleUVE, std::size_t, bool, std::size_t,
                                             std::size_t) override {
        return false;
    }
    [[nodiscard]] bool ScheduleSourceStreamWindowUVE(Audio::VoiceHandleUVE, std::size_t) override {
        return false;
    }
    [[nodiscard]] bool PopSourceStreamWindowUVE(Audio::VoiceHandleUVE,
                                                 Audio::Pcm16StreamWindowPlanUVE&) override {
        return false;
    }
    [[nodiscard]] bool ScheduleSourcePcmGainWindowUVE(
        Audio::VoiceHandleUVE, const Audio::PcmGainEffectWindowUVE&) override {
        return false;
    }
    [[nodiscard]] bool ApplySourcePcmGainEffectsUVE(
        Audio::VoiceHandleUVE, const std::vector<float>&, std::vector<float>&) override {
        return false;
    }
    void UpdateUVE() override { ++updateCallCount; }

    int updateCallCount = 0;
};

class FakeAudioSourceSystemUVE final : public Audio::IAudioSourceSystemUVE {
public:
    void SyncUVE(Scene::IEntityManagerUVE&, Audio::IAudioSystemUVE&) override { ++syncCallCount; }

    int syncCallCount = 0;
};

class FakeSaveGameSystemUVE final : public Save::ISaveGameSystemUVE {
public:
    [[nodiscard]] bool SaveUVE(int, Scene::IEntityManagerUVE&, const std::vector<Scene::EntityUVE>&,
                                const Save::GameStateMetadataUVE&) override {
        ++saveCallCount;
        return true;
    }
    [[nodiscard]] Save::SaveMigrationRegistrationResultUVE RegisterMigrationUVE(
        std::uint32_t, std::uint32_t, Save::SavePayloadMigrationTransformUVE) override {
        return {Save::SaveMigrationRegistrationCodeUVE::Accepted, "fake migration accepted"};
    }
    [[nodiscard]] std::vector<Scene::EntityUVE> LoadUVE(int, Scene::IEntityManagerUVE&) override { return {}; }
    [[nodiscard]] bool DeleteSaveUVE(int) override { return false; }
    [[nodiscard]] bool HasSaveUVE(int) const override { return false; }
    [[nodiscard]] std::optional<Save::GameStateMetadataUVE> GetSaveMetadataUVE(int) const override {
        return std::nullopt;
    }
    [[nodiscard]] std::vector<int> ListUsedSlotsUVE() const override { return {}; }
    [[nodiscard]] Save::SaveMigrationDiagnosticsUVE GetLastMigrationDiagnosticsUVE() const override { return {}; }

    int saveCallCount = 0;
};

class FakeCheckpointManagerUVE final : public Save::ICheckpointManagerUVE {
public:
    void UpdateUVE(double, Scene::IEntityManagerUVE&, const std::vector<Scene::EntityUVE>&) override {
        ++updateCallCount;
    }
    [[nodiscard]] bool CheckpointUVE(Scene::IEntityManagerUVE&, const std::vector<Scene::EntityUVE>&) override {
        return true;
    }
    void SetAutoSaveIntervalSecondsUVE(double) noexcept override {}
    [[nodiscard]] double GetAutoSaveIntervalSecondsUVE() const noexcept override { return 0.0; }
    [[nodiscard]] double GetElapsedSinceLastSaveSecondsUVE() const noexcept override { return 0.0; }
    [[nodiscard]] double GetTotalPlaytimeSecondsUVE() const noexcept override { return 0.0; }

    int updateCallCount = 0;
};

class FakeWindowManagerUVE final : public Window::IWindowManagerUVE {
public:
    [[nodiscard]] bool IsValidUVE() const noexcept override { return true; }
    void AttachInputSystemUVE(Input::IInputSystemUVE* /*inputSystem*/) noexcept override {}
    void PollEventsUVE() override { ++pollEventsCallCount; }
    void SwapBuffersUVE() override {}
    [[nodiscard]] bool IsCloseRequestedUVE() const noexcept override { return false; }
    void SetVSyncEnabledUVE(bool) override {}
    [[nodiscard]] bool IsVSyncEnabledUVE() const noexcept override { return true; }
    void SetFullscreenUVE(bool) override {}
    [[nodiscard]] bool IsFullscreenUVE() const noexcept override { return false; }
    [[nodiscard]] std::uint32_t GetWidthUVE() const noexcept override { return 0; }
    [[nodiscard]] std::uint32_t GetHeightUVE() const noexcept override { return 0; }
    [[nodiscard]] std::vector<Window::MonitorInfoUVE> EnumerateMonitorsUVE() const override { return {}; }
    [[nodiscard]] void* GetNativeWindowHandleUVE() const noexcept override { return nullptr; }
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override { return "Fake"; }

    int pollEventsCallCount = 0;
};

TEST(EngineServicesUVETest, Accessors_ReturnExactSameInstancesPassedIn) {
    FakeLoggerUVE logger;
    FakeTimerUVE timer;
    FakeEventSystemUVE eventSystem;
    FakeMemoryManagerUVE memoryManager;
    FakeThreadPoolUVE threadPool;
    FakeCommandLineUVE commandLine;
    FakeConfigManagerUVE configManager;
    FakeEntityManagerUVE entityManager;
    FakeSceneGraphUVE sceneGraph;
    FakeAssetDatabaseUVE assetDatabase;
    FakeProjectFileIndexUVE projectFileIndex;
    FakeDerivedArtifactCacheUVE derivedArtifactCache;
    FakeProjectChangeWatcherUVE projectChangeWatcher;
    FakeSceneSerializerUVE sceneSerializer;
    FakePrefabSystemUVE prefabSystem;
    FakeParticleRuntimeUVE particleRuntime;
    FakeHotReloadUVE hotReload;
    FakeAssetManagerUVE assetManager;
    FakeAssetImporterUVE assetImporter;
    FakeAssetImportQueueUVE assetImportQueue;
    FakeAssetBundleUVE assetBundle;
    FakeFileSystemUVE fileSystem;
    FakeRenderDeviceUVE renderDevice;
    FakeShaderManagerUVE shaderManager;
    FakeRenderSystemUVE renderSystem;
    FakeCameraSystemUVE cameraSystem;
    FakeMeshRendererUVE meshRenderer;
    FakeLightSystemUVE lightSystem;
    FakeRenderer3DUVE renderer3D;
    FakeCollisionSystemUVE collisionSystem;
    FakePhysicsSystemUVE physicsSystem;
    Physics::PhysicsQuerySystemUVE physicsQuerySystem(collisionSystem);
    Physics::PhysicsConstraintSystemUVE physicsConstraintSystem;
    FakeRaycastSystemUVE raycastSystem;
    FakeInputSystemUVE inputSystem;
    Input::GamepadInputSystemUVE gamepadInputSystem;
    Input::MobileInputSystemUVE mobileInputSystem;
    Input::MobileGestureSystemUVE mobileGestureSystem(mobileInputSystem);
    FakeAudioDeviceUVE audioDevice;
    FakeAudioSystemUVE audioSystem;
    FakeAudioSourceSystemUVE audioSourceSystem;
    FakeSaveGameSystemUVE saveGameSystem;
    FakeCheckpointManagerUVE checkpointManager;
    FakeWindowManagerUVE windowManager;

    const EngineServicesUVE services(logger, timer, eventSystem, memoryManager, threadPool,
                                      commandLine, configManager, entityManager, sceneGraph,
                                      assetDatabase, projectFileIndex, derivedArtifactCache, projectChangeWatcher,
                                      sceneSerializer, prefabSystem, particleRuntime,
                                      hotReload, assetManager, assetImporter, assetImportQueue, assetBundle, fileSystem,
                                      renderDevice, shaderManager, renderSystem, cameraSystem,
                                      meshRenderer, lightSystem, renderer3D, collisionSystem, physicsSystem,
                                      physicsQuerySystem, raycastSystem, physicsConstraintSystem, inputSystem,
                                      gamepadInputSystem, mobileInputSystem, mobileGestureSystem, audioDevice, audioSystem,
                                      audioSourceSystem, saveGameSystem, checkpointManager,
                                      windowManager);

    EXPECT_EQ(&services.GetLoggerUVE(), &logger);
    EXPECT_EQ(&services.GetTimerUVE(), &timer);
    EXPECT_EQ(&services.GetEventSystemUVE(), &eventSystem);
    EXPECT_EQ(&services.GetMemoryManagerUVE(), &memoryManager);
    EXPECT_EQ(&services.GetThreadPoolUVE(), &threadPool);
    EXPECT_EQ(&services.GetCommandLineUVE(), &commandLine);
    EXPECT_EQ(&services.GetConfigManagerUVE(), &configManager);
    EXPECT_EQ(&services.GetEntityManagerUVE(), &entityManager);
    EXPECT_EQ(&services.GetSceneGraphUVE(), &sceneGraph);
    EXPECT_EQ(&services.GetAssetDatabaseUVE(), &assetDatabase);
    EXPECT_EQ(&services.GetProjectFileIndexUVE(), &projectFileIndex);
    EXPECT_EQ(&services.GetDerivedArtifactCacheUVE(), &derivedArtifactCache);
    EXPECT_EQ(&services.GetProjectChangeWatcherUVE(), &projectChangeWatcher);
    EXPECT_EQ(&services.GetSceneSerializerUVE(), &sceneSerializer);
    EXPECT_EQ(&services.GetPrefabSystemUVE(), &prefabSystem);
    EXPECT_EQ(&services.GetParticleRuntimeUVE(), &particleRuntime);
    EXPECT_EQ(&services.GetHotReloadUVE(), &hotReload);
    EXPECT_EQ(&services.GetAssetManagerUVE(), &assetManager);
    EXPECT_EQ(&services.GetAssetImporterUVE(), &assetImporter);
    EXPECT_EQ(&services.GetAssetBundleUVE(), &assetBundle);
    EXPECT_EQ(&services.GetFileSystemUVE(), &fileSystem);
    EXPECT_EQ(&services.GetRenderDeviceUVE(), &renderDevice);
    EXPECT_EQ(&services.GetShaderManagerUVE(), &shaderManager);
    EXPECT_EQ(&services.GetRenderSystemUVE(), &renderSystem);
    EXPECT_EQ(&services.GetCameraSystemUVE(), &cameraSystem);
    EXPECT_EQ(&services.GetMeshRendererUVE(), &meshRenderer);
    EXPECT_EQ(&services.GetLightSystemUVE(), &lightSystem);
    EXPECT_EQ(&services.GetRenderer3DUVE(), &renderer3D);
    EXPECT_EQ(&services.GetCollisionSystemUVE(), &collisionSystem);
    EXPECT_EQ(&services.GetPhysicsSystemUVE(), &physicsSystem);
    EXPECT_EQ(&services.GetPhysicsQuerySystemUVE(), &physicsQuerySystem);
    EXPECT_EQ(&services.GetRaycastSystemUVE(), &raycastSystem);
    EXPECT_EQ(&services.GetPhysicsConstraintSystemUVE(), &physicsConstraintSystem);
    EXPECT_EQ(&services.GetInputSystemUVE(), &inputSystem);
    EXPECT_EQ(&services.GetGamepadInputSystemUVE(), &gamepadInputSystem);
    EXPECT_EQ(&services.GetMobileInputSystemUVE(), &mobileInputSystem);
    EXPECT_EQ(&services.GetMobileGestureSystemUVE(), &mobileGestureSystem);
    EXPECT_EQ(&services.GetAudioDeviceUVE(), &audioDevice);
    EXPECT_EQ(&services.GetAudioSystemUVE(), &audioSystem);
    EXPECT_EQ(&services.GetAudioSourceSystemUVE(), &audioSourceSystem);
    EXPECT_EQ(&services.GetSaveGameSystemUVE(), &saveGameSystem);
    EXPECT_EQ(&services.GetCheckpointManagerUVE(), &checkpointManager);
    EXPECT_EQ(&services.GetWindowManagerUVE(), &windowManager);
}

TEST(EngineServicesUVETest, Accessors_ProveInterfacesAreGenuinelySubstitutable) {
    FakeLoggerUVE logger;
    FakeTimerUVE timer;
    FakeEventSystemUVE eventSystem;
    FakeMemoryManagerUVE memoryManager;
    FakeThreadPoolUVE threadPool;
    FakeCommandLineUVE commandLine;
    FakeConfigManagerUVE configManager;
    FakeEntityManagerUVE entityManager;
    FakeSceneGraphUVE sceneGraph;
    FakeAssetDatabaseUVE assetDatabase;
    FakeProjectFileIndexUVE projectFileIndex;
    FakeDerivedArtifactCacheUVE derivedArtifactCache;
    FakeProjectChangeWatcherUVE projectChangeWatcher;
    FakeSceneSerializerUVE sceneSerializer;
    FakePrefabSystemUVE prefabSystem;
    FakeParticleRuntimeUVE particleRuntime;
    FakeHotReloadUVE hotReload;
    FakeAssetManagerUVE assetManager;
    FakeAssetImporterUVE assetImporter;
    FakeAssetImportQueueUVE assetImportQueue;
    FakeAssetBundleUVE assetBundle;
    FakeFileSystemUVE fileSystem;
    FakeRenderDeviceUVE renderDevice;
    FakeShaderManagerUVE shaderManager;
    FakeRenderSystemUVE renderSystem;
    FakeCameraSystemUVE cameraSystem;
    FakeMeshRendererUVE meshRenderer;
    FakeLightSystemUVE lightSystem;
    FakeRenderer3DUVE renderer3D;
    FakeCollisionSystemUVE collisionSystem;
    FakePhysicsSystemUVE physicsSystem;
    Physics::PhysicsQuerySystemUVE physicsQuerySystem(collisionSystem);
    Physics::PhysicsConstraintSystemUVE physicsConstraintSystem;
    FakeRaycastSystemUVE raycastSystem;
    FakeInputSystemUVE inputSystem;
    Input::GamepadInputSystemUVE gamepadInputSystem;
    Input::MobileInputSystemUVE mobileInputSystem;
    Input::MobileGestureSystemUVE mobileGestureSystem(mobileInputSystem);
    FakeAudioDeviceUVE audioDevice;
    FakeAudioSystemUVE audioSystem;
    FakeAudioSourceSystemUVE audioSourceSystem;
    FakeSaveGameSystemUVE saveGameSystem;
    FakeCheckpointManagerUVE checkpointManager;
    FakeWindowManagerUVE windowManager;
    const EngineServicesUVE services(logger, timer, eventSystem, memoryManager, threadPool,
                                      commandLine, configManager, entityManager, sceneGraph,
                                      assetDatabase, projectFileIndex, derivedArtifactCache, projectChangeWatcher,
                                      sceneSerializer, prefabSystem, particleRuntime,
                                      hotReload, assetManager, assetImporter, assetImportQueue, assetBundle, fileSystem,
                                      renderDevice, shaderManager, renderSystem, cameraSystem,
                                      meshRenderer, lightSystem, renderer3D, collisionSystem, physicsSystem,
                                      physicsQuerySystem, raycastSystem, physicsConstraintSystem, inputSystem,
                                      gamepadInputSystem, mobileInputSystem, mobileGestureSystem, audioDevice, audioSystem,
                                      audioSourceSystem, saveGameSystem, checkpointManager,
                                      windowManager);

    services.GetTimerUVE().Tick();
    services.GetEventSystemUVE().DispatchQueuedUVE();
    services.GetMemoryManagerUVE().LogLeakReportUVE();
    services.GetThreadPoolUVE().SubmitUVE([] {});
    static_cast<void>(services.GetCommandLineUVE().HasFlagUVE("server"));
    services.GetConfigManagerUVE().SaveUVE();
    static_cast<void>(services.GetEntityManagerUVE().CreateEntityUVE());
    services.GetSceneGraphUVE().UpdateUVE(services.GetEntityManagerUVE());
    EXPECT_TRUE(services.GetParticleRuntimeUVE().AttachUVE(Scene::EntityUVE{1, 1}, Scene::ParticleEmitterComponentUVE{}));
    services.GetAssetDatabaseUVE().SaveUVE();
    static_cast<void>(services.GetProjectFileIndexUVE().RefreshUVE(services.GetAssetDatabaseUVE()));
    static_cast<void>(services.GetDerivedArtifactCacheUVE().LoadImportRecordUVE("unused.asset"));
    EXPECT_TRUE(services.GetProjectChangeWatcherUVE().PollNowUVE(services.GetAssetDatabaseUVE(),
                                                                   services.GetDerivedArtifactCacheUVE()));
    static_cast<void>(services.GetAssetImportQueueUVE().TickUVE());
    static_cast<void>(services.GetSceneSerializerUVE().SaveUVE(
        services.GetEntityManagerUVE(), {}, "unused.uvescene", Scene::SceneAssetTypeUVE::Scene));
    static_cast<void>(services.GetPrefabSystemUVE().SavePrefabUVE(
        services.GetEntityManagerUVE(), services.GetAssetDatabaseUVE(), Scene::kInvalidEntityUVE,
        "unused.uveprefab"));
    services.GetHotReloadUVE().PollUVE(services.GetAssetManagerUVE(), services.GetAssetDatabaseUVE(), 0.0);
    services.GetAssetManagerUVE().CollectGarbageUVE();
    static_cast<void>(
        services.GetAssetImporterUVE().ImportUVE("unused_source", "unused_dest", services.GetAssetDatabaseUVE()));
    static_cast<void>(services.GetAssetBundleUVE().PackUVE({}, "unused.uvebundle"));
    static_cast<void>(services.GetFileSystemUVE().WriteFileUVE("unused.txt", {}));
    static_cast<void>(services.GetRenderDeviceUVE().CreateBufferUVE(Render::BufferDescUVE{}));
    services.GetShaderManagerUVE().UpdateUVE(0.0);
    services.GetRenderSystemUVE().BeginFrameUVE();
    static_cast<void>(
        services.GetCameraSystemUVE().ComputeViewMatrixUVE(services.GetEntityManagerUVE(), Scene::kInvalidEntityUVE));
    static_cast<void>(
        services.GetCameraSystemUVE().GetWorldPositionUVE(services.GetEntityManagerUVE(), Scene::kInvalidEntityUVE));
    static_cast<void>(services.GetMeshRendererUVE().ExtractRenderQueueUVE(
        services.GetEntityManagerUVE(), services.GetAssetManagerUVE(), services.GetAssetDatabaseUVE(),
        Math::FrustumUVE{}));
    static_cast<void>(services.GetLightSystemUVE().ExtractActiveLightsUVE(services.GetEntityManagerUVE()));
    services.GetRenderer3DUVE().RenderFrameUVE(services.GetEntityManagerUVE(), Scene::kInvalidEntityUVE);
    renderer3D.diagnostics.primitiveItemsExtracted = 7U;
    EXPECT_EQ(services.GetRenderer3DUVE().GetLastFrameDiagnosticsUVE().primitiveItemsExtracted, 7U);
    static_cast<void>(services.GetRaycastSystemUVE().RaycastUVE(services.GetEntityManagerUVE(), Physics::RaycastQueryUVE{}));
    services.GetGamepadInputSystemUVE().UpdateUVE();
    services.GetMobileInputSystemUVE().UpdateUVE();
    services.GetMobileGestureSystemUVE().UpdateUVE(0.0F);
    services.GetInputSystemUVE().UpdateUVE();
    services.GetAudioSystemUVE().UpdateUVE();
    services.GetAudioSourceSystemUVE().SyncUVE(services.GetEntityManagerUVE(), services.GetAudioSystemUVE());
    static_cast<void>(services.GetSaveGameSystemUVE().SaveUVE(0, services.GetEntityManagerUVE(), {},
                                                                Save::GameStateMetadataUVE{}));
    services.GetCheckpointManagerUVE().UpdateUVE(0.0, services.GetEntityManagerUVE(), {});
    services.GetWindowManagerUVE().PollEventsUVE();

    EXPECT_EQ(timer.tickCount, 1);
    EXPECT_EQ(eventSystem.dispatchCount, 1);
    EXPECT_EQ(memoryManager.logLeakReportCallCount, 1);
    EXPECT_EQ(threadPool.submitCount, 1);
    EXPECT_EQ(commandLine.hasFlagCallCount, 1);
    EXPECT_EQ(configManager.saveCallCount, 1);
    EXPECT_EQ(entityManager.createEntityCallCount, 1);
    EXPECT_EQ(sceneGraph.updateCallCount, 1);
    EXPECT_EQ(particleRuntime.attachCallCount, 1);
    EXPECT_EQ(assetDatabase.saveCallCount, 1);
    EXPECT_EQ(projectFileIndex.refreshCallCount, 1);
    EXPECT_EQ(derivedArtifactCache.loadCallCount, 1);
    EXPECT_EQ(projectChangeWatcher.pollNowCallCount, 1);
    EXPECT_EQ(assetImportQueue.tickCallCount, 1);
    EXPECT_EQ(sceneSerializer.saveCallCount, 1);
    EXPECT_EQ(prefabSystem.savePrefabCallCount, 1);
    EXPECT_EQ(hotReload.pollCallCount, 1);
    EXPECT_EQ(assetManager.collectGarbageCallCount, 1);
    EXPECT_EQ(assetImporter.importCallCount, 1);
    EXPECT_EQ(assetBundle.packCallCount, 1);
    EXPECT_EQ(fileSystem.writeCallCount, 1);
    EXPECT_EQ(renderDevice.createBufferCallCount, 1);
    EXPECT_EQ(shaderManager.updateCallCount, 1);
    EXPECT_EQ(renderSystem.beginFrameCallCount, 1);
    EXPECT_EQ(cameraSystem.computeViewCallCount, 1);
    EXPECT_EQ(cameraSystem.getWorldPositionCallCount, 1);
    EXPECT_EQ(meshRenderer.extractRenderQueueCallCount, 1);
    EXPECT_EQ(lightSystem.extractActiveLightCallCount, 1);
    EXPECT_EQ(renderer3D.renderFrameCallCount, 1);
    EXPECT_EQ(renderer3D.getDiagnosticsCallCount, 1);
    EXPECT_EQ(raycastSystem.raycastCallCount, 1);
    EXPECT_EQ(inputSystem.updateCallCount, 1);
    EXPECT_EQ(audioSystem.updateCallCount, 1);
    EXPECT_EQ(audioSourceSystem.syncCallCount, 1);
    EXPECT_EQ(saveGameSystem.saveCallCount, 1);
    EXPECT_EQ(checkpointManager.updateCallCount, 1);
    EXPECT_EQ(windowManager.pollEventsCallCount, 1);
}

} // namespace
} // namespace UVE::Core::Tests
