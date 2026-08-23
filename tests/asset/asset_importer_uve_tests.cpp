// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
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

class RejectingAssetDatabaseUVE final : public IAssetDatabaseUVE {
public:
    bool LoadUVE(const std::filesystem::path&) override { return false; }
    bool SaveUVE() override { return false; }
    bool SaveUVE(const std::filesystem::path&) override { return false; }
    [[nodiscard]] AssetGuidUVE RegisterUVE(const std::filesystem::path&) override {
        ++registerCount;
        return kInvalidAssetGuidUVE;
    }
    [[nodiscard]] std::filesystem::path ResolveUVE(AssetGuidUVE) const override { return {}; }
    [[nodiscard]] bool HasGuidUVE(AssetGuidUVE) const override { return false; }
    [[nodiscard]] std::vector<AssetRecordUVE> GetRegisteredAssetsUVE() const override { return {}; }

    int registerCount = 0;
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

    constexpr std::array<ClassificationCaseUVE, 9> kCases = {{
        {"Readme.TXT", AssetImportSourceKindUVE::PlainText, "txt", true, false,
         "built-in text parser is registered"},
        {"Character.UVEMODEL", AssetImportSourceKindUVE::MeshEnvelope, "uvemodel", true, false,
         "built-in generic copy importer is registered"},
        {"Character.FBX", AssetImportSourceKindUVE::RawModel, "fbx", false, true,
         "format-specific parser is not registered"},
        {"Character.OBJ", AssetImportSourceKindUVE::RawModel, "obj", true, true,
         "format-specific parser is registered"},
        {"Albedo.PNG", AssetImportSourceKindUVE::RawTexture, "png", true, true,
         "format-specific parser is registered"},
        {"Albedo.BMP", AssetImportSourceKindUVE::RawTexture, "bmp", true, true,
         "format-specific parser is registered"},
        {"Surface.MTL", AssetImportSourceKindUVE::RawMaterial, "mtl", true, true,
         "format-specific parser is registered"},
        {"Music.OGG", AssetImportSourceKindUVE::RawAudio, "ogg", false, true,
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

TEST_F(AssetImporterUVETest, ImportUVE_RegistrationFailureReturnsInvalidAfterSuccessfulConversion) {
    const std::filesystem::path sourcePath = "uve_asset_importer_registration_failure_source.custom";
    const std::filesystem::path destinationPath = "uve_asset_importer_registration_failure_dest.custom";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "converted before registration failure");

    importer.RegisterImporterUVE("custom", [](const std::filesystem::path& source,
                                                const std::filesystem::path& destination,
                                                const AssetImportSettingsUVE&) {
        std::ifstream input(source, std::ios::binary);
        std::ofstream output(destination, std::ios::binary | std::ios::trunc);
        output << input.rdbuf();
        return input.good() && output.good();
    });
    RejectingAssetDatabaseUVE rejectingDatabase;

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, rejectingDatabase);

    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(rejectingDatabase.registerCount, 1);
    EXPECT_TRUE(std::filesystem::exists(destinationPath));
    EXPECT_EQ(ReadFileUVE(destinationPath), "converted before registration failure");

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
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

TEST_F(AssetImporterUVETest, TextImportUVE_PreservesTextVerbatim) {
    const std::filesystem::path sourcePath = "uve_text_parser_preserve_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_preserve_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    const std::string source = "first\r\nsecond\rthird\nfourth";
    WriteFixtureFileUVE(sourcePath, source);

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(ReadFileUVE(destinationPath), source);
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, TextImportUVE_RejectsNulByte) {
    const std::filesystem::path sourcePath = "uve_text_parser_nul_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_nul_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, std::string{"prefix\0suffix", 13U});

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);

    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, TextImportUVE_NormalizesLineEndingsToLF) {
    const std::filesystem::path sourcePath = "uve_text_parser_lf_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_lf_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "first\r\nsecond\rthird\nfourth");
    TextImportSettingsUVE settings;
    settings.lineEnding = TextImportLineEndingUVE::LineFeed;

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase, settings);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(ReadFileUVE(destinationPath), "first\nsecond\nthird\nfourth");
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, TextImportUVE_NormalizesLineEndingsToCRLF) {
    const std::filesystem::path sourcePath = "uve_text_parser_crlf_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_crlf_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "first\nsecond\rthird\r\nfourth");
    TextImportSettingsUVE settings;
    settings.lineEnding = TextImportLineEndingUVE::CarriageReturnLineFeed;

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase, settings);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(ReadFileUVE(destinationPath), "first\r\nsecond\r\nthird\r\nfourth");
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, TextImportUVE_EnsuresTrailingNewline) {
    const std::filesystem::path sourcePath = "uve_text_parser_trailing_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_trailing_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "text without final newline");
    TextImportSettingsUVE settings;
    settings.lineEnding = TextImportLineEndingUVE::LineFeed;
    settings.ensureTrailingLineEnding = true;

    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase, settings);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(ReadFileUVE(destinationPath), "text without final newline\n");
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, TextImportUVE_RejectsOverCapSourceBeforePublish) {
    const std::filesystem::path sourcePath = "uve_text_parser_oversized_source.txt";
    const std::filesystem::path destinationPath = "uve_text_parser_oversized_destination.txt";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    const std::string oversized(kMaximumTextImportBytesUVE + 1U, 'T');
    WriteFixtureFileUVE(sourcePath, oversized);

    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, assetDatabase), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(AssetImportSettingsUVETest, TextImportSettingsUVE_CacheVersionIsStable) {
    TextImportSettingsUVE preserve;
    TextImportSettingsUVE same = preserve;
    TextImportSettingsUVE normalized = preserve;
    normalized.lineEnding = TextImportLineEndingUVE::LineFeed;
    TextImportSettingsUVE trailing = normalized;
    trailing.ensureTrailingLineEnding = true;

    EXPECT_EQ(preserve.GetCacheVersionUVE(), "text-v1;line-ending=preserve;trailing-newline=false");
    EXPECT_EQ(preserve.GetCacheVersionUVE(), same.GetCacheVersionUVE());
    EXPECT_NE(preserve.GetCacheVersionUVE(), normalized.GetCacheVersionUVE());
    EXPECT_NE(normalized.GetCacheVersionUVE(), trailing.GetCacheVersionUVE());
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

TEST_F(AssetImporterUVETest, RegisterImporterUVE_InvalidRegistrationPreservesExistingImporter) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_invalid_registration.custom";
    const std::filesystem::path destinationPath = "uve_asset_importer_tests_invalid_registration_dest.custom";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "source");

    bool validImporterRan = false;
    const auto validImporter = [&validImporterRan](const std::filesystem::path&,
                                                     const std::filesystem::path& destination,
                                                     const AssetImportSettingsUVE&) {
        validImporterRan = true;
        std::ofstream file(destination, std::ios::binary);
        file << "valid importer";
        return file.good();
    };
    importer.RegisterImporterUVE("custom", validImporter);
    importer.RegisterImporterUVE("", {});
    importer.RegisterImporterUVE("custom", {});

    const AssetImportSourceClassificationUVE noExtension = importer.ClassifySourceUVE("asset");
    EXPECT_FALSE(noExtension.importerRegistered);
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);
    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(validImporterRan);
    EXPECT_EQ(ReadFileUVE(destinationPath), "valid importer");

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
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

