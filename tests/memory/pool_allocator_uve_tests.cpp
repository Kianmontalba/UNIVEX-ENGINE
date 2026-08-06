// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/pool_allocator_uve.h"

#include <cstdint>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "uve/platform/platform_uve.h"

namespace UVE::Memory::Tests {
namespace {

TEST(PoolAllocatorUVETest, AllocateUpToCapacity_Succeeds) {
    PoolAllocatorUVE pool(32, 8, 4);
    std::vector<void*> pointers;
    for (int i = 0; i < 4; ++i) {
        void* const pointer = pool.AllocateUVE(32, 8, __FILE__, __LINE__);
        ASSERT_NE(pointer, nullptr);
        pointers.push_back(pointer);
    }
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 4U);
    EXPECT_EQ(pool.GetFreeBlocksUVE(), 0U);
}

#if UVE_DEBUG
TEST(PoolAllocatorUVEDeathTest, AllocateBeyondCapacity_Asserts) {
    PoolAllocatorUVE pool(32, 8, 1);
    (void)pool.AllocateUVE(32, 8, __FILE__, __LINE__);
    EXPECT_DEATH({ (void)pool.AllocateUVE(32, 8, __FILE__, __LINE__); }, "");
}

TEST(PoolAllocatorUVEDeathTest, ConstructWithInvalidAlignment_Asserts) {
    EXPECT_DEATH({ PoolAllocatorUVE pool(32, 3, 4); }, "");
}

TEST(PoolAllocatorUVEDeathTest, DeallocateForeignPointer_Asserts) {
    PoolAllocatorUVE pool(32, 8, 2);
    int foreign = 0;
    EXPECT_DEATH({ pool.DeallocateUVE(&foreign); }, "");
}
#endif

TEST(PoolAllocatorUVETest, DeallocateThenReallocate_ReusesFreedBlock) {
    PoolAllocatorUVE pool(32, 8, 2);
    void* const first = pool.AllocateUVE(32, 8, __FILE__, __LINE__);
    pool.DeallocateUVE(first);
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 0U);

    void* const second = pool.AllocateUVE(32, 8, __FILE__, __LINE__);
    EXPECT_EQ(second, first);
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 1U);
}

TEST(PoolAllocatorUVETest, AllocatedPointers_AreWithinRangeAndBlockAligned) {
    PoolAllocatorUVE pool(24, 16, 5);
    std::unordered_set<void*> seen;
    for (int i = 0; i < 5; ++i) {
        void* const pointer = pool.AllocateUVE(24, 16, __FILE__, __LINE__);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(pointer) % 16, 0U);
        EXPECT_TRUE(seen.insert(pointer).second) << "pool returned the same block twice";
    }
}

TEST(PoolAllocatorUVETest, BlockStats_UpdateAcrossAllocateAndDeallocate) {
    PoolAllocatorUVE pool(16, 8, 8);
    EXPECT_EQ(pool.GetCapacityBlocksUVE(), 8U);
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 0U);
    EXPECT_EQ(pool.GetFreeBlocksUVE(), 8U);

    std::vector<void*> pointers;
    for (int i = 0; i < 3; ++i) {
        pointers.push_back(pool.AllocateUVE(16, 8, __FILE__, __LINE__));
    }
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 3U);
    EXPECT_EQ(pool.GetFreeBlocksUVE(), 5U);

    pool.DeallocateUVE(pointers.front());
    EXPECT_EQ(pool.GetUsedBlocksUVE(), 2U);
    EXPECT_EQ(pool.GetFreeBlocksUVE(), 6U);
}

} // namespace
} // namespace UVE::Memory::Tests
