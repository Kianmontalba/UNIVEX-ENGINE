// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/save/save_game_system_uve.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/save/save_payload_compression_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_serializer_uve.h"

namespace UVE::Save::Tests {
namespace {

using Scene::EntityManagerUVE;
using Scene::EntityUVE;
using Scene::HierarchyComponentUVE;
using Scene::kInvalidEntityUVE;
using Scene::LightComponentUVE;
using Scene::MeshComponentUVE;
using Scene::RigidBodyComponentUVE;
using Scene::SceneSerializerUVE;

class SaveGameSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneSerializerUVE sceneSerializer;
    std::filesystem::path saveDirectory = "uve_save_game_system_tests_saves";
    SaveGameSystemUVE saveGameSystem{sceneSerializer, saveDirectory};

    void TearDown() override {
        std::error_code errorCode;
        std::filesystem::remove_all(saveDirectory, errorCode);
    }
};

TEST_F(SaveGameSystemUVETest, SaveThenLoad_SingleEntityWithComponents_RoundTripsExactly) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(
        entity, MeshComponentUVE{Asset::AssetGuidUVE{111}, Asset::AssetGuidUVE{222}});
    entityManager.AddComponentUVE<LightComponentUVE>(entity,
                                                       LightComponentUVE{Math::Vector3UVE{0.2F, 0.4F, 0.6F}, 2.5F});

    GameStateMetadataUVE metadata;
    metadata.saveName = "Before the Dragon Fight";
    metadata.playtimeSeconds = 123.5;
    ASSERT_TRUE(saveGameSystem.SaveUVE(5, entityManager, {entity}, metadata));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = saveGameSystem.LoadUVE(5, loadedManager);
    ASSERT_EQ(roots.size(), 1U);
    const EntityUVE loaded = roots[0];

    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(loaded).meshGuid, Asset::AssetGuidUVE{111});
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<LightComponentUVE>(loaded).intensity, 2.5F);
}

TEST_F(SaveGameSystemUVETest, SaveThenLoad_MultipleRootEntitiesWithHierarchy_RoundTripsExactly) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(parent, HierarchyComponentUVE{kInvalidEntityUVE});
    const EntityUVE child = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(child, HierarchyComponentUVE{parent});

    ASSERT_TRUE(saveGameSystem.SaveUVE(7, entityManager, {parent}, GameStateMetadataUVE{}));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = saveGameSystem.LoadUVE(7, loadedManager);
    ASSERT_EQ(roots.size(), 1U);

    EntityUVE loadedChild = kInvalidEntityUVE;
    loadedManager.ForEachUVE<HierarchyComponentUVE>(
        [&loadedChild, parentEntity = roots[0]](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == parentEntity) {
                loadedChild = entity;
            }
        });
    EXPECT_NE(loadedChild, kInvalidEntityUVE);
}

TEST_F(SaveGameSystemUVETest, GetSaveMetadataUVE_RejectsTruncatedAndTrailingWorldSections) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(16, entityManager, {entity}, GameStateMetadataUVE{}));
    const std::filesystem::path slotPath = saveDirectory / "slot_16.uvesave";
    const auto originalFile = Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(originalFile.has_value());

    std::vector<std::byte> expandedPayload;
    ASSERT_TRUE(DecompressSavePayloadUVE(originalFile->second, expandedPayload));
    ASSERT_GT(expandedPayload.size(), sizeof(std::uint32_t) + sizeof(std::uint64_t));

    std::vector<std::byte> truncatedPayload = expandedPayload;
    truncatedPayload.pop_back();
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save,
                                       CompressSavePayloadUVE(truncatedPayload)));
    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(16).has_value());

    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save,
                                       CompressSavePayloadUVE(expandedPayload)));
    expandedPayload.push_back(std::byte{0xA5});
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save,
                                       CompressSavePayloadUVE(expandedPayload)));
    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(16).has_value());
}

