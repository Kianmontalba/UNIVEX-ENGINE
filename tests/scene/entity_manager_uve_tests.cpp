// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/entity_manager_uve.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/scene/entity_lifecycle_events_uve.h"

namespace UVE::Scene::Tests {
namespace {

struct PositionComponentUVE {
    int value = 0;
};

struct TagComponentUVE {
    int value = 0;
};

struct NonTrivialComponentUVE {
    std::string name;
};

class EntityManagerUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
};

TEST_F(EntityManagerUVETest, CreateEntityUVE_IsAliveAndCounted) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    EXPECT_TRUE(entityManager.IsAliveUVE(entity));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), 1U);
}

TEST_F(EntityManagerUVETest, DestroyEntityUVE_IsNoLongerAlive) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(entity);
    EXPECT_FALSE(entityManager.IsAliveUVE(entity));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), 0U);
}

#if UVE_DEBUG
TEST_F(EntityManagerUVETest, DestroyEntityUVE_StaleHandleAsserts) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(entity);
    EXPECT_DEATH({ entityManager.DestroyEntityUVE(entity); }, "");
}

TEST_F(EntityManagerUVETest, HasComponentUVE_StaleHandleAsserts) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(entity);
    EXPECT_DEATH({ static_cast<void>(entityManager.HasComponentUVE<PositionComponentUVE>(entity)); }, "");
}

TEST_F(EntityManagerUVETest, GetComponentTypesUVE_StaleHandleAsserts) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(entity);
    EXPECT_DEATH({ static_cast<void>(entityManager.GetComponentTypesUVE(entity)); }, "");
}
#else
TEST_F(EntityManagerUVETest, StaleHandleQueriesAndDestroyFailClosed) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(entity);

    EXPECT_NO_FATAL_FAILURE(entityManager.DestroyEntityUVE(entity));
    EXPECT_FALSE(entityManager.HasComponentUVE<PositionComponentUVE>(entity));
    EXPECT_TRUE(entityManager.GetComponentTypesUVE(entity).empty());
    EXPECT_EQ(entityManager.GetEntityCountUVE(), 0U);
}
#endif

TEST_F(EntityManagerUVETest, ReusedIndex_YieldsDifferentGeneration) {
    const EntityUVE first = entityManager.CreateEntityUVE();
    entityManager.DestroyEntityUVE(first);
    const EntityUVE second = entityManager.CreateEntityUVE();

    EXPECT_EQ(first.index, second.index);
    EXPECT_NE(first.generation, second.generation);
    EXPECT_FALSE(entityManager.IsAliveUVE(first));
    EXPECT_TRUE(entityManager.IsAliveUVE(second));
}

TEST_F(EntityManagerUVETest, AddGetHasRemoveComponent_PlainData_RoundTrips) {
    const EntityUVE entity = entityManager.CreateEntityUVE();

    EXPECT_FALSE(entityManager.HasComponentUVE<PositionComponentUVE>(entity));
    PositionComponentUVE& added = entityManager.AddComponentUVE<PositionComponentUVE>(entity);
    added.value = 42;

    EXPECT_TRUE(entityManager.HasComponentUVE<PositionComponentUVE>(entity));
    EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entity).value, 42);

    entityManager.RemoveComponentUVE<PositionComponentUVE>(entity);
    EXPECT_FALSE(entityManager.HasComponentUVE<PositionComponentUVE>(entity));
}

TEST_F(EntityManagerUVETest, AddGetRemoveComponent_NonTrivialType_RoundTrips) {
    const EntityUVE entity = entityManager.CreateEntityUVE();

    entityManager.AddComponentUVE<NonTrivialComponentUVE>(entity, "hello");
    EXPECT_EQ(entityManager.GetComponentUVE<NonTrivialComponentUVE>(entity).name, "hello");

    entityManager.RemoveComponentUVE<NonTrivialComponentUVE>(entity);
    EXPECT_FALSE(entityManager.HasComponentUVE<NonTrivialComponentUVE>(entity));
}

TEST_F(EntityManagerUVETest, ArchetypeMigration_DoesNotCorruptUnrelatedEntity) {
    const EntityUVE entityA = entityManager.CreateEntityUVE();
    const EntityUVE entityB = entityManager.CreateEntityUVE();

    entityManager.AddComponentUVE<PositionComponentUVE>(entityA).value = 1;
    entityManager.AddComponentUVE<PositionComponentUVE>(entityB).value = 2;

    // entityA migrates to a new archetype (Position + Tag); entityB must be unaffected.
    entityManager.AddComponentUVE<TagComponentUVE>(entityA).value = 100;

    EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entityA).value, 1);
    EXPECT_EQ(entityManager.GetComponentUVE<TagComponentUVE>(entityA).value, 100);
    EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entityB).value, 2);
    EXPECT_FALSE(entityManager.HasComponentUVE<TagComponentUVE>(entityB));

    entityManager.RemoveComponentUVE<TagComponentUVE>(entityA);
    EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entityA).value, 1);
    EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entityB).value, 2);
}

