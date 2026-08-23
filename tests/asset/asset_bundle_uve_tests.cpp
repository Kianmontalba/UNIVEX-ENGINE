// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_bundle_uve.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/uve_file_envelope_uve.h"
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

[[nodiscard]] std::string ToHexStringUVE(std::uint64_t value) {
    char buffer[17];
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
    return std::string(buffer);
}

TEST(AssetBundleUVETest, PackThenUnpack_RoundTripsThreeFilesByteExact) {
    const std::filesystem::path sourceA = "uve_asset_bundle_tests_a.txt";
    const std::filesystem::path sourceB = "uve_asset_bundle_tests_b.txt";
    const std::filesystem::path sourceC = "uve_asset_bundle_tests_c.txt";
    WriteFixtureFileUVE(sourceA, "contents of A");
    WriteFixtureFileUVE(sourceB, "contents of B, a bit longer");
    WriteFixtureFileUVE(sourceC, "");

    const AssetGuidUVE guidA{1001};
    const AssetGuidUVE guidB{1002};
    const AssetGuidUVE guidC{1003};

    const std::filesystem::path bundlePath = "uve_asset_bundle_tests.uvebundle";
    std::filesystem::remove(bundlePath);

    AssetBundleUVE bundle;
    ASSERT_TRUE(
        bundle.PackUVE({{guidA, sourceA, ""}, {guidB, sourceB, ""}, {guidC, sourceC, ""}}, bundlePath));

    const std::filesystem::path outputDirectory = "uve_asset_bundle_tests_output";
    std::filesystem::remove_all(outputDirectory);
    ASSERT_TRUE(bundle.UnpackUVE(bundlePath, outputDirectory));

    EXPECT_EQ(ReadFileUVE(outputDirectory / ToHexStringUVE(guidA.value)), "contents of A");
    EXPECT_EQ(ReadFileUVE(outputDirectory / ToHexStringUVE(guidB.value)), "contents of B, a bit longer");
    EXPECT_EQ(ReadFileUVE(outputDirectory / ToHexStringUVE(guidC.value)), "");

    std::filesystem::remove(sourceA);
    std::filesystem::remove(sourceB);
    std::filesystem::remove(sourceC);
    std::filesystem::remove(bundlePath);
    std::filesystem::remove_all(outputDirectory);
}

