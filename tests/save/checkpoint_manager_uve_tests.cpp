// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/save/checkpoint_manager_uve.h"

#include <filesystem>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/save/save_game_system_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_serializer_uve.h"

namespace UVE::Save::Tests {
namespace {

using Scene::EntityManagerUVE;
using Scene::EntityUVE;
using Scene::SceneSerializerUVE;

class CheckpointManagerUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneSerializerUVE sceneSerializer;
    std::filesystem::path saveDirectory = "uve_checkpoint_manager_tests_saves";
    SaveGameSystemUVE saveGameSystem{sceneSerializer, saveDirectory};
    CheckpointManagerUVE checkpointManager{saveGameSystem, 2.0};

    [[nodiscard]] std::vector<EntityUVE> MakeRootEntitiesUVE() {
        return {entityManager.CreateEntityUVE()};
    }

    void TearDown() override {
        std::error_code errorCode;
        std::filesystem::remove_all(saveDirectory, errorCode);
    }
};

TEST_F(CheckpointManagerUVETest, UpdateUVE_BeforeIntervalElapses_NoSaveWritten) {
    checkpointManager.UpdateUVE(1.0, entityManager, MakeRootEntitiesUVE());
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
}

TEST_F(CheckpointManagerUVETest, CheckpointUVE_FailedSavePreservesElapsedTime) {
    checkpointManager.UpdateUVE(0.5, entityManager, MakeRootEntitiesUVE());
    ASSERT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.5);

    EXPECT_FALSE(checkpointManager.CheckpointUVE(entityManager, {Scene::kInvalidEntityUVE}));
    EXPECT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.5);
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kManualCheckpointSlotIndexUVE));
}

TEST_F(CheckpointManagerUVETest, UpdateUVE_AtOrAfterIntervalElapses_WritesAutoSaveAndResetsElapsed) {
    checkpointManager.UpdateUVE(1.0, entityManager, MakeRootEntitiesUVE());
    checkpointManager.UpdateUVE(1.5, entityManager, MakeRootEntitiesUVE());

    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
    EXPECT_LT(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 2.0);
}

TEST_F(CheckpointManagerUVETest, UpdateUVE_RejectsInvalidDeltaWithoutChangingCounters) {
    checkpointManager.UpdateUVE(0.5, entityManager, MakeRootEntitiesUVE());
    ASSERT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.5);
    ASSERT_DOUBLE_EQ(checkpointManager.GetTotalPlaytimeSecondsUVE(), 0.5);

    checkpointManager.UpdateUVE(-1.0, entityManager, MakeRootEntitiesUVE());
    checkpointManager.UpdateUVE(std::numeric_limits<double>::quiet_NaN(), entityManager,
                                MakeRootEntitiesUVE());
    checkpointManager.UpdateUVE(std::numeric_limits<double>::infinity(), entityManager,
                                MakeRootEntitiesUVE());

    EXPECT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.5);
    EXPECT_DOUBLE_EQ(checkpointManager.GetTotalPlaytimeSecondsUVE(), 0.5);
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));

    checkpointManager.UpdateUVE(1.5, entityManager, MakeRootEntitiesUVE());
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
}

TEST_F(CheckpointManagerUVETest, UpdateUVE_AccumulatesTotalPlaytimeAcrossCalls) {
    checkpointManager.UpdateUVE(1.0, entityManager, MakeRootEntitiesUVE());
    checkpointManager.UpdateUVE(0.5, entityManager, MakeRootEntitiesUVE());
    EXPECT_DOUBLE_EQ(checkpointManager.GetTotalPlaytimeSecondsUVE(), 1.5);
}

TEST_F(CheckpointManagerUVETest, CheckpointUVE_WritesImmediately_RegardlessOfElapsedTime) {
    checkpointManager.UpdateUVE(0.1, entityManager, MakeRootEntitiesUVE());
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));

    EXPECT_TRUE(checkpointManager.CheckpointUVE(entityManager, MakeRootEntitiesUVE()));
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kManualCheckpointSlotIndexUVE));
}

TEST_F(CheckpointManagerUVETest, CheckpointUVE_ResetsElapsedTimer_DefersNextAutoSave) {
    ASSERT_TRUE(checkpointManager.CheckpointUVE(entityManager, MakeRootEntitiesUVE()));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kManualCheckpointSlotIndexUVE));
    EXPECT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 0.0);

    checkpointManager.UpdateUVE(1.9, entityManager, MakeRootEntitiesUVE());
    // Still under the 2.0s interval since the checkpoint reset the timer - no second auto-save.
    EXPECT_LT(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 2.0);
}

TEST_F(CheckpointManagerUVETest, AutoSaveNeverCollidesWithNumberedSlots) {
    const EntityUVE numberedEntity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(7, entityManager, {numberedEntity}, GameStateMetadataUVE{}));

    ASSERT_TRUE(checkpointManager.CheckpointUVE(entityManager, MakeRootEntitiesUVE()));

    EXPECT_TRUE(saveGameSystem.HasSaveUVE(7));
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kManualCheckpointSlotIndexUVE));
    EXPECT_EQ(saveGameSystem.ListUsedSlotsUVE(), (std::vector<int>{7}));
}

TEST_F(CheckpointManagerUVETest, SetAutoSaveIntervalSecondsUVE_TakesEffectWithoutResettingElapsed) {
    checkpointManager.UpdateUVE(1.5, entityManager, MakeRootEntitiesUVE());
    ASSERT_FALSE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));

    checkpointManager.SetAutoSaveIntervalSecondsUVE(1.0);
    EXPECT_DOUBLE_EQ(checkpointManager.GetElapsedSinceLastSaveSecondsUVE(), 1.5);

    // The next UpdateUVE() call should fire immediately since 1.5s already exceeds the new 1.0s
    // interval, even with a tiny additional delta.
    checkpointManager.UpdateUVE(0.01, entityManager, MakeRootEntitiesUVE());
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
}

TEST_F(CheckpointManagerUVETest, AutoSaveAndManualCheckpoint_UseIndependentReservedSlots) {
    checkpointManager.UpdateUVE(2.0, entityManager, MakeRootEntitiesUVE());
    ASSERT_TRUE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
    ASSERT_TRUE(checkpointManager.CheckpointUVE(entityManager, MakeRootEntitiesUVE()));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kAutoSaveSlotIndexUVE));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(kManualCheckpointSlotIndexUVE));
    EXPECT_EQ(saveGameSystem.GetSaveMetadataUVE(kAutoSaveSlotIndexUVE)->slotIndex, kAutoSaveSlotIndexUVE);
    EXPECT_EQ(saveGameSystem.GetSaveMetadataUVE(kManualCheckpointSlotIndexUVE)->slotIndex,
              kManualCheckpointSlotIndexUVE);
}

TEST_F(CheckpointManagerUVETest, GetTotalPlaytimeSecondsUVE_FeedsIntoSavedMetadata) {
    checkpointManager.UpdateUVE(1.0, entityManager, MakeRootEntitiesUVE());
    ASSERT_TRUE(checkpointManager.CheckpointUVE(entityManager, MakeRootEntitiesUVE()));

    const std::optional<GameStateMetadataUVE> metadata = saveGameSystem.GetSaveMetadataUVE(kManualCheckpointSlotIndexUVE);
    ASSERT_TRUE(metadata.has_value());
    EXPECT_DOUBLE_EQ(metadata->playtimeSeconds, checkpointManager.GetTotalPlaytimeSecondsUVE());
}

} // namespace
} // namespace UVE::Save::Tests
