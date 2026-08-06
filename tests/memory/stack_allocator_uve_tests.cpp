// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/stack_allocator_uve.h"

#include <cstddef>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "uve/platform/platform_uve.h"

namespace UVE::Memory::Tests {
namespace {

class FakeTrackerUVE final : public IMemoryTrackerUVE {
public:
    void RecordAllocationUVE(void*, std::size_t, std::size_t, std::string_view, const char*,
                              int) override {
        ++allocationCount;
    }
    void RecordDeallocationUVE(void* pointer) override { deallocatedPointers.push_back(pointer); }

    int allocationCount = 0;
    std::vector<void*> deallocatedPointers;
};

TEST(StackAllocatorUVETest, SequentialAllocations_BumpOffsetForward) {
    StackAllocatorUVE stack(1024);
    void* const first = stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    void* const second = stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    EXPECT_NE(first, second);
    EXPECT_GT(stack.GetAllocatedBytesUVE(), 0U);
}

#if UVE_DEBUG
TEST(StackAllocatorUVEDeathTest, ExhaustingCapacity_Asserts) {
    StackAllocatorUVE stack(16);
    EXPECT_DEATH({ (void)stack.AllocateUVE(1024, 8, __FILE__, __LINE__); }, "");
}

TEST(StackAllocatorUVEDeathTest, OutOfOrderDeallocate_Asserts) {
    StackAllocatorUVE stack(256);
    void* const first = stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__); // second allocation, now on top
    EXPECT_DEATH({ stack.DeallocateUVE(first); }, "");
}

TEST(StackAllocatorUVEDeathTest, MarkerFromDifferentAllocator_Asserts) {
    StackAllocatorUVE stackA(256);
    StackAllocatorUVE stackB(256);
    const StackMarkerUVE markerFromB = stackB.GetMarkerUVE();
    EXPECT_DEATH({ stackA.RewindToMarkerUVE(markerFromB); }, "");
}

TEST(StackAllocatorUVEDeathTest, StaleMarkerBeyondCurrentOffset_Asserts) {
    StackAllocatorUVE stack(256);
    const StackMarkerUVE earlyMarker = stack.GetMarkerUVE();
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    const StackMarkerUVE lateMarker = stack.GetMarkerUVE();

    stack.RewindToMarkerUVE(earlyMarker); // current offset now back before lateMarker's position
    EXPECT_DEATH({ stack.RewindToMarkerUVE(lateMarker); }, "");
}
#endif

TEST(StackAllocatorUVETest, MarkerRewind_FreesAndSpaceIsReusable) {
    StackAllocatorUVE stack(1024);
    const StackMarkerUVE marker = stack.GetMarkerUVE();
    (void)stack.AllocateUVE(64, 8, __FILE__, __LINE__);
    (void)stack.AllocateUVE(64, 8, __FILE__, __LINE__);
    EXPECT_GT(stack.GetAllocatedBytesUVE(), 0U);

    stack.RewindToMarkerUVE(marker);
    EXPECT_EQ(stack.GetAllocatedBytesUVE(), 0U);

    void* const reused = stack.AllocateUVE(64, 8, __FILE__, __LINE__);
    ASSERT_NE(reused, nullptr);
}

TEST(StackAllocatorUVETest, DeallocateUVE_StrictLifo_Succeeds) {
    StackAllocatorUVE stack(256);
    void* const first = stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    void* const second = stack.AllocateUVE(16, 8, __FILE__, __LINE__);

    stack.DeallocateUVE(second);
    stack.DeallocateUVE(first);
    EXPECT_EQ(stack.GetAllocatedBytesUVE(), 0U);
}

TEST(StackAllocatorUVETest, RewindToMarkerUVE_ReportsOneDeallocationPerFreedAllocation) {
    FakeTrackerUVE tracker;
    StackAllocatorUVE stack(1024, &tracker);
    const StackMarkerUVE marker = stack.GetMarkerUVE();

    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    EXPECT_EQ(tracker.allocationCount, 3);

    stack.RewindToMarkerUVE(marker);
    EXPECT_EQ(tracker.deallocatedPointers.size(), 3U);
}

TEST(StackAllocatorUVETest, Reset_FreesEverything) {
    StackAllocatorUVE stack(256);
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    (void)stack.AllocateUVE(16, 8, __FILE__, __LINE__);
    stack.Reset();
    EXPECT_EQ(stack.GetAllocatedBytesUVE(), 0U);
}

} // namespace
} // namespace UVE::Memory::Tests