TEST(AssetBundleUVETest, PackUVE_MissingSourceFile_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    AssetBundleUVE bundle;
    const std::filesystem::path bundlePath = "uve_asset_bundle_tests_missing_source.uvebundle";
    std::filesystem::remove(bundlePath);
    EXPECT_FALSE(bundle.PackUVE({{AssetGuidUVE{1}, "uve_asset_bundle_tests_nonexistent.txt", ""}}, bundlePath));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(AssetBundleUVETest, PackUVE_TraversalVirtualName_ReturnsFalse) {
    const std::filesystem::path source = "uve_asset_bundle_tests_unsafe_name.txt";
    const std::filesystem::path bundlePath = "uve_asset_bundle_tests_unsafe_name.uvebundle";
    WriteFixtureFileUVE(source, "data");
    std::filesystem::remove(bundlePath);

    AssetBundleUVE bundle;
    EXPECT_FALSE(bundle.PackUVE({{AssetGuidUVE{99}, source, "../outside.txt"}}, bundlePath));
    EXPECT_FALSE(bundle.PackUVE({{AssetGuidUVE{100}, source, "..\\\\outside.txt"}}, bundlePath));
    EXPECT_FALSE(std::filesystem::exists(bundlePath));

    std::filesystem::remove(source);
}

TEST(AssetBundleUVETest, PackAndScan_RejectUnboundedEntryCountsAndNames) {
    AssetBundleUVE bundle;
    const std::filesystem::path source = "uve_asset_bundle_tests_bounds.txt";
    const std::filesystem::path bundlePath = "uve_asset_bundle_tests_bounds.uvebundle";
    WriteFixtureFileUVE(source, "data");
    std::filesystem::remove(bundlePath);

    std::vector<AssetBundleEntryUVE> tooManyEntries(kMaximumAssetBundleEntriesUVE + 1U);
    EXPECT_FALSE(bundle.PackUVE(tooManyEntries, bundlePath));
    EXPECT_FALSE(std::filesystem::exists(bundlePath));

    const std::string oversizedName(kMaximumAssetBundleEntryNameBytesUVE + 1U, 'N');
    EXPECT_FALSE(bundle.PackUVE({{AssetGuidUVE{101}, source, oversizedName}}, bundlePath));
    EXPECT_FALSE(std::filesystem::exists(bundlePath));
    const std::string nulName = std::string("valid") + '\0' + "hidden";
    EXPECT_FALSE(bundle.PackUVE({{AssetGuidUVE{102}, source, nulName}}, bundlePath));
    EXPECT_FALSE(std::filesystem::exists(bundlePath));

    const std::uint32_t invalidEntryCount =
        static_cast<std::uint32_t>(kMaximumAssetBundleEntriesUVE + 1U);
    const auto* const countBytes = reinterpret_cast<const std::byte*>(&invalidEntryCount);
    const std::vector<std::byte> malformedPayload(countBytes, countBytes + sizeof(invalidEntryCount));
    ASSERT_TRUE(WriteUveFileUVE(bundlePath, AssetKindUVE::Bundle, malformedPayload));
    EXPECT_FALSE(bundle.HasEntryUVE(bundlePath, "anything"));

    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST(AssetBundleUVETest, UnpackUVE_NonBundleAssetType_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_asset_bundle_tests_not_a_bundle.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    AssetBundleUVE bundle;
    EXPECT_FALSE(bundle.UnpackUVE(path, "uve_asset_bundle_tests_not_a_bundle_output"));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("not a bundle") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(AssetBundleUVETest, HasEntryAndReadEntry_FindEntryByExplicitVirtualName) {
    const std::filesystem::path source = "uve_asset_bundle_tests_virtual_name.txt";
    WriteFixtureFileUVE(source, "found by virtual name");

    const std::filesystem::path bundlePath = "uve_asset_bundle_tests_virtual_name.uvebundle";
    std::filesystem::remove(bundlePath);

    AssetBundleUVE bundle;
    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{2001};
    entry.sourcePath = source;
    entry.virtualName = "textures/rock.uvetex";
    ASSERT_TRUE(bundle.PackUVE({entry}, bundlePath));

    EXPECT_TRUE(bundle.HasEntryUVE(bundlePath, "textures/rock.uvetex"));
    EXPECT_FALSE(bundle.HasEntryUVE(bundlePath, "textures/other.uvetex"));

    const std::optional<std::vector<std::byte>> data = bundle.ReadEntryUVE(bundlePath, "textures/rock.uvetex");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(data->data()), data->size()), "found by virtual name");

    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST(AssetBundleUVETest, ReadEntryUVE_UnknownName_ReturnsNulloptAndLogsError) {
    const std::filesystem::path source = "uve_asset_bundle_tests_unknown_entry.txt";
    WriteFixtureFileUVE(source, "data");
    const std::filesystem::path bundlePath = "uve_asset_bundle_tests_unknown_entry.uvebundle";
    std::filesystem::remove(bundlePath);

    AssetBundleUVE bundle;
    ASSERT_TRUE(bundle.PackUVE({{AssetGuidUVE{3001}, source, ""}}, bundlePath));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    EXPECT_FALSE(bundle.ReadEntryUVE(bundlePath, "no-such-entry").has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("no entry named") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST(AssetBundleUVETest, ReadEntryUVE_NonBundleAssetType_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_asset_bundle_tests_read_entry_not_a_bundle.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    AssetBundleUVE bundle;
    EXPECT_FALSE(bundle.HasEntryUVE(path, "anything"));
    EXPECT_FALSE(bundle.ReadEntryUVE(path, "anything").has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("not a bundle") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