TEST_F(SaveGameSystemUVETest, SaveUVE_OverwritesExistingSlot) {
    const EntityUVE firstEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(firstEntity, RigidBodyComponentUVE{1.0F, false});
    ASSERT_TRUE(saveGameSystem.SaveUVE(2, entityManager, {firstEntity}, GameStateMetadataUVE{}));

    const EntityUVE secondEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(secondEntity, RigidBodyComponentUVE{9.0F, true});
    ASSERT_TRUE(saveGameSystem.SaveUVE(2, entityManager, {secondEntity}, GameStateMetadataUVE{}));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = saveGameSystem.LoadUVE(2, loadedManager);
    ASSERT_EQ(roots.size(), 1U);
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(roots[0]).mass, 9.0F);
    EXPECT_TRUE(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(roots[0]).isKinematic);
}

TEST_F(SaveGameSystemUVETest, HasSaveUVE_ReflectsPresenceAfterSaveAndDelete) {
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(4));

    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(4, entityManager, {entity}, GameStateMetadataUVE{}));
    EXPECT_TRUE(saveGameSystem.HasSaveUVE(4));

    EXPECT_TRUE(saveGameSystem.DeleteSaveUVE(4));
    EXPECT_FALSE(saveGameSystem.HasSaveUVE(4));
}

TEST_F(SaveGameSystemUVETest, DeleteSaveUVE_EmptySlot_ReturnsFalseWithoutError) {
    EXPECT_FALSE(saveGameSystem.DeleteSaveUVE(10));
}

TEST_F(SaveGameSystemUVETest, GetSaveMetadataUVE_ReturnsMetadataWithoutCreatingEntities) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    GameStateMetadataUVE metadata;
    metadata.saveName = "My Save";
    metadata.playtimeSeconds = 42.0;
    ASSERT_TRUE(saveGameSystem.SaveUVE(8, entityManager, {entity}, metadata));

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::optional<GameStateMetadataUVE> loaded = saveGameSystem.GetSaveMetadataUVE(8);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->saveName, "My Save");
    EXPECT_DOUBLE_EQ(loaded->playtimeSeconds, 42.0);
    EXPECT_EQ(loaded->slotIndex, 8);
    EXPECT_GT(loaded->savedAtUnixSecondsUVE, 0);
    EXPECT_EQ(freshManager.GetEntityCountUVE(), 0U);
}

TEST_F(SaveGameSystemUVETest, GetSaveMetadataUVE_RejectsEmbeddedSlotIdentityMismatch) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(16, entityManager, {entity}, GameStateMetadataUVE{}));
    const std::filesystem::path slotPath = saveDirectory / "slot_16.uvesave";
    const auto originalFile = Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(originalFile.has_value());

    std::vector<std::byte> expandedPayload;
    ASSERT_TRUE(DecompressSavePayloadUVE(originalFile->second, expandedPayload));
    ASSERT_GE(expandedPayload.size(), sizeof(std::uint32_t));
    std::uint32_t metadataLength = 0U;
    std::memcpy(&metadataLength, expandedPayload.data(), sizeof(metadataLength));
    ASSERT_LE(sizeof(metadataLength) + static_cast<std::size_t>(metadataLength), expandedPayload.size());
    std::string metadata(reinterpret_cast<const char*>(expandedPayload.data() + sizeof(metadataLength)), metadataLength);
    const std::string expectedSlot = "\"slotIndex\":16";
    const std::size_t slotOffset = metadata.find(expectedSlot);
    ASSERT_NE(slotOffset, std::string::npos);
    metadata.replace(slotOffset, expectedSlot.size(), "\"slotIndex\":17");
    std::memcpy(expandedPayload.data() + sizeof(metadataLength), metadata.data(), metadata.size());
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save,
                                       CompressSavePayloadUVE(expandedPayload)));

    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(16).has_value());
    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    EXPECT_TRUE(saveGameSystem.LoadUVE(16, loadedManager).empty());
    EXPECT_EQ(loadedManager.GetEntityCountUVE(), 0U);
}

TEST_F(SaveGameSystemUVETest, GetSaveMetadataUVE_EmptySlot_ReturnsNullopt) {
    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(11).has_value());
}

