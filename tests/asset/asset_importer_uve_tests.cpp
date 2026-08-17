// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
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

TEST_F(AssetImporterUVETest, ClassifySourceUVE_ReportsAuthorityAndRawParserBoundary) {
    struct ClassificationCaseUVE {
        std::string_view path;
        AssetImportSourceKindUVE kind;
        std::string_view normalizedExtension;
        bool importerRegistered;
        bool requiresFormatSpecificParser;
        std::string_view diagnostic;
    };

    constexpr std::array<ClassificationCaseUVE, 6> kCases = {{
        {"Readme.TXT", AssetImportSourceKindUVE::PlainText, "txt", true, false,
         "built-in generic copy importer is registered"},
        {"Character.UVEMODEL", AssetImportSourceKindUVE::MeshEnvelope, "uvemodel", true, false,
         "built-in generic copy importer is registered"},
        {"Character.FBX", AssetImportSourceKindUVE::RawModel, "fbx", false, true,
         "format-specific parser is not registered"},
        {"Albedo.PNG", AssetImportSourceKindUVE::RawTexture, "png", false, true,
         "format-specific parser is not registered"},
        {"Surface.MTL", AssetImportSourceKindUVE::RawMaterial, "mtl", false, true,
         "format-specific parser is not registered"},
        {"Notes.UNKNOWN", AssetImportSourceKindUVE::Unknown, "unknown", false, false,
         "unsupported source extension"},
    }};

    for (const ClassificationCaseUVE& expected : kCases) {
        const AssetImportSourceClassificationUVE actual = importer.ClassifySourceUVE(expected.path);
        EXPECT_EQ(actual.kind, expected.kind) << expected.path;
        EXPECT_EQ(actual.normalizedExtension, expected.normalizedExtension) << expected.path;
        EXPECT_EQ(actual.importerRegistered, expected.importerRegistered) << expected.path;
        EXPECT_EQ(actual.requiresFormatSpecificParser, expected.requiresFormatSpecificParser) << expected.path;
        EXPECT_EQ(actual.diagnostic, expected.diagnostic) << expected.path;
    }

    importer.RegisterImporterUVE("custom", [](const std::filesystem::path&, const std::filesystem::path&,
                                               const AssetImportSettingsUVE&) { return true; });
    const AssetImportSourceClassificationUVE custom = importer.ClassifySourceUVE("asset.custom");
    EXPECT_EQ(custom.kind, AssetImportSourceKindUVE::Unknown);
    EXPECT_TRUE(custom.importerRegistered);
    EXPECT_FALSE(custom.requiresFormatSpecificParser);
    EXPECT_EQ(custom.diagnostic, "custom importer is registered without built-in classification");
}

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

TEST_F(AssetImporterUVETest, ImportUVE_TypedUVEEnvelopeExtensions_CopyAndRegisterGuid) {
    constexpr std::array<std::string_view, 4> kTypedEnvelopeExtensions = {
        ".uvemodel", ".uvetex", ".uveshader", ".uvemat"};

    for (const std::string_view extension : kTypedEnvelopeExtensions) {
        const std::filesystem::path sourcePath =
            std::string("uve_asset_importer_typed_source") + std::string(extension);
        const std::filesystem::path destinationPath =
            std::string("uve_asset_importer_typed_dest") + std::string(extension);
        std::filesystem::remove(sourcePath);
        std::filesystem::remove(destinationPath);
        WriteFixtureFileUVE(sourcePath, "typed UVE envelope import");

        const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);
        ASSERT_NE(guid, kInvalidAssetGuidUVE) << extension;
        EXPECT_TRUE(std::filesystem::exists(destinationPath));
        EXPECT_EQ(ReadFileUVE(destinationPath), "typed UVE envelope import");
        EXPECT_EQ(assetDatabase.ResolveUVE(guid), destinationPath);

        std::filesystem::remove(sourcePath);
        std::filesystem::remove(destinationPath);
    }
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
