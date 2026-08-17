// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_asset_loader_uve.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/file_system_uve.h"

namespace UVE::Scripting::Tests {
namespace {

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void WriteTextUVE(const std::filesystem::path& path, const std::string& text) {
    WriteBytesUVE(path, std::vector<std::byte>(reinterpret_cast<const std::byte*>(text.data()),
                                               reinterpret_cast<const std::byte*>(text.data() + text.size())));
}

[[nodiscard]] ScriptGraphSchemaUVE MakeSchemaUVE(const std::uint32_t nodeId, const std::string& typeId) {
    ScriptGraphSchemaUVE schema;
    EXPECT_TRUE(schema.graph.AddNodeUVE({nodeId, typeId}));
    schema.layout = {{nodeId, 12.0F, 24.0F}};
    schema.metadata = {{"source", typeId}};
    return schema;
}

class ScriptAssetLoaderUVETest : public ::testing::Test {
protected:
    Asset::AssetBundleUVE assetBundle;
    Asset::FileSystemUVE fileSystem{assetBundle};
    const std::filesystem::path fixtureDirectory = "uve_script_asset_loader_tests";

    void SetUp() override {
        std::filesystem::remove_all(fixtureDirectory);
        std::filesystem::create_directories(fixtureDirectory);
        fileSystem.MountDirectoryUVE("", fixtureDirectory, 0);
    }

    void TearDown() override { std::filesystem::remove_all(fixtureDirectory); }
};

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_ReadsVfsPathAndReturnsCopiedSchema) {
    const ScriptGraphSchemaUVE expected = MakeSchemaUVE(7U, "test.source");
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphSchemaUVE(expected, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    WriteTextUVE(fixtureDirectory / "scripts" / "player.uvescript", encoded);

    const Scene::ScriptComponentUVE component{"scripts/player.uvescript"};
    const ScriptAssetLoadResultUVE result = ScriptAssetLoaderUVE::LoadSchemaUVE(component, fileSystem);

    ASSERT_TRUE(result.IsLoadedUVE());
    EXPECT_EQ(result.sourceByteCount, encoded.size());
    EXPECT_TRUE(result.schema.has_value());
    EXPECT_EQ(*result.schema, expected);
}

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_RepeatedLoadReflectsUpdatedVfsContentDeterministically) {
    const ScriptGraphSchemaUVE first = MakeSchemaUVE(1U, "test.first");
    const ScriptGraphSchemaUVE second = MakeSchemaUVE(2U, "test.second");
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string firstEncoded = EncodeScriptGraphSchemaUVE(first, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    WriteTextUVE(fixtureDirectory / "reload.uvescript", firstEncoded);
    const Scene::ScriptComponentUVE component{"reload.uvescript"};

    const ScriptAssetLoadResultUVE firstResult = ScriptAssetLoaderUVE::LoadSchemaUVE(component, fileSystem);
    ASSERT_TRUE(firstResult.IsLoadedUVE());

    diagnostics.clear();
    const std::string secondEncoded = EncodeScriptGraphSchemaUVE(second, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    WriteTextUVE(fixtureDirectory / "reload.uvescript", secondEncoded);
    const ScriptAssetLoadResultUVE secondResult = ScriptAssetLoaderUVE::LoadSchemaUVE(component, fileSystem);

    ASSERT_TRUE(secondResult.IsLoadedUVE());
    EXPECT_EQ(secondResult.sourceByteCount, secondEncoded.size());
    EXPECT_EQ(*secondResult.schema, second);
}

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_EmptyPathIsNoScriptWithoutVfsRead) {
    const ScriptAssetLoadResultUVE result =
        ScriptAssetLoaderUVE::LoadSchemaUVE(Scene::ScriptComponentUVE{}, fileSystem);

    EXPECT_TRUE(result.IsNoScriptUVE());
    EXPECT_EQ(result.sourceByteCount, 0U);
    EXPECT_FALSE(result.schema.has_value());
}

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_RejectsInvalidPathAndMissingFile) {
    const ScriptAssetLoadResultUVE invalid = ScriptAssetLoaderUVE::LoadSchemaUVE(
        Scene::ScriptComponentUVE{"../escape.uvescript"}, fileSystem);
    EXPECT_EQ(invalid.code, ScriptAssetLoadCodeUVE::InvalidPath);

    const ScriptAssetLoadResultUVE missing = ScriptAssetLoaderUVE::LoadSchemaUVE(
        Scene::ScriptComponentUVE{"missing.uvescript"}, fileSystem);
    EXPECT_EQ(missing.code, ScriptAssetLoadCodeUVE::MissingFile);
}

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_RejectsOversizeAndEmbeddedNulBeforeDecode) {
    WriteTextUVE(fixtureDirectory / "large.uvescript", "123456789");
    ScriptGraphPersistenceLimitsUVE limits;
    limits.maximumTextBytes = 4U;
    const ScriptAssetLoadResultUVE oversize = ScriptAssetLoaderUVE::LoadSchemaUVE(
        Scene::ScriptComponentUVE{"large.uvescript"}, fileSystem, limits);
    EXPECT_EQ(oversize.code, ScriptAssetLoadCodeUVE::TextTooLarge);
    EXPECT_EQ(oversize.sourceByteCount, 9U);

    WriteBytesUVE(fixtureDirectory / "nul.uvescript", {std::byte{'{'}, std::byte{0}, std::byte{'}'}});
    const ScriptAssetLoadResultUVE nul = ScriptAssetLoaderUVE::LoadSchemaUVE(
        Scene::ScriptComponentUVE{"nul.uvescript"}, fileSystem);
    EXPECT_EQ(nul.code, ScriptAssetLoadCodeUVE::EmbeddedNul);
}

TEST_F(ScriptAssetLoaderUVETest, LoadSchemaUVE_PropagatesDecodeDiagnosticsForMalformedGraphSchema) {
    WriteTextUVE(fixtureDirectory / "malformed.uvescript", R"({"schemaVersion":1,"nodes":[]})");

    const ScriptAssetLoadResultUVE result = ScriptAssetLoaderUVE::LoadSchemaUVE(
        Scene::ScriptComponentUVE{"malformed.uvescript"}, fileSystem);

    EXPECT_EQ(result.code, ScriptAssetLoadCodeUVE::DecodeRejected);
    EXPECT_FALSE(result.diagnostics.empty());
    EXPECT_FALSE(result.schema.has_value());
}

} // namespace
} // namespace UVE::Scripting::Tests