TEST_F(SaveGameSystemUVETest, ListUsedSlotsUVE_ReturnsAscendingUsedSlotsOnly_ExcludesAutoSaveSlot) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(40, entityManager, {entity}, GameStateMetadataUVE{}));
    ASSERT_TRUE(saveGameSystem.SaveUVE(1, entityManager, {entity}, GameStateMetadataUVE{}));
    ASSERT_TRUE(saveGameSystem.SaveUVE(3, entityManager, {entity}, GameStateMetadataUVE{}));
    ASSERT_TRUE(saveGameSystem.SaveUVE(kAutoSaveSlotIndexUVE, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::vector<int> used = saveGameSystem.ListUsedSlotsUVE();
    EXPECT_EQ(used, (std::vector<int>{1, 3, 40}));
}

TEST_F(SaveGameSystemUVETest, ListUsedSlotsUVE_NoSaveDirectoryYet_ReturnsEmptyNotError) {
    EXPECT_TRUE(saveGameSystem.ListUsedSlotsUVE().empty());
}

TEST_F(SaveGameSystemUVETest, SaveUVE_OutOfRangeSlotIndex_ReturnsFalse) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    EXPECT_FALSE(saveGameSystem.SaveUVE(-3, entityManager, {entity}, GameStateMetadataUVE{}));
    EXPECT_FALSE(saveGameSystem.SaveUVE(kSaveSlotCountUVE, entityManager, {entity}, GameStateMetadataUVE{}));
    EXPECT_FALSE(saveGameSystem.SaveUVE(1000, entityManager, {entity}, GameStateMetadataUVE{}));
}

TEST_F(SaveGameSystemUVETest, LoadUVE_OutOfRangeSlotIndex_ReturnsEmptyVector) {
    EXPECT_TRUE(saveGameSystem.LoadUVE(-3, entityManager).empty());
    EXPECT_TRUE(saveGameSystem.LoadUVE(kSaveSlotCountUVE, entityManager).empty());
}

TEST_F(SaveGameSystemUVETest, LoadUVE_MissingSlot_ReturnsEmptyVector) {
    EXPECT_TRUE(saveGameSystem.LoadUVE(20, entityManager).empty());
}

TEST_F(SaveGameSystemUVETest, SaveUVE_ReservedSlots_RoundTripIndependentlyOfNumberedSlots) {
    const EntityUVE numberedEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(numberedEntity, RigidBodyComponentUVE{1.0F, false});
    ASSERT_TRUE(saveGameSystem.SaveUVE(0, entityManager, {numberedEntity}, GameStateMetadataUVE{}));

    const EntityUVE autoSaveEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(autoSaveEntity, RigidBodyComponentUVE{5.0F, true});
    ASSERT_TRUE(saveGameSystem.SaveUVE(kAutoSaveSlotIndexUVE, entityManager, {autoSaveEntity}, GameStateMetadataUVE{}));

    const EntityUVE manualCheckpointEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(manualCheckpointEntity, RigidBodyComponentUVE{9.0F, true});
    ASSERT_TRUE(saveGameSystem.SaveUVE(kManualCheckpointSlotIndexUVE, entityManager, {manualCheckpointEntity},
                                       GameStateMetadataUVE{}));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> numberedRoots = saveGameSystem.LoadUVE(0, loadedManager);
    ASSERT_EQ(numberedRoots.size(), 1U);
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(numberedRoots[0]).mass, 1.0F);

    EntityManagerUVE loadedAutoSaveManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> autoSaveRoots = saveGameSystem.LoadUVE(kAutoSaveSlotIndexUVE, loadedAutoSaveManager);
    ASSERT_EQ(autoSaveRoots.size(), 1U);
    EXPECT_FLOAT_EQ(loadedAutoSaveManager.GetComponentUVE<RigidBodyComponentUVE>(autoSaveRoots[0]).mass, 5.0F);

    EntityManagerUVE loadedManualManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> manualRoots = saveGameSystem.LoadUVE(kManualCheckpointSlotIndexUVE, loadedManualManager);
    ASSERT_EQ(manualRoots.size(), 1U);
    EXPECT_FLOAT_EQ(loadedManualManager.GetComponentUVE<RigidBodyComponentUVE>(manualRoots[0]).mass, 9.0F);

    EXPECT_EQ(saveGameSystem.ListUsedSlotsUVE(), (std::vector<int>{0}));
}

