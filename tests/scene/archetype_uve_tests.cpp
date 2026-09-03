// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "archetype_uve.h"

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

class ArchetypeUVETest : public ::testing::Test {
protected:
    Memory::HeapAllocatorUVE allocator;
    std::unordered_map<std::type_index, ComponentTypeInfoUVE> typeInfos{
        {std::type_index(typeid(TestComponentUVE)), MakeComponentTypeInfoUVE<TestComponentUVE>()}};
    ArchetypeSignatureUVE signature{std::vector<std::type_index>{std::type_index(typeid(TestComponentUVE))}};
    ArchetypeUVE archetype{signature, allocator, typeInfos};
};

TEST_F(ArchetypeUVETest, ReserveEntityUVE_GrowsCountAndReturnsLocations) {
    const ArchetypeUVE::LocationUVE location = archetype.ReserveEntityUVE(EntityUVE{});
    EXPECT_EQ(location.chunkIndex, 0U);
    EXPECT_EQ(location.row, 0U);
    EXPECT_EQ(archetype.GetEntityCountUVE(), 1U);
}

#if UVE_DEBUG
TEST_F(ArchetypeUVETest, DestroyEntityAtUVE_OutOfRangeChunkIndexAsserts) {
    EXPECT_DEATH({ static_cast<void>(archetype.DestroyEntityAtUVE(ArchetypeUVE::LocationUVE{5, 0})); }, "");
}

TEST_F(ArchetypeUVETest, VacateWithoutDestroyUVE_OutOfRangeChunkIndexAsserts) {
    EXPECT_DEATH({ static_cast<void>(archetype.VacateWithoutDestroyUVE(ArchetypeUVE::LocationUVE{5, 0})); }, "");
}

TEST_F(ArchetypeUVETest, GetChunkUVE_OutOfRangeChunkIndexAsserts) {
    EXPECT_DEATH({ static_cast<void>(archetype.GetChunkUVE(5)); }, "");
}
#else
TEST_F(ArchetypeUVETest, DestroyEntityAtUVE_OutOfRangeChunkIndexReturnsInvalidEntityInsteadOfIndexingPastChunks) {
    EXPECT_EQ(archetype.DestroyEntityAtUVE(ArchetypeUVE::LocationUVE{5, 0}), kInvalidEntityUVE);
}

TEST_F(ArchetypeUVETest, VacateWithoutDestroyUVE_OutOfRangeChunkIndexReturnsInvalidEntity) {
    EXPECT_EQ(archetype.VacateWithoutDestroyUVE(ArchetypeUVE::LocationUVE{5, 0}), kInvalidEntityUVE);
}

TEST_F(ArchetypeUVETest, GetChunkUVE_OutOfRangeChunkIndexThrowsOutOfRangeInsteadOfIndexingPastChunks) {
    EXPECT_THROW({ static_cast<void>(archetype.GetChunkUVE(5)); }, std::out_of_range);

    const ArchetypeUVE& constArchetype = archetype;
    EXPECT_THROW({ static_cast<void>(constArchetype.GetChunkUVE(5)); }, std::out_of_range);
}
#endif

} // namespace
} // namespace UVE::Scene::Detail::Tests
