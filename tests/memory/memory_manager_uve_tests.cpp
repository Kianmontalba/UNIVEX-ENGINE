// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/memory_manager_uve.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Memory::Tests {
namespace {

TEST(MemoryManagerUVETest, RecordAllocationAndDeallocation_UpdateActiveStats) {
    MemoryManagerUVE manager;
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 0U);
    EXPECT_EQ(manager.GetActiveBytesUVE(), 0U);

    int dummyA = 0;
    int dummyB = 0;
    manager.RecordAllocationUVE(&dummyA, 100, 8, "Test", __FILE__, __LINE__);
    manager.RecordAllocationUVE(&dummyB, 50, 8, "Test", __FILE__, __LINE__);
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 2U);
    EXPECT_EQ(manager.GetActiveBytesUVE(), 150U);

    manager.RecordDeallocationUVE(&dummyA);
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 1U);
    EXPECT_EQ(manager.GetActiveBytesUVE(), 50U);

    manager.RecordDeallocationUVE(&dummyB);
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 0U);
}

TEST(MemoryManagerUVETest, PeakBytes_TracksHighWaterMarkAndNeverDecreases) {
    MemoryManagerUVE manager;
    int dummyA = 0;
    int dummyB = 0;
    manager.RecordAllocationUVE(&dummyA, 100, 8, "Test", __FILE__, __LINE__);
    manager.RecordAllocationUVE(&dummyB, 100, 8, "Test", __FILE__, __LINE__);
    EXPECT_EQ(manager.GetPeakBytesUVE(), 200U);

    manager.RecordDeallocationUVE(&dummyA);
    EXPECT_EQ(manager.GetPeakBytesUVE(), 200U);
    EXPECT_EQ(manager.GetActiveBytesUVE(), 100U);

    manager.RecordDeallocationUVE(&dummyB);
}

TEST(MemoryManagerUVETest, AllocationId_IsUniqueAndMonotonicallyIncreasing) {
    MemoryManagerUVE manager;
    int dummyA = 0;
    int dummyB = 0;
    manager.RecordAllocationUVE(&dummyA, 8, 8, "Test", __FILE__, __LINE__);
    manager.RecordAllocationUVE(&dummyB, 8, 8, "Test", __FILE__, __LINE__);

    const std::vector<AllocationRecordUVE> leaks = manager.GetLeakedAllocationsUVE();
    ASSERT_EQ(leaks.size(), 2U);

    const AllocationRecordUVE* recordA = nullptr;
    const AllocationRecordUVE* recordB = nullptr;
    for (const AllocationRecordUVE& record : leaks) {
        if (record.pointer == &dummyA) {
            recordA = &record;
        } else if (record.pointer == &dummyB) {
            recordB = &record;
        }
    }
    ASSERT_NE(recordA, nullptr);
    ASSERT_NE(recordB, nullptr);
    EXPECT_GT(recordA->allocationId, 0U);
    EXPECT_EQ(recordB->allocationId, recordA->allocationId + 1);

    manager.RecordDeallocationUVE(&dummyA);
    manager.RecordDeallocationUVE(&dummyB);
}

TEST(MemoryManagerUVETest, GetDefaultAllocatorUVE_ReturnsWorkingSelfTrackedAllocator) {
    MemoryManagerUVE manager;
    IAllocatorUVE& allocator = manager.GetDefaultAllocatorUVE();

    void* const pointer = allocator.AllocateUVE(64, 8, __FILE__, __LINE__);
    ASSERT_NE(pointer, nullptr);
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 1U);
    EXPECT_EQ(manager.GetActiveBytesUVE(), 64U);

    allocator.DeallocateUVE(pointer);
    EXPECT_EQ(manager.GetActiveAllocationCountUVE(), 0U);
}

TEST(MemoryManagerUVETest, HasLeaksAndGetLeakedAllocations_FlagIntentionallyUnfreedAllocation) {
    MemoryManagerUVE manager;
    EXPECT_FALSE(manager.HasLeaksUVE());

    int leaked = 0;
    manager.RecordAllocationUVE(&leaked, 42, 8, "LeakyAllocator", __FILE__, 12345);

    EXPECT_TRUE(manager.HasLeaksUVE());
    const std::vector<AllocationRecordUVE> leaks = manager.GetLeakedAllocationsUVE();
    ASSERT_EQ(leaks.size(), 1U);
    EXPECT_EQ(leaks[0].pointer, &leaked);
    EXPECT_EQ(leaks[0].sizeBytes, 42U);
    EXPECT_EQ(leaks[0].allocatorTag, "LeakyAllocator");
    EXPECT_EQ(leaks[0].sourceLine, 12345);
    EXPECT_GT(leaks[0].allocationId, 0U);

    manager.RecordDeallocationUVE(&leaked); // clean up so the test doesn't itself "leak"
}

TEST(MemoryManagerUVETest, LogLeakReportUVE_EmitsErrorPerLeak) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    MemoryManagerUVE manager;
    int leaked = 0;
    manager.RecordAllocationUVE(&leaked, 16, 8, "LeakyAllocator", __FILE__, __LINE__);

    manager.LogLeakReportUVE();

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundLeakError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("Memory leak") != std::string::npos;
        });
    EXPECT_TRUE(foundLeakError);

    manager.RecordDeallocationUVE(&leaked);
    logger.Shutdown();
}

} // namespace
} // namespace UVE::Memory::Tests