TEST_F(SaveGameSystemUVETest, SaveThenLoad_ScratchFilesAreCleanedUpAfterward) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(9, entityManager, {entity}, GameStateMetadataUVE{}));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    ASSERT_FALSE(saveGameSystem.LoadUVE(9, loadedManager).empty());

    for (const auto& entry : std::filesystem::directory_iterator(saveDirectory)) {
        const std::string fileName = entry.path().filename().string();
        EXPECT_TRUE(fileName.starts_with("slot_")) << "unexpected leftover file: " << fileName;
    }
}

TEST_F(SaveGameSystemUVETest, LoadUVE_TruncatedFile_ReturnsEmptyVectorWithoutCrashing) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(6, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::filesystem::path slotPath = saveDirectory / "slot_06.uvesave";
    ASSERT_TRUE(std::filesystem::exists(slotPath));
    std::error_code errorCode;
    std::filesystem::resize_file(slotPath, 10, errorCode);
    ASSERT_FALSE(errorCode);

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    EXPECT_TRUE(saveGameSystem.LoadUVE(6, freshManager).empty());
}

TEST_F(SaveGameSystemUVETest, LoadUVE_PayloadWithBogusLengthPrefix_ReturnsEmptyVectorWithoutCrashing) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(12, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::filesystem::path slotPath = saveDirectory / "slot_12.uvesave";
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(file.has_value());
    std::vector<std::byte> payload = file->second;
    ASSERT_GE(payload.size(), sizeof(std::uint32_t));
    // Overwrite the metadataJsonLength prefix with an absurdly large value, so
    // SplitSavePayloadUVE's bounds check fails deterministically.
    const std::uint32_t bogusLength = 0xFFFFFFFFU;
    std::memcpy(payload.data(), &bogusLength, sizeof(bogusLength));
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save, payload));

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    EXPECT_TRUE(saveGameSystem.LoadUVE(12, freshManager).empty());
    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(12).has_value());
}

TEST_F(SaveGameSystemUVETest, LoadUVE_WorldLengthUint64OverflowIsRejectedWithoutCrashing) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(15, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::filesystem::path slotPath = saveDirectory / "slot_15.uvesave";
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(file.has_value());
    std::vector<std::byte> expandedPayload;
    ASSERT_TRUE(DecompressSavePayloadUVE(file->second, expandedPayload));

    std::uint32_t metadataLength = 0U;
    ASSERT_GE(expandedPayload.size(), sizeof(metadataLength));
    std::memcpy(&metadataLength, expandedPayload.data(), sizeof(metadataLength));
    const std::size_t worldLengthOffset = sizeof(metadataLength) + static_cast<std::size_t>(metadataLength);
    ASSERT_LE(worldLengthOffset + sizeof(std::uint64_t), expandedPayload.size());
    const std::uint64_t absurdWorldLength = std::numeric_limits<std::uint64_t>::max();
    std::memcpy(expandedPayload.data() + worldLengthOffset, &absurdWorldLength, sizeof(absurdWorldLength));
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save, expandedPayload));

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    EXPECT_TRUE(saveGameSystem.LoadUVE(15, freshManager).empty());
    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(15).has_value());
}

