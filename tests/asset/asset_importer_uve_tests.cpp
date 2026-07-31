//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_importer_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << contents;
}

[[nodiscard]] std::string ReadFileUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

class AssetImporterUVETest : public ::testing::Test {
protected:
    AssetImporterUVE importer;
    AssetDatabaseUVE assetDatabase;
};

TEST_F(AssetImporterUVETest, ImportUVE_GenericImporter_CopiesFileAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.txt";
    const std::filesystem::path destinationPath = "uve_asset_importer_tests_dest.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "hello importer");

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);
    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(std::filesystem::exists(destinationPath));
    EXPECT_EQ(ReadFileUVE(destinationPath), "hello importer");
    EXPECT_EQ(assetDatabase.ResolveUVE(guid), destinationPath);

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, ImportUVE_UnregisteredExtension_ReturnsInvalidAndLogsError) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.unknownext";
    std::filesystem::remove(sourcePath);
    WriteFixtureFileUVE(sourcePath, "data");

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, "uve_asset_importer_tests_dest.unknownext", assetDatabase);
    EXPECT_EQ(guid, kInvalidAssetGuidUVE);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("no importer registered") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(sourcePath);
}

TEST_F(AssetImporterUVETest, RegisterImporterUVE_CustomExtension_IsPickedOverGeneric) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.custom";
    const std::filesystem::path destinationPath = "uve_asset_importer_tests_dest.custom";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "ignored by custom importer");

    bool customImporterRan = false;
    importer.RegisterImporterUVE("custom", [&customImporterRan](const std::filesystem::path&,
                                                                  const std::filesystem::path& destination,
                                                                  const AssetImportSettingsUVE&) {
        customImporterRan = true;
        std::ofstream file(destination, std::ios::binary);
        file << "written by custom importer";
        return file.good();
    });

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);
    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(customImporterRan);
    EXPECT_EQ(ReadFileUVE(destinationPath), "written by custom importer");

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, ImportUVE_ImporterReportsFailure_ReturnsInvalidAndLogsError) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.failing";
    std::filesystem::remove(sourcePath);
    WriteFixtureFileUVE(sourcePath, "data");

    importer.RegisterImporterUVE("failing", [](const std::filesystem::path&, const std::filesystem::path&,
                                                 const AssetImportSettingsUVE&) { return false; });

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const AssetGuidUVE guid =
        importer.ImportUVE(sourcePath, "uve_asset_importer_tests_dest.failing", assetDatabase);
    EXPECT_EQ(guid, kInvalidAssetGuidUVE);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("import failed") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(sourcePath);
}

} // namespace
} // namespace UVE::Asset::Tests
