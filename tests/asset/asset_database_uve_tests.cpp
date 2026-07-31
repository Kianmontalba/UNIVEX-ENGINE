//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_database_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream file(path);
    ASSERT_TRUE(file.is_open());
    file << contents;
}

TEST(AssetDatabaseUVETest, RegisterUVE_SamePathTwice_ReturnsSameGuid) {
    AssetDatabaseUVE database;
    const AssetGuidUVE first = database.RegisterUVE("meshes/cube.uvemodel");
    const AssetGuidUVE second = database.RegisterUVE("meshes/cube.uvemodel");
    EXPECT_EQ(first, second);
}

TEST(AssetDatabaseUVETest, RegisterUVE_DifferentPaths_ReturnDifferentGuids) {
    AssetDatabaseUVE database;
    const AssetGuidUVE first = database.RegisterUVE("meshes/cube.uvemodel");
    const AssetGuidUVE second = database.RegisterUVE("meshes/sphere.uvemodel");
    EXPECT_NE(first, second);
}

TEST(AssetDatabaseUVETest, ResolveUVE_RoundTripsRegisteredPath) {
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = database.RegisterUVE("prefabs/tree.uveprefab");
    EXPECT_EQ(database.ResolveUVE(guid), std::filesystem::path("prefabs/tree.uveprefab"));
}

TEST(AssetDatabaseUVETest, ResolveUVE_UnknownGuid_ReturnsEmptyPath) {
    AssetDatabaseUVE database;
    EXPECT_TRUE(database.ResolveUVE(AssetGuidUVE{12345}).empty());
}

TEST(AssetDatabaseUVETest, HasGuidUVE_TrueForRegisteredFalseOtherwise) {
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = database.RegisterUVE("prefabs/rock.uveprefab");
    EXPECT_TRUE(database.HasGuidUVE(guid));
    EXPECT_FALSE(database.HasGuidUVE(AssetGuidUVE{999}));
    EXPECT_FALSE(database.HasGuidUVE(kInvalidAssetGuidUVE));
}

TEST(AssetDatabaseUVETest, SaveThenLoad_RoundTripsThroughDisk) {
    const std::filesystem::path savePath = "uve_asset_database_tests_roundtrip.uveassetdb";
    std::filesystem::remove(savePath);

    AssetGuidUVE guid{};
    {
        AssetDatabaseUVE writer;
        guid = writer.RegisterUVE("prefabs/tree.uveprefab");
        ASSERT_TRUE(writer.SaveUVE(savePath));
    }

    AssetDatabaseUVE reader;
    ASSERT_TRUE(reader.LoadUVE(savePath));
    EXPECT_EQ(reader.ResolveUVE(guid), std::filesystem::path("prefabs/tree.uveprefab"));

    std::filesystem::remove(savePath);
}

TEST(AssetDatabaseUVETest, LoadUVE_MissingFile_LogsWarning) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    AssetDatabaseUVE database;
    EXPECT_FALSE(database.LoadUVE("uve_asset_database_tests_nonexistent.uveassetdb"));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundWarning =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Warning &&
                   message.message.find("not found") != std::string::npos;
        });
    EXPECT_TRUE(foundWarning);

    logger.Shutdown();
}

TEST(AssetDatabaseUVETest, LoadUVE_MalformedJson_ReturnsFalseAndLogsError) {
    const std::filesystem::path fixturePath = "uve_asset_database_tests_malformed.uveassetdb";
    WriteFixtureFileUVE(fixturePath, "{ not valid json");

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    AssetDatabaseUVE database;
    EXPECT_FALSE(database.LoadUVE(fixturePath));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundParseError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("failed to parse") != std::string::npos;
        });
    EXPECT_TRUE(foundParseError);

    logger.Shutdown();
    std::filesystem::remove(fixturePath);
}

} // namespace
} // namespace UVE::Asset::Tests