TEST_F(SaveGameSystemUVETest, SavePayloadMigrationRegistryUVE_ValidatesRegistrationAndFailureAtomicity) {
    SavePayloadMigrationRegistryUVE registry;
    EXPECT_EQ(registry.RegisterUVE(1U, 1U, [](std::vector<std::byte>&, std::string&) { return true; }).code,
              SaveMigrationRegistrationCodeUVE::InvalidVersionRange);
    EXPECT_EQ(registry.RegisterUVE(0U, 1U, {}).code, SaveMigrationRegistrationCodeUVE::MissingTransform);

    ASSERT_TRUE(registry.RegisterUVE(0U, 1U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x02});
        return true;
    }).IsAcceptedUVE());
    EXPECT_EQ(registry.RegisterUVE(0U, 1U, [](std::vector<std::byte>&, std::string&) { return true; }).code,
              SaveMigrationRegistrationCodeUVE::DuplicateTransform);

    std::vector<std::byte> migratedPayload{std::byte{0x01}};
    const SaveMigrationDiagnosticsUVE migrated = registry.MigrateUVE(0U, 1U, migratedPayload);
    EXPECT_EQ(migrated.status, SaveMigrationStatusUVE::Migrated);
    ASSERT_EQ(migratedPayload.size(), 2U);
    EXPECT_EQ(migratedPayload[1], std::byte{0x02});

    ASSERT_TRUE(registry.RegisterUVE(2U, 1U, [](std::vector<std::byte>& payload, std::string& reason) {
        payload.clear();
        reason = "legacy payload rejected";
        return false;
    }).IsAcceptedUVE());
    std::vector<std::byte> rejectedPayload{std::byte{0x03}};
    const SaveMigrationDiagnosticsUVE rejected = registry.MigrateUVE(2U, 1U, rejectedPayload);
    EXPECT_EQ(rejected.status, SaveMigrationStatusUVE::InvalidPayload);
    EXPECT_EQ(rejectedPayload, std::vector<std::byte>{std::byte{0x03}});
    EXPECT_EQ(registry.GetTransformCountUVE(), 2U);

    for (std::uint32_t index = 0U; index < kMaximumSaveMigrationTransformsUVE - 2U; ++index) {
        ASSERT_TRUE(registry.RegisterUVE(100U + index, 200U + index,
                                         [](std::vector<std::byte>& payload, std::string&) {
                                             payload.push_back(std::byte{0x04});
                                             return true;
                                         }).IsAcceptedUVE());
    }
    EXPECT_EQ(registry.GetTransformCountUVE(), kMaximumSaveMigrationTransformsUVE);
    EXPECT_EQ(registry.RegisterUVE(999U, 1000U, [](std::vector<std::byte>&, std::string&) { return true; }).code,
              SaveMigrationRegistrationCodeUVE::CapacityExceeded);
}

TEST_F(SaveGameSystemUVETest, SavePayloadMigrationRegistryUVE_ComposesDeterministicMultiHopPath) {
    SavePayloadMigrationRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE(0U, 1U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x02});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(1U, 2U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x03});
        return true;
    }).IsAcceptedUVE());

    std::vector<std::byte> payload{std::byte{0x01}};
    const SaveMigrationDiagnosticsUVE diagnostics = registry.MigrateUVE(0U, 2U, payload);

    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::Migrated);
    EXPECT_EQ(diagnostics.appliedStepCount, 2U);
    EXPECT_EQ(payload, (std::vector<std::byte>{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}}));
}

TEST_F(SaveGameSystemUVETest, SavePayloadMigrationRegistryUVE_ChoosesShortestRegistrationOrderPath) {
    SavePayloadMigrationRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE(0U, 1U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x01});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(1U, 3U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x13});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(0U, 2U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x02});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(2U, 3U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x23});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(0U, 3U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x33});
        return true;
    }).IsAcceptedUVE());

    std::vector<std::byte> payload{std::byte{0x00}};
    const SaveMigrationDiagnosticsUVE diagnostics = registry.MigrateUVE(0U, 3U, payload);

    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::Migrated);
    EXPECT_EQ(diagnostics.appliedStepCount, 1U);
    EXPECT_EQ(payload, (std::vector<std::byte>{std::byte{0x00}, std::byte{0x33}}));
}

TEST_F(SaveGameSystemUVETest, SavePayloadMigrationRegistryUVE_MultiHopFailureIsAtomic) {
    SavePayloadMigrationRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE(0U, 1U, [](std::vector<std::byte>& payload, std::string&) {
        payload.push_back(std::byte{0x02});
        return true;
    }).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE(1U, 2U, [](std::vector<std::byte>& payload, std::string& reason) {
        payload.push_back(std::byte{0x03});
        reason = "second migration rejected";
        return false;
    }).IsAcceptedUVE());

    const std::vector<std::byte> originalPayload{std::byte{0x01}};
    std::vector<std::byte> payload = originalPayload;
    const SaveMigrationDiagnosticsUVE diagnostics = registry.MigrateUVE(0U, 2U, payload);

    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::InvalidPayload);
    EXPECT_EQ(diagnostics.appliedStepCount, 1U);
    EXPECT_EQ(payload, originalPayload);
    EXPECT_EQ(diagnostics.reason, "second migration rejected");
}

