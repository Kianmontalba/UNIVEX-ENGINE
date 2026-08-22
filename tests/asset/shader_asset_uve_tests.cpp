// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/shader_asset_uve.h"

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

[[nodiscard]] ShaderAssetUVE MakeTestShaderUVE() {
    ShaderAssetUVE shader;
    shader.stage = ShaderStageKindUVE::Fragment;
    shader.sourceCode = "void main() { }";
    shader.entryPointName = "main";
    return shader;
}

TEST(ShaderAssetUVETest, SaveThenLoad_RoundTripsFieldExact) {
    const std::filesystem::path path = "uve_shader_asset_tests_round_trip.uveshader";
    std::filesystem::remove(path);
    const ShaderAssetUVE original = MakeTestShaderUVE();
    ASSERT_TRUE(SaveShaderAssetUVE(original, path));

    ShaderAssetUVE loaded;
    ASSERT_TRUE(LoadShaderAssetUVE(path, loaded));

    EXPECT_EQ(loaded.stage, original.stage);
    EXPECT_EQ(loaded.sourceCode, original.sourceCode);
    EXPECT_EQ(loaded.entryPointName, original.entryPointName);

    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, LoadShaderAssetUVE_WrongAssetKind_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_shader_asset_tests_wrong_kind.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ShaderAssetUVE shader;
    EXPECT_FALSE(LoadShaderAssetUVE(path, shader));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("not a shader file") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, LoadShaderAssetUVE_MissingFile_ReturnsFalse) {
    const std::filesystem::path path = "uve_shader_asset_tests_nonexistent.uveshader";
    std::filesystem::remove(path);

    ShaderAssetUVE shader;
    EXPECT_FALSE(LoadShaderAssetUVE(path, shader));
}

TEST(ShaderAssetUVETest, LoadShaderAssetUVE_EmptySourceCode_FailsAndLogsError) {
    const std::filesystem::path path = "uve_shader_asset_tests_empty_source.uveshader";
    std::filesystem::remove(path);

    ShaderAssetUVE invalidShader = MakeTestShaderUVE();
    invalidShader.sourceCode.clear();
    ASSERT_TRUE(SaveShaderAssetUVE(invalidShader, path));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ShaderAssetUVE loaded;
    EXPECT_FALSE(LoadShaderAssetUVE(path, loaded));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("empty source code") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, LoadShaderAssetUVE_UnknownStageFailsBeforePublication) {
    const std::filesystem::path path = "uve_shader_asset_tests_unknown_stage.uveshader";
    std::filesystem::remove(path);
    const std::string invalidPayload =
        R"({"stage":255,"sourceCode":"void main() { }","entryPointName":"main"})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(invalidPayload.data());
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Shader,
                                std::vector<std::byte>(payloadBytes, payloadBytes + invalidPayload.size())));

    ShaderAssetUVE loaded = MakeTestShaderUVE();
    const ShaderAssetUVE original = loaded;
    EXPECT_FALSE(LoadShaderAssetUVE(path, loaded));
    EXPECT_EQ(loaded.stage, original.stage);
    EXPECT_EQ(loaded.sourceCode, original.sourceCode);
    EXPECT_EQ(loaded.entryPointName, original.entryPointName);
    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, SaveShaderAssetUVE_UnknownStagePreservesExistingEnvelope) {
    const std::filesystem::path path = "uve_shader_asset_tests_unknown_stage_save.uveshader";
    std::filesystem::remove(path);
    const ShaderAssetUVE original = MakeTestShaderUVE();
    ASSERT_TRUE(SaveShaderAssetUVE(original, path));

    ShaderAssetUVE invalid = original;
    invalid.stage = static_cast<ShaderStageKindUVE>(0xFFU);
    EXPECT_FALSE(SaveShaderAssetUVE(invalid, path));

    ShaderAssetUVE loaded;
    ASSERT_TRUE(LoadShaderAssetUVE(path, loaded));
    EXPECT_EQ(loaded.stage, original.stage);
    EXPECT_EQ(loaded.sourceCode, original.sourceCode);
    EXPECT_EQ(loaded.entryPointName, original.entryPointName);
    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, LoadShaderAssetUVE_MalformedJson_FailsAndLogsError) {
    const std::filesystem::path path = "uve_shader_asset_tests_malformed_json.uveshader";
    std::filesystem::remove(path);
    const std::string garbage = "{ not valid json";
    const auto* const garbageBytes = reinterpret_cast<const std::byte*>(garbage.data());
    ASSERT_TRUE(
        WriteUveFileUVE(path, AssetKindUVE::Shader, std::vector<std::byte>(garbageBytes, garbageBytes + garbage.size())));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ShaderAssetUVE shader;
    EXPECT_FALSE(LoadShaderAssetUVE(path, shader));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(ShaderAssetUVETest, EndToEnd_RegisterLoaderThenLoadUVE_ReachesLoadedWithMatchingData) {
    const std::filesystem::path path = "uve_shader_asset_tests_end_to_end.uveshader";
    std::filesystem::remove(path);
    const ShaderAssetUVE original = MakeTestShaderUVE();
    ASSERT_TRUE(SaveShaderAssetUVE(original, path));

    Threading::ThreadPoolUVE threadPool(2);
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    AssetManagerUVE assetManager(threadPool, eventSystem);
    assetManager.RegisterLoaderUVE<ShaderAssetUVE>(&LoadShaderAssetUVE);

    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    const AssetHandleUVE<ShaderAssetUVE> handle = assetManager.LoadUVE<ShaderAssetUVE>(guid, assetDatabase);

    bool ready = false;
    for (int iteration = 0; iteration < 200000 && !ready; ++iteration) {
        ready = handle.IsReadyUVE() || handle.HasFailedUVE();
        if (!ready) {
            std::this_thread::yield();
        }
    }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(handle.IsReadyUVE());
    const ShaderAssetUVE* const loaded = handle.TryGetUVE();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->sourceCode, original.sourceCode);

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