TEST_F(AssetImporterUVETest, ImportUVE_ImporterExceptionReturnsInvalidWithoutDatabaseRegistration) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.throwing";
    const std::filesystem::path destinationPath = "uve_asset_importer_tests_dest.throwing";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "source");

    class ThrowingAssetDatabaseUVE final : public IAssetDatabaseUVE {
    public:
        bool LoadUVE(const std::filesystem::path&) override { return false; }
        bool SaveUVE() override { return false; }
        bool SaveUVE(const std::filesystem::path&) override { return false; }
        [[nodiscard]] AssetGuidUVE RegisterUVE(const std::filesystem::path&) override {
            registered = true;
            return AssetGuidUVE{777U};
        }
        [[nodiscard]] std::filesystem::path ResolveUVE(AssetGuidUVE) const override { return {}; }
        [[nodiscard]] bool HasGuidUVE(AssetGuidUVE) const override { return false; }
        [[nodiscard]] std::vector<AssetRecordUVE> GetRegisteredAssetsUVE() const override { return {}; }
        bool registered = false;
    } database;

    importer.RegisterImporterUVE("throwing", [](const std::filesystem::path&, const std::filesystem::path&,
                                                  const AssetImportSettingsUVE&) -> bool {
        throw std::runtime_error("injected importer exception");
    });
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, database), kInvalidAssetGuidUVE);
    EXPECT_FALSE(database.registered);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST_F(AssetImporterUVETest, ImportUVE_DatabaseRegistrationExceptionReturnsInvalidWithoutEscaping) {
    const std::filesystem::path sourcePath = "uve_asset_importer_tests_source.database_throwing";
    const std::filesystem::path destinationPath = "uve_asset_importer_tests_dest.database_throwing";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteFixtureFileUVE(sourcePath, "source");

    class ThrowingRegistrationDatabaseUVE final : public IAssetDatabaseUVE {
    public:
        bool LoadUVE(const std::filesystem::path&) override { return false; }
        bool SaveUVE() override { return false; }
        bool SaveUVE(const std::filesystem::path&) override { return false; }
        [[nodiscard]] AssetGuidUVE RegisterUVE(const std::filesystem::path&) override {
            throw std::runtime_error("injected registration exception");
        }
        [[nodiscard]] std::filesystem::path ResolveUVE(AssetGuidUVE) const override { return {}; }
        [[nodiscard]] bool HasGuidUVE(AssetGuidUVE) const override { return false; }
        [[nodiscard]] std::vector<AssetRecordUVE> GetRegisteredAssetsUVE() const override { return {}; }
    } database;

    importer.RegisterImporterUVE("database_throwing", [](const std::filesystem::path&,
                                                           const std::filesystem::path& destination,
                                                           const AssetImportSettingsUVE&) -> bool {
        std::ofstream file(destination, std::ios::binary | std::ios::trunc);
        file << "converted before registration failure";
        return file.good();
    });

    AssetGuidUVE guid = kInvalidAssetGuidUVE;
    EXPECT_NO_THROW(guid = importer.ImportUVE(sourcePath, destinationPath, database));
    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(std::filesystem::exists(destinationPath));

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