TEST_F(SaveGameSystemUVETest, SaveGameSystemUVE_LoadsRegisteredVersionTransformAtomically) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(15, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::filesystem::path slotPath = saveDirectory / "slot_15.uvesave";
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(file.has_value());
    std::vector<std::byte> payload = file->second;
    ASSERT_GE(payload.size(), sizeof(std::uint32_t));
    std::uint32_t metadataLength = 0U;
    std::memcpy(&metadataLength, payload.data(), sizeof(metadataLength));
    ASSERT_GT(metadataLength, 0U);
    ASSERT_LE(sizeof(metadataLength) + metadataLength, payload.size());
    std::string metadataText(reinterpret_cast<const char*>(payload.data() + sizeof(metadataLength)), metadataLength);
    const std::string currentVersionToken = "\"payloadSchemaVersion\":1";
    const std::size_t tokenOffset = metadataText.find(currentVersionToken);
    ASSERT_NE(tokenOffset, std::string::npos);
    metadataText.replace(tokenOffset, currentVersionToken.size(), "\"payloadSchemaVersion\":0");
    std::memcpy(payload.data() + sizeof(metadataLength), metadataText.data(), metadataLength);
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save, payload));

    ASSERT_TRUE(saveGameSystem.RegisterMigrationUVE(0U, 1U, [](std::vector<std::byte>& payloadBytes, std::string& reason) {
        const std::string oldToken = "\"payloadSchemaVersion\":0";
        const std::string newToken = "\"payloadSchemaVersion\":1";
        const std::string text(reinterpret_cast<const char*>(payloadBytes.data()), payloadBytes.size());
        const std::size_t offset = text.find(oldToken);
        if (offset == std::string::npos) {
            reason = "legacy metadata schema token is missing";
            return false;
        }
        std::string migrated = text;
        migrated.replace(offset, oldToken.size(), newToken);
        const auto* bytes = reinterpret_cast<const std::byte*>(migrated.data());
        payloadBytes.assign(bytes, bytes + migrated.size());
        return true;
    }).IsAcceptedUVE());

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    ASSERT_FALSE(saveGameSystem.LoadUVE(15, loadedManager).empty());
    const SaveMigrationDiagnosticsUVE diagnostics = saveGameSystem.GetLastMigrationDiagnosticsUVE();
    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::Migrated);
    EXPECT_EQ(diagnostics.sourceSchemaVersion, 0U);
    EXPECT_EQ(diagnostics.targetSchemaVersion, 1U);
}

