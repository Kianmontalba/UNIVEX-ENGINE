// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "chunk_uve.h"

#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "uve/memory/heap_allocator_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/scene/component_type_info_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene::Detail::Tests {
namespace {

struct TestComponentUVE {
    int value = 0;
};

struct UnregisteredComponentUVE {
    int value = 0;
};

class ChunkUVETest : public ::testing::Test {
protected:
    Memory::HeapAllocatorUVE allocator;
    std::unordered_map<std::type_index, ComponentTypeInfoUVE> typeInfos{
        {std::type_index(typeid(TestComponentUVE)), MakeComponentTypeInfoUVE<TestComponentUVE>()}};
    ChunkUVE chunk{allocator, {std::type_index(typeid(TestComponentUVE))}, typeInfos};
};

TEST_F(ChunkUVETest, ReserveRowUVE_ReturnsSequentialRowsUntilFull) {
    for (std::size_t expectedRow = 0; expectedRow < kChunkCapacityUVE; ++expectedRow) {
        EXPECT_EQ(chunk.ReserveRowUVE(EntityUVE{}), expectedRow);
    }
    EXPECT_TRUE(chunk.IsFullUVE());
}

#if UVE_DEBUG
TEST_F(ChunkUVETest, GetComponentPointerUVE_UnregisteredComponentTypeAsserts) {
    static_cast<void>(chunk.ReserveRowUVE(EntityUVE{}));
    EXPECT_DEATH(
        { static_cast<void>(chunk.GetComponentPointerUVE(std::type_index(typeid(UnregisteredComponentUVE)), 0)); },
        "");
}
#else
TEST_F(ChunkUVETest, GetComponentPointerUVE_UnregisteredComponentTypeThrowsOutOfRange) {
    static_cast<void>(chunk.ReserveRowUVE(EntityUVE{}));
    EXPECT_THROW(
        { static_cast<void>(chunk.GetComponentPointerUVE(std::type_index(typeid(UnregisteredComponentUVE)), 0)); },
        std::out_of_range);
}
#endif

#if UVE_DEBUG
TEST_F(ChunkUVETest, ReserveRowUVE_OnFullChunkAsserts) {
    for (std::size_t i = 0; i < kChunkCapacityUVE; ++i) {
        static_cast<void>(chunk.ReserveRowUVE(EntityUVE{}));
    }
    EXPECT_DEATH({ static_cast<void>(chunk.ReserveRowUVE(EntityUVE{})); }, "");
}

TEST_F(ChunkUVETest, VacateRowUVE_OutOfRangeRowAsserts) {
    EXPECT_DEATH({ static_cast<void>(chunk.VacateRowUVE(0)); }, "");
}

TEST_F(ChunkUVETest, GetEntityAtRowUVE_OutOfRangeRowAsserts) {
    EXPECT_DEATH({ static_cast<void>(chunk.GetEntityAtRowUVE(0)); }, "");
}
#else
TEST_F(ChunkUVETest, ReserveRowUVE_OnFullChunkReturnsSentinelInsteadOfOverrunningStorage) {
    for (std::size_t i = 0; i < kChunkCapacityUVE; ++i) {
        static_cast<void>(chunk.ReserveRowUVE(EntityUVE{}));
    }
    EXPECT_EQ(chunk.ReserveRowUVE(EntityUVE{}), kInvalidRowUVE);
    EXPECT_TRUE(chunk.IsFullUVE()); // the failed reservation must not have grown m_count further
}

TEST_F(ChunkUVETest, VacateRowUVE_OutOfRangeRowReturnsInvalidEntityInsteadOfUnderflowing) {
    EXPECT_EQ(chunk.VacateRowUVE(0), kInvalidEntityUVE);
    EXPECT_EQ(chunk.GetCountUVE(), 0U);
}

TEST_F(ChunkUVETest, GetEntityAtRowUVE_OutOfRangeRowReturnsInvalidEntity) {
    EXPECT_EQ(chunk.GetEntityAtRowUVE(0), kInvalidEntityUVE);
}
#endif

} // namespace
} // namespace UVE::Scene::Detail::Tests
