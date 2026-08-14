// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_database_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
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

class CurrentWorkingDirectoryGuardUVE final {
public:
    CurrentWorkingDirectoryGuardUVE() : m_originalPath(std::filesystem::current_path()) {}

    ~CurrentWorkingDirectoryGuardUVE() {
        std::error_code errorCode;
        std::filesystem::current_path(m_originalPath, errorCode);
    }

    CurrentWorkingDirectoryGuardUVE(const CurrentWorkingDirectoryGuardUVE&) = delete;
    CurrentWorkingDirectoryGuardUVE& operator=(const CurrentWorkingDirectoryGuardUVE&) = delete;

private:
    std::filesystem::path m_originalPath;
};

TEST(AssetDatabaseUVETest, GetRegisteredAssetsUVE_EmptyDatabase_ReturnsEmptySnapshot) {
    AssetDatabaseUVE database;
    EXPECT_TRUE(database.GetRegisteredAssetsUVE().empty());
}

TEST(AssetDatabaseUVETest, GetRegisteredAssetsUVE_SortsLexicallyThenByGuid) {
    const std::filesystem::path fixturePath = "uve_asset_database_tests_snapshot_sort.uveassetdb";
    WriteFixtureFileUVE(
        fixturePath,
        R"({
            "0000000000000020": "assets/same.uveprefab",
            "0000000000000010": "assets/same.uveprefab",
            "0000000000000030": "assets/zebra.uveprefab",
            "0000000000000040": "assets/apple.uveprefab"
        })");

    AssetDatabaseUVE database;
    ASSERT_TRUE(database.LoadUVE(fixturePath));
    const std::vector<AssetRecordUVE> records = database.GetRegisteredAssetsUVE();

    ASSERT_EQ(records.size(), 4U);
    EXPECT_EQ(records[0].path, std::filesystem::path("assets/apple.uveprefab"));
    EXPECT_EQ(records[0].guid, AssetGuidUVE{0x40U});
    EXPECT_EQ(records[1].path, std::filesystem::path("assets/same.uveprefab"));
    EXPECT_EQ(records[1].guid, AssetGuidUVE{0x10U});
    EXPECT_EQ(records[2].path, std::filesystem::path("assets/same.uveprefab"));
    EXPECT_EQ(records[2].guid, AssetGuidUVE{0x20U});
    EXPECT_EQ(records[3].path, std::filesystem::path("assets/zebra.uveprefab"));
    EXPECT_EQ(records[3].guid, AssetGuidUVE{0x30U});

    std::filesystem::remove(fixturePath);
}

TEST(AssetDatabaseUVETest, GetRegisteredAssetsUVE_ReturnsSnapshotIsolatedFromLaterRegistration) {
    AssetDatabaseUVE database;
    const AssetGuidUVE initialGuid = database.RegisterUVE("assets/initial.uveprefab");
    const std::vector<AssetRecordUVE> snapshot = database.GetRegisteredAssetsUVE();

    static_cast<void>(database.RegisterUVE("assets/later.uveprefab"));

    ASSERT_EQ(snapshot.size(), 1U);
    EXPECT_EQ(snapshot.front().guid, initialGuid);
    EXPECT_EQ(snapshot.front().path, std::filesystem::path("assets/initial.uveprefab"));
    EXPECT_EQ(database.GetRegisteredAssetsUVE().size(), 2U);
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

TEST(AssetDatabaseUVETest, RegisterUVE_EquivalentLexicalPaths_ReturnSameGuidAndNormalizedStoredPath) {
    AssetDatabaseUVE database;
    const AssetGuidUVE first = database.RegisterUVE("meshes/generated/../cube.uvemodel");
    const AssetGuidUVE second = database.RegisterUVE("meshes/cube.uvemodel");

    EXPECT_EQ(first, second);
    EXPECT_EQ(database.ResolveUVE(first), std::filesystem::path("meshes/cube.uvemodel"));
    const std::vector<AssetRecordUVE> records = database.GetRegisteredAssetsUVE();
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records.front().path, std::filesystem::path("meshes/cube.uvemodel"));
}

TEST(AssetDatabaseUVETest, RegisterUVE_StableRelativePath_IsIndependentOfCurrentWorkingDirectory) {
    std::error_code errorCode;
    const std::filesystem::path fixtureRoot =
        std::filesystem::absolute("uve_asset_database_tests_working_directory", errorCode);
    ASSERT_FALSE(errorCode);
    const std::filesystem::path firstWorkingDirectory = fixtureRoot / "first";
    const std::filesystem::path secondWorkingDirectory = fixtureRoot / "second";
    std::filesystem::remove_all(fixtureRoot);
    std::filesystem::create_directories(firstWorkingDirectory);
    std::filesystem::create_directories(secondWorkingDirectory);

    {
        CurrentWorkingDirectoryGuardUVE workingDirectoryGuard;
        errorCode.clear();
        AssetDatabaseUVE database;
        std::filesystem::current_path(firstWorkingDirectory, errorCode);
        ASSERT_FALSE(errorCode);
        const AssetGuidUVE firstGuid = database.RegisterUVE("assets/tree.uveprefab");

        std::filesystem::current_path(secondWorkingDirectory, errorCode);
        ASSERT_FALSE(errorCode);
        const AssetGuidUVE secondGuid = database.RegisterUVE("assets/tree.uveprefab");

        EXPECT_EQ(firstGuid, secondGuid);
        EXPECT_EQ(database.ResolveUVE(firstGuid), std::filesystem::path("assets/tree.uveprefab"));
    }
    std::filesystem::remove_all(fixtureRoot);
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

TEST(AssetDatabaseUVETest, LoadUVE_LegacyEquivalentPathAliases_UsesSmallestGuidForFutureRegistration) {
    const std::filesystem::path fixturePath = "uve_asset_database_tests_legacy_aliases.uveassetdb";
    WriteFixtureFileUVE(
        fixturePath,
        R"({
            "0000000000000020": "assets/generated/../tree.uveprefab",
            "0000000000000010": "assets/tree.uveprefab"
        })");

    AssetDatabaseUVE database;
    ASSERT_TRUE(database.LoadUVE(fixturePath));
    EXPECT_EQ(database.RegisterUVE("assets/tree.uveprefab"), AssetGuidUVE{0x10U});
    EXPECT_EQ(database.RegisterUVE("assets/./tree.uveprefab"), AssetGuidUVE{0x10U});

    const std::vector<AssetRecordUVE> records = database.GetRegisteredAssetsUVE();
    ASSERT_EQ(records.size(), 2U);
    EXPECT_EQ(records[0].path, std::filesystem::path("assets/tree.uveprefab"));
    EXPECT_EQ(records[1].path, std::filesystem::path("assets/tree.uveprefab"));
    std::filesystem::remove(fixturePath);
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
