// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/material_asset_uve.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] MaterialAssetUVE MakeTestMaterialUVE() {
    MaterialAssetUVE material;
    material.albedoColor = Math::Vector3UVE{0.8F, 0.2F, 0.2F};
    material.albedoTexture = AssetGuidUVE{111};
    material.normalTexture = AssetGuidUVE{222};
    material.metallic = 0.5F;
    material.roughness = 0.3F;
    material.aoTexture = AssetGuidUVE{333};
    material.emissiveColor = Math::Vector3UVE{0.1F, 0.0F, 0.0F};
    material.vertexShader = AssetGuidUVE{444};
    material.fragmentShader = AssetGuidUVE{555};
    material.isTransparent = true;
    return material;
}

TEST(MaterialAssetUVETest, DefaultConstruction_HasSensibleDefaults) {
    constexpr MaterialAssetUVE material;
    EXPECT_EQ(material.albedoTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.normalTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.aoTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.vertexShader, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.fragmentShader, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.metallic, 0.0F);
    EXPECT_EQ(material.roughness, 0.5F);
    EXPECT_FALSE(material.isTransparent);
}

TEST(MaterialAssetUVETest, SaveThenLoad_RoundTripsFieldExact) {
    const std::filesystem::path path = "uve_material_asset_tests_round_trip.uvemat";
    std::filesystem::remove(path);
    const MaterialAssetUVE original = MakeTestMaterialUVE();
    ASSERT_TRUE(SaveMaterialAssetUVE(original, path));

    MaterialAssetUVE loaded;
    ASSERT_TRUE(LoadMaterialAssetUVE(path, loaded));

    EXPECT_EQ(loaded.albedoColor, original.albedoColor);
    EXPECT_EQ(loaded.albedoTexture, original.albedoTexture);
    EXPECT_EQ(loaded.normalTexture, original.normalTexture);
    EXPECT_EQ(loaded.metallic, original.metallic);
    EXPECT_EQ(loaded.roughness, original.roughness);
    EXPECT_EQ(loaded.aoTexture, original.aoTexture);
    EXPECT_EQ(loaded.emissiveColor, original.emissiveColor);
    EXPECT_EQ(loaded.vertexShader, original.vertexShader);
    EXPECT_EQ(loaded.fragmentShader, original.fragmentShader);
    EXPECT_EQ(loaded.isTransparent, original.isTransparent);

    std::filesystem::remove(path);
}

TEST(MaterialAssetUVETest, LoadMaterialAssetUVE_WrongAssetKind_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_material_asset_tests_wrong_kind.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    MaterialAssetUVE material;
    EXPECT_FALSE(LoadMaterialAssetUVE(path, material));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("not a material file") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(MaterialAssetUVETest, LoadMaterialAssetUVE_MissingFile_ReturnsFalse) {
    const std::filesystem::path path = "uve_material_asset_tests_nonexistent.uvemat";
    std::filesystem::remove(path);

    MaterialAssetUVE material;
    EXPECT_FALSE(LoadMaterialAssetUVE(path, material));
}

TEST(MaterialAssetUVETest, LoadMaterialAssetUVE_MissingField_FailsAndLogsError) {
    const std::filesystem::path path = "uve_material_asset_tests_missing_field.uvemat";
    std::filesystem::remove(path);
    const std::string incompleteJson = "{\"albedoColor\": {\"x\": 1.0, \"y\": 1.0, \"z\": 1.0}}";
    const auto* const jsonBytes = reinterpret_cast<const std::byte*>(incompleteJson.data());
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Material,
                                 std::vector<std::byte>(jsonBytes, jsonBytes + incompleteJson.size())));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    MaterialAssetUVE material;
    EXPECT_FALSE(LoadMaterialAssetUVE(path, material));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("missing an expected field") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(MaterialAssetUVETest, EndToEnd_RegisterLoaderThenLoadUVE_ReachesLoadedWithMatchingData) {
    const std::filesystem::path path = "uve_material_asset_tests_end_to_end.uvemat";
    std::filesystem::remove(path);
    const MaterialAssetUVE original = MakeTestMaterialUVE();
    ASSERT_TRUE(SaveMaterialAssetUVE(original, path));

    Threading::ThreadPoolUVE threadPool(2);
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    AssetManagerUVE assetManager(threadPool, eventSystem);
    assetManager.RegisterLoaderUVE<MaterialAssetUVE>(&LoadMaterialAssetUVE);

    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    const AssetHandleUVE<MaterialAssetUVE> handle = assetManager.LoadUVE<MaterialAssetUVE>(guid, assetDatabase);

    bool ready = false;
    for (int iteration = 0; iteration < 200000 && !ready; ++iteration) {
        ready = handle.IsReadyUVE() || handle.HasFailedUVE();
        if (!ready) {
            std::this_thread::yield();
        }
    }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(handle.IsReadyUVE());
    const MaterialAssetUVE* const loaded = handle.TryGetUVE();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->albedoTexture, original.albedoTexture);
    EXPECT_EQ(loaded->isTransparent, original.isTransparent);

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
