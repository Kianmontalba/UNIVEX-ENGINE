// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/alignment_utils_uve.h"

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "uve/memory/heap_allocator_uve.h"
#include "uve/memory/pool_allocator_uve.h"
#include "uve/memory/stack_allocator_uve.h"

namespace UVE::Memory::Tests {
namespace {

class AlignmentUVETest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(AlignmentUVETest, HeapAllocatorUVE_ReturnsCorrectlyAlignedPointer) {
    const std::size_t alignment = GetParam();
    HeapAllocatorUVE allocator;
    void* const pointer = allocator.AllocateUVE(64, alignment, __FILE__, __LINE__);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(pointer) % alignment, 0U);
    allocator.DeallocateUVE(pointer);
}

TEST_P(AlignmentUVETest, PoolAllocatorUVE_ReturnsCorrectlyAlignedPointer) {
    const std::size_t alignment = GetParam();
    PoolAllocatorUVE pool(64, alignment, 4);
    void* const pointer = pool.AllocateUVE(64, alignment, __FILE__, __LINE__);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(pointer) % alignment, 0U);
    pool.DeallocateUVE(pointer);
}

TEST_P(AlignmentUVETest, StackAllocatorUVE_ReturnsCorrectlyAlignedPointer) {
    const std::size_t alignment = GetParam();
    StackAllocatorUVE stack(1024);
    void* const pointer = stack.AllocateUVE(64, alignment, __FILE__, __LINE__);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(pointer) % alignment, 0U);
}

INSTANTIATE_TEST_SUITE_P(VariousAlignments, AlignmentUVETest,
                          ::testing::Values<std::size_t>(8, 16, 32, 64, 128));

TEST(IsValidAlignmentUVETest, PowersOfTwo_AreValid) {
    EXPECT_TRUE(IsValidAlignmentUVE(1));
    EXPECT_TRUE(IsValidAlignmentUVE(2));
    EXPECT_TRUE(IsValidAlignmentUVE(8));
    EXPECT_TRUE(IsValidAlignmentUVE(128));
}

TEST(IsValidAlignmentUVETest, ZeroAndNonPowersOfTwo_AreInvalid) {
    EXPECT_FALSE(IsValidAlignmentUVE(0));
    EXPECT_FALSE(IsValidAlignmentUVE(3));
    EXPECT_FALSE(IsValidAlignmentUVE(6));
    EXPECT_FALSE(IsValidAlignmentUVE(100));
}

} // namespace
} // namespace UVE::Memory::Tests
