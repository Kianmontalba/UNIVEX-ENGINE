//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/memory/heap_allocator_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Memory::Tests {
namespace {

class FakeTrackerUVE final : public IMemoryTrackerUVE {
public:
    void RecordAllocationUVE(void* pointer, std::size_t sizeBytes, std::size_t alignment,
                              std::string_view allocatorTag, const char* sourceFile,
                              int sourceLine) override {
        allocations.push_back(AllocationRecordUVE{pointer, sizeBytes, alignment,
                                                    std::string(allocatorTag), sourceFile,
                                                    sourceLine, 0});
    }
    void RecordDeallocationUVE(void* pointer) override { deallocatedPointers.push_back(pointer); }

    std::vector<AllocationRecordUVE> allocations;
    std::vector<void*> deallocatedPointers;
};

TEST(HeapAllocatorUVETest, AllocateDeallocate_RoundTrip) {
    HeapAllocatorUVE allocator;
    void* const pointer = allocator.AllocateUVE(64, 8, __FILE__, __LINE__);
    ASSERT_NE(pointer, nullptr);
    allocator.DeallocateUVE(pointer);
}

TEST(HeapAllocatorUVETest, AllocateUVE_ReturnsAlignedPointer) {
    HeapAllocatorUVE allocator;
    for (const std::size_t alignment : {std::size_t{8}, std::size_t{16}, std::size_t{32}, std::size_t{64}}) {
        void* const pointer = allocator.AllocateUVE(16, alignment, __FILE__, __LINE__);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(pointer) % alignment, 0U);
        allocator.DeallocateUVE(pointer);
    }
}

TEST(HeapAllocatorUVETest, GetAllocatedBytesUVE_TracksOutstandingBytes) {
    HeapAllocatorUVE allocator;
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 0U);

    void* const first = allocator.AllocateUVE(100, 8, __FILE__, __LINE__);
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 100U);

    void* const second = allocator.AllocateUVE(50, 8, __FILE__, __LINE__);
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 150U);

    allocator.DeallocateUVE(first);
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 50U);

    allocator.DeallocateUVE(second);
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 0U);
}

TEST(HeapAllocatorUVETest, Tracker_ReceivesMatchingAllocationAndDeallocationCalls) {
    FakeTrackerUVE tracker;
    HeapAllocatorUVE allocator(&tracker, "TestHeap");

    void* const pointer = allocator.AllocateUVE(32, 16, __FILE__, __LINE__);
    ASSERT_EQ(tracker.allocations.size(), 1U);
    EXPECT_EQ(tracker.allocations[0].pointer, pointer);
    EXPECT_EQ(tracker.allocations[0].sizeBytes, 32U);
    EXPECT_EQ(tracker.allocations[0].alignment, 16U);
    EXPECT_EQ(tracker.allocations[0].allocatorTag, "TestHeap");

    allocator.DeallocateUVE(pointer);
    ASSERT_EQ(tracker.deallocatedPointers.size(), 1U);
    EXPECT_EQ(tracker.deallocatedPointers[0], pointer);
}

} // namespace
} // namespace UVE::Memory::Tests