TEST_F(SaveGameSystemUVETest, LegacyMetadataMigrationRunsBeforeDecodeForLoadAndMetadata) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    GameStateMetadataUVE metadata;
    metadata.playtimeSeconds = 321.5;
    ASSERT_TRUE(saveGameSystem.SaveUVE(16, entityManager, {entity}, metadata));

    const std::filesystem::path slotPath = saveDirectory / "slot_16.uvesave";
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(file.has_value());
    std::vector<std::byte> payload = file->second;
    ASSERT_GE(payload.size(), sizeof(std::uint32_t));
    std::uint32_t metadataLength = 0U;
    std::memcpy(&metadataLength, payload.data(), sizeof(metadataLength));
    ASSERT_GT(metadataLength, 0U);
    ASSERT_LE(sizeof(metadataLength) + metadataLength, payload.size());
    std::string metadataText(reinterpret_cast<const char*>(payload.data() + sizeof(metadataLength)), metadataLength);
    const std::string currentVersionToken = "\"payloadSchemaVersion\":1";
    const std::size_t versionOffset = metadataText.find(currentVersionToken);
    ASSERT_NE(versionOffset, std::string::npos);
    metadataText.replace(versionOffset, currentVersionToken.size(), "\"payloadSchemaVersion\":0");
    const std::string currentPlaytimeKey = "\"playtimeSeconds\"";
    const std::string legacyPlaytimeKey = "\"legacyTimeValue\"";
    const std::size_t playtimeOffset = metadataText.find(currentPlaytimeKey);
    ASSERT_NE(playtimeOffset, std::string::npos);
    ASSERT_EQ(currentPlaytimeKey.size(), legacyPlaytimeKey.size());
    metadataText.replace(playtimeOffset, currentPlaytimeKey.size(), legacyPlaytimeKey);
    ASSERT_EQ(metadataText.size(), metadataLength);
    std::memcpy(payload.data() + sizeof(metadataLength), metadataText.data(), metadataLength);
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save, payload));

    ASSERT_TRUE(saveGameSystem.RegisterMigrationUVE(0U, 1U, [](std::vector<std::byte>& payloadBytes, std::string& reason) {
        std::string text(reinterpret_cast<const char*>(payloadBytes.data()), payloadBytes.size());
        const std::string oldVersionToken = "\"payloadSchemaVersion\":0";
        const std::string newVersionToken = "\"payloadSchemaVersion\":1";
        const std::size_t migratedVersionOffset = text.find(oldVersionToken);
        const std::size_t legacyPlaytimeOffset = text.find("\"legacyTimeValue\"");
        if (migratedVersionOffset == std::string::npos || legacyPlaytimeOffset == std::string::npos) {
            reason = "legacy metadata fields are missing from the framed payload";
            return false;
        }
        text.replace(migratedVersionOffset, oldVersionToken.size(), newVersionToken);
        text.replace(legacyPlaytimeOffset, std::string{"\"legacyTimeValue\""}.size(),
                     "\"playtimeSeconds\"");
        const auto* bytes = reinterpret_cast<const std::byte*>(text.data());
        payloadBytes.assign(bytes, bytes + text.size());
        return true;
    }).IsAcceptedUVE());

    const std::optional<GameStateMetadataUVE> migratedMetadata = saveGameSystem.GetSaveMetadataUVE(16);
    ASSERT_TRUE(migratedMetadata.has_value());
    EXPECT_DOUBLE_EQ(migratedMetadata->playtimeSeconds, 321.5);
    EXPECT_EQ(saveGameSystem.GetLastMigrationDiagnosticsUVE().status, SaveMigrationStatusUVE::Migrated);

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    ASSERT_FALSE(saveGameSystem.LoadUVE(16, loadedManager).empty());
    EXPECT_EQ(saveGameSystem.GetLastMigrationDiagnosticsUVE().status, SaveMigrationStatusUVE::Migrated);
}

TEST_F(SaveGameSystemUVETest, SaveThenLoad_CurrentSchemaReportsNoMigrationRequired) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(13, entityManager, {entity}, GameStateMetadataUVE{}));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    ASSERT_FALSE(saveGameSystem.LoadUVE(13, loadedManager).empty());
    const SaveMigrationDiagnosticsUVE diagnostics = saveGameSystem.GetLastMigrationDiagnosticsUVE();
    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::NotRequired);
    EXPECT_EQ(diagnostics.sourceSchemaVersion, kCurrentSavePayloadSchemaVersionUVE);
    EXPECT_EQ(diagnostics.targetSchemaVersion, kCurrentSavePayloadSchemaVersionUVE);
    EXPECT_TRUE(diagnostics.reason.empty());
}

