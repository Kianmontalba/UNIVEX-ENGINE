// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/texture_asset_uve.h"

#include <algorithm>
#include <filesystem>
#include <memory>
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

[[nodiscard]] TextureAssetUVE MakeTestTextureUVE() {
    TextureAssetUVE texture;
    texture.width = 2;
    texture.height = 2;
    texture.format = TextureFormatUVE::RGBA8Unorm;
    texture.pixels.resize(2 * 2 * 4);
    for (std::size_t index = 0; index < texture.pixels.size(); ++index) {
        texture.pixels[index] = static_cast<std::byte>(index);
    }
    return texture;
}

TEST(TextureAssetUVETest, BytesPerPixelUVE_ReturnsExpectedValues) {
    EXPECT_EQ(BytesPerPixelUVE(TextureFormatUVE::RGBA8Unorm), 4U);
    EXPECT_EQ(BytesPerPixelUVE(TextureFormatUVE::RGBA16Float), 8U);
}

TEST(TextureAssetUVETest, SaveThenLoad_RoundTripsByteExact) {
    const std::filesystem::path path = "uve_texture_asset_tests_round_trip.uvetex";
    std::filesystem::remove(path);
    const TextureAssetUVE original = MakeTestTextureUVE();
    ASSERT_TRUE(SaveTextureAssetUVE(original, path));

    TextureAssetUVE loaded;
    ASSERT_TRUE(LoadTextureAssetUVE(path, loaded));

    EXPECT_EQ(loaded.width, original.width);
    EXPECT_EQ(loaded.height, original.height);
    EXPECT_EQ(loaded.format, original.format);
    EXPECT_EQ(loaded.pixels, original.pixels);

    std::filesystem::remove(path);
}

TEST(TextureAssetUVETest, LoadTextureAssetUVE_WrongAssetKind_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_texture_asset_tests_wrong_kind.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    TextureAssetUVE texture;
    EXPECT_FALSE(LoadTextureAssetUVE(path, texture));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("not a texture file") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(TextureAssetUVETest, LoadTextureAssetUVE_MissingFile_ReturnsFalse) {
    const std::filesystem::path path = "uve_texture_asset_tests_nonexistent.uvetex";
    std::filesystem::remove(path);

    TextureAssetUVE texture;
    EXPECT_FALSE(LoadTextureAssetUVE(path, texture));
}

TEST(TextureAssetUVETest, LoadTextureAssetUVE_PixelByteCountMismatch_FailsAndLogsError) {
    const std::filesystem::path path = "uve_texture_asset_tests_bad_pixel_count.uvetex";
    std::filesystem::remove(path);

    TextureAssetUVE invalidTexture = MakeTestTextureUVE();
    invalidTexture.pixels.resize(3); // 2x2 RGBA8Unorm expects 16 bytes, not 3
    ASSERT_TRUE(SaveTextureAssetUVE(invalidTexture, path));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    TextureAssetUVE loaded;
    EXPECT_FALSE(LoadTextureAssetUVE(path, loaded));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("pixel bytes") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(TextureAssetUVETest, EndToEnd_RegisterLoaderThenLoadUVE_ReachesLoadedWithMatchingData) {
    const std::filesystem::path path = "uve_texture_asset_tests_end_to_end.uvetex";
    std::filesystem::remove(path);
    const TextureAssetUVE original = MakeTestTextureUVE();
    ASSERT_TRUE(SaveTextureAssetUVE(original, path));

    Threading::ThreadPoolUVE threadPool(2);
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    AssetManagerUVE assetManager(threadPool, eventSystem);
    assetManager.RegisterLoaderUVE<TextureAssetUVE>(&LoadTextureAssetUVE);

    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    const AssetHandleUVE<TextureAssetUVE> handle = assetManager.LoadUVE<TextureAssetUVE>(guid, assetDatabase);

    bool ready = false;
    for (int iteration = 0; iteration < 200000 && !ready; ++iteration) {
        ready = handle.IsReadyUVE() || handle.HasFailedUVE();
        if (!ready) {
            std::this_thread::yield();
        }
    }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(handle.IsReadyUVE());
    const TextureAssetUVE* const loaded = handle.TryGetUVE();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->pixels, original.pixels);

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
