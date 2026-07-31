//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_core_uve.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Core::Tests {
namespace {

EngineConfigUVE MakeTestConfigUVE() {
    EngineConfigUVE config{};
    config.enableConsoleLogging = false;
    config.logFilePath = "uve_engine_core_tests.log";
    config.threadPoolWorkerCount = 2; // keep the whole suite's thread churn small and fast
    config.settingsFilePath = "uve_engine_core_tests.uvesettings"; // never touch a real settings file
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

#if UVE_DEBUG
TEST(EngineCoreUVEDeathTest, ShutdownBeforeInit_TriggersInvalidTransitionAssert) {
    EngineCoreUVE engine(MakeTestConfigUVE());
    EXPECT_DEATH({ engine.Shutdown(); }, "");
}
#endif

} // namespace
} // namespace UVE::Core::Tests