TEST_F(EntityManagerUVETest, SwapRemove_DestroyingMiddleEntity_KeepsSurvivorsCorrect) {
    std::vector<EntityUVE> entities;
    constexpr int kEntityCount = 10;
    for (int i = 0; i < kEntityCount; ++i) {
        const EntityUVE entity = entityManager.CreateEntityUVE();
        entityManager.AddComponentUVE<PositionComponentUVE>(entity).value = i;
        entities.push_back(entity);
    }

    // Destroy one from the middle.
    entityManager.DestroyEntityUVE(entities[5]);
    entities.erase(entities.begin() + 5);

    for (std::size_t i = 0; i < entities.size(); ++i) {
        ASSERT_TRUE(entityManager.IsAliveUVE(entities[i]));
    }
    // Every surviving entity's own value must still match what it was created with (a
    // corrupted swap-remove would show up as a mismatched value somewhere here).
    std::vector<int> observedValues;
    for (const EntityUVE& entity : entities) {
        observedValues.push_back(entityManager.GetComponentUVE<PositionComponentUVE>(entity).value);
    }
    const std::vector<int> expectedValues{0, 1, 2, 3, 4, 6, 7, 8, 9};
    // Order is not guaranteed by swap-remove, so compare as a multiset via sorting.
    std::vector<int> sortedObserved = observedValues;
    std::vector<int> sortedExpected = expectedValues;
    std::sort(sortedObserved.begin(), sortedObserved.end());
    std::sort(sortedExpected.begin(), sortedExpected.end());
    EXPECT_EQ(sortedObserved, sortedExpected);

    // The freed index must be reusable.
    const EntityUVE reused = entityManager.CreateEntityUVE();
    EXPECT_TRUE(entityManager.IsAliveUVE(reused));
}

TEST_F(EntityManagerUVETest, ChunkOverflow_MoreThanOneChunkCapacity_AllEntitiesRemainCorrect) {
    constexpr int kEntityCount = 600; // > kChunkCapacityUVE (512), forces a second chunk
    std::vector<EntityUVE> entities;
    entities.reserve(kEntityCount);
    for (int i = 0; i < kEntityCount; ++i) {
        const EntityUVE entity = entityManager.CreateEntityUVE();
        entityManager.AddComponentUVE<PositionComponentUVE>(entity).value = i;
        entities.push_back(entity);
    }

    for (int i = 0; i < kEntityCount; ++i) {
        EXPECT_EQ(entityManager.GetComponentUVE<PositionComponentUVE>(entities[static_cast<std::size_t>(i)]).value, i);
    }
}

TEST_F(EntityManagerUVETest, ForEachUVE_VisitsOnlyEntitiesWithAllRequestedComponents) {
    const EntityUVE onlyPosition = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<PositionComponentUVE>(onlyPosition).value = 1;

    const EntityUVE onlyTag = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<TagComponentUVE>(onlyTag).value = 2;

    const EntityUVE both = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<PositionComponentUVE>(both).value = 3;
    entityManager.AddComponentUVE<TagComponentUVE>(both).value = 4;

    std::vector<EntityUVE> visited;
    entityManager.ForEachUVE<PositionComponentUVE, TagComponentUVE>(
        [&visited](EntityUVE entity, PositionComponentUVE&, TagComponentUVE&) { visited.push_back(entity); });

    ASSERT_EQ(visited.size(), 1U);
    EXPECT_EQ(visited[0], both);
}

TEST_F(EntityManagerUVETest, EntityCreatedAndDestroyedEvents_ArePublished) {
    EntityUVE createdEntity = kInvalidEntityUVE;
    EntityUVE destroyedEntity = kInvalidEntityUVE;
    eventSystem.Subscribe<EntityCreatedEventUVE>(
        [&createdEntity](const EntityCreatedEventUVE& event) { createdEntity = event.entity; });
    eventSystem.Subscribe<EntityDestroyedEventUVE>(
        [&destroyedEntity](const EntityDestroyedEventUVE& event) { destroyedEntity = event.entity; });

    const EntityUVE entity = entityManager.CreateEntityUVE();
    EXPECT_EQ(createdEntity, entity);

    entityManager.DestroyEntityUVE(entity);
    EXPECT_EQ(destroyedEntity, entity);
}

TEST(EntityManagerUVEDestructorTest, DestructorWithLiveEntities_LeavesNoMemoryLeaks) {
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    {
        EntityManagerUVE entityManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
        for (int i = 0; i < 5; ++i) {
            const EntityUVE entity = entityManager.CreateEntityUVE();
            entityManager.AddComponentUVE<NonTrivialComponentUVE>(entity, "still alive at shutdown");
        }
        // entityManager destructs here without any explicit DestroyEntityUVE() calls.
    }
    EXPECT_FALSE(memoryManager.HasLeaksUVE());
}

} // namespace
} // namespace UVE::Scene::Tests
