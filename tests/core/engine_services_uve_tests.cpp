//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_services_uve.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include <gtest/gtest.h>

#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"

// These hand-written fakes exist to prove that EngineServicesUVE (and, by
// extension, anything that consumes ILoggerUVE/ITimerUVE/IEventSystemUVE/
// IMemoryManagerUVE/IThreadPoolUVE/ICommandLineUVE/IConfigManagerUVE/
// IEntityManagerUVE/ISceneGraphUVE) works against ANY conforming
// implementation, independent of the concrete LoggerUVE/TimerUVE/
// EventSystemUVE/MemoryManagerUVE/ThreadPoolUVE/CommandLineUVE/
// ConfigManagerUVE/EntityManagerUVE/SceneGraphUVE classes used by
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

    int tickCount = 0;
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

    const EngineServicesUVE services(logger, timer, eventSystem, memoryManager, threadPool,
                                      commandLine, configManager, entityManager, sceneGraph);

    EXPECT_EQ(&services.GetLoggerUVE(), &logger);
    EXPECT_EQ(&services.GetTimerUVE(), &timer);
    EXPECT_EQ(&services.GetEventSystemUVE(), &eventSystem);
    EXPECT_EQ(&services.GetMemoryManagerUVE(), &memoryManager);
    EXPECT_EQ(&services.GetThreadPoolUVE(), &threadPool);
    EXPECT_EQ(&services.GetCommandLineUVE(), &commandLine);
    EXPECT_EQ(&services.GetConfigManagerUVE(), &configManager);
    EXPECT_EQ(&services.GetEntityManagerUVE(), &entityManager);
    EXPECT_EQ(&services.GetSceneGraphUVE(), &sceneGraph);
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
    const EngineServicesUVE services(logger, timer, eventSystem, memoryManager, threadPool,
                                      commandLine, configManager, entityManager, sceneGraph);

    services.GetTimerUVE().Tick();
    services.GetEventSystemUVE().DispatchQueuedUVE();
    services.GetMemoryManagerUVE().LogLeakReportUVE();
    services.GetThreadPoolUVE().SubmitUVE([] {});
    static_cast<void>(services.GetCommandLineUVE().HasFlagUVE("server"));
    services.GetConfigManagerUVE().SaveUVE();
    static_cast<void>(services.GetEntityManagerUVE().CreateEntityUVE());
    services.GetSceneGraphUVE().UpdateUVE(services.GetEntityManagerUVE());

    EXPECT_EQ(timer.tickCount, 1);
    EXPECT_EQ(eventSystem.dispatchCount, 1);
    EXPECT_EQ(memoryManager.logLeakReportCallCount, 1);
    EXPECT_EQ(threadPool.submitCount, 1);
    EXPECT_EQ(commandLine.hasFlagCallCount, 1);
    EXPECT_EQ(configManager.saveCallCount, 1);
    EXPECT_EQ(entityManager.createEntityCallCount, 1);
    EXPECT_EQ(sceneGraph.updateCallCount, 1);
}

} // namespace
} // namespace UVE::Core::Tests