TEST_F(SaveGameSystemUVETest, LoadUVE_UnsupportedSchemaReportsBoundedMigrationDiagnostics) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ASSERT_TRUE(saveGameSystem.SaveUVE(14, entityManager, {entity}, GameStateMetadataUVE{}));

    const std::filesystem::path slotPath = saveDirectory / "slot_14.uvesave";
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(slotPath);
    ASSERT_TRUE(file.has_value());
    std::vector<std::byte> payload = file->second;
    ASSERT_GE(payload.size(), sizeof(std::uint32_t));

    std::uint32_t metadataLength = 0U;
    std::memcpy(&metadataLength, payload.data(), sizeof(metadataLength));
    ASSERT_GT(metadataLength, 0U);
    ASSERT_LE(sizeof(metadataLength) + metadataLength, payload.size());
    std::string metadataText(reinterpret_cast<const char*>(payload.data() + sizeof(metadataLength)), metadataLength);
    const std::string currentVersionToken = "\"payloadSchemaVersion\":1";
    const std::size_t tokenOffset = metadataText.find(currentVersionToken);
    ASSERT_NE(tokenOffset, std::string::npos);
    metadataText.replace(tokenOffset, currentVersionToken.size(), "\"payloadSchemaVersion\":9");
    ASSERT_EQ(metadataText.size(), metadataLength);
    std::memcpy(payload.data() + sizeof(metadataLength), metadataText.data(), metadataLength);
    ASSERT_TRUE(Asset::WriteUveFileUVE(slotPath, Asset::AssetKindUVE::Save, payload));

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    EXPECT_TRUE(saveGameSystem.LoadUVE(14, freshManager).empty());
    SaveMigrationDiagnosticsUVE diagnostics = saveGameSystem.GetLastMigrationDiagnosticsUVE();
    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::UnsupportedSourceVersion);
    EXPECT_EQ(diagnostics.sourceSchemaVersion, 9U);
    EXPECT_EQ(diagnostics.targetSchemaVersion, kCurrentSavePayloadSchemaVersionUVE);
    EXPECT_FALSE(diagnostics.reason.empty());

    EXPECT_FALSE(saveGameSystem.GetSaveMetadataUVE(14).has_value());
    diagnostics = saveGameSystem.GetLastMigrationDiagnosticsUVE();
    EXPECT_EQ(diagnostics.status, SaveMigrationStatusUVE::UnsupportedSourceVersion);
    EXPECT_EQ(diagnostics.sourceSchemaVersion, 9U);
}

TEST_F(SaveGameSystemUVETest, SaveUVE_NegativePlaytimeMetadataFailsBeforeDirectoryMutation) {
    GameStateMetadataUVE metadata;
    metadata.playtimeSeconds = -1.0;
    const std::filesystem::path invalidMetadataDirectory = "uve_save_game_system_tests_invalid_metadata_negative";
    std::filesystem::remove_all(invalidMetadataDirectory);

    EXPECT_FALSE(saveGameSystem.SaveUVE(3, entityManager, {}, metadata));
    EXPECT_FALSE(std::filesystem::exists(invalidMetadataDirectory));
}

TEST_F(SaveGameSystemUVETest, SaveUVE_NonFinitePlaytimeMetadataFailsBeforeDirectoryMutation) {
    GameStateMetadataUVE metadata;
    metadata.playtimeSeconds = std::numeric_limits<double>::quiet_NaN();
    const std::filesystem::path invalidMetadataDirectory = "uve_save_game_system_tests_invalid_metadata_nan";
    std::filesystem::remove_all(invalidMetadataDirectory);

    SaveGameSystemUVE invalidMetadataSystem(sceneSerializer, invalidMetadataDirectory);
    EXPECT_FALSE(invalidMetadataSystem.SaveUVE(3, entityManager, {}, metadata));
    EXPECT_FALSE(std::filesystem::exists(invalidMetadataDirectory));
}

TEST_F(SaveGameSystemUVETest, SaveUVE_DirectoryCannotBeCreated_FailsWithoutLeavingAnyFile) {
    // Point the save directory at a path where a path component is a regular file rather than a
    // directory - directory creation fails structurally (ENOTDIR) regardless of privilege level
    // (unlike a permission-based failure, which root would bypass in this sandbox).
    const std::filesystem::path blockerFile = "uve_save_game_system_tests_blocker_file";
    {
        std::ofstream blocker(blockerFile);
        blocker << "not a directory";
    }
    const std::filesystem::path badSaveDirectory = blockerFile / "saves";
    SaveGameSystemUVE brokenSaveGameSystem(sceneSerializer, badSaveDirectory);

    const EntityUVE entity = entityManager.CreateEntityUVE();
    EXPECT_FALSE(brokenSaveGameSystem.SaveUVE(3, entityManager, {entity}, GameStateMetadataUVE{}));
    EXPECT_FALSE(std::filesystem::exists(badSaveDirectory));

    std::filesystem::remove(blockerFile);
}

} // namespace
} // namespace UVE::Save::Tests
