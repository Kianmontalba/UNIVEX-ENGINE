// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/file_system_uve.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << contents;
}

[[nodiscard]] std::string ReadRealFileAsStringUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

[[nodiscard]] std::string ToStringUVE(const std::vector<std::byte>& bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

[[nodiscard]] std::vector<std::byte> ToBytesUVE(std::string_view text) {
    const auto* const bytes = reinterpret_cast<const std::byte*>(text.data());
    return std::vector<std::byte>(bytes, bytes + text.size());
}

class FileSystemUVETest : public ::testing::Test {
protected:
    AssetBundleUVE assetBundle;
    FileSystemUVE fileSystem{assetBundle};
};

TEST_F(FileSystemUVETest, ReadFileUVE_DirectoryMount_ReturnsRealFileBytes) {
    const std::filesystem::path dir = "uve_file_system_tests_basic";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    WriteFixtureFileUVE(dir / "hello.txt", "hello vfs");

    fileSystem.MountDirectoryUVE("", dir, 0);

    const std::optional<std::vector<std::byte>> data = fileSystem.ReadFileUVE("hello.txt");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(ToStringUVE(*data), "hello vfs");

    std::filesystem::remove_all(dir);
}

TEST_F(FileSystemUVETest, ReadFileUVE_TwoDirectoryMountsSamePrefix_HigherPriorityWins) {
    const std::filesystem::path lowDir = "uve_file_system_tests_low";
    const std::filesystem::path highDir = "uve_file_system_tests_high";
    std::filesystem::remove_all(lowDir);
    std::filesystem::remove_all(highDir);
    std::filesystem::create_directories(lowDir);
    std::filesystem::create_directories(highDir);
    WriteFixtureFileUVE(lowDir / "a.txt", "low priority content");
    WriteFixtureFileUVE(highDir / "a.txt", "high priority content");

    fileSystem.MountDirectoryUVE("", lowDir, 0);
    fileSystem.MountDirectoryUVE("", highDir, 10);

    const std::optional<std::vector<std::byte>> data = fileSystem.ReadFileUVE("a.txt");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(ToStringUVE(*data), "high priority content");

    std::filesystem::remove_all(lowDir);
    std::filesystem::remove_all(highDir);
}

TEST_F(FileSystemUVETest, HasFileUVE_PrefixMatchesOnSegmentBoundaryOnly) {
    const std::filesystem::path dir = "uve_file_system_tests_tex";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    WriteFixtureFileUVE(dir / "rock.uvetex", "data");

    fileSystem.MountDirectoryUVE("tex", dir, 0);

    EXPECT_FALSE(fileSystem.HasFileUVE("textures/rock.uvetex")); // "tex" must not match "textures/..."
    EXPECT_TRUE(fileSystem.HasFileUVE("tex/rock.uvetex"));

    std::filesystem::remove_all(dir);
}

TEST_F(FileSystemUVETest, ReadFileUVE_BundleMount_ReturnsBundledBytes) {
    const std::filesystem::path source = "uve_file_system_tests_bundle_source.txt";
    WriteFixtureFileUVE(source, "bundled content");
    const std::filesystem::path bundlePath = "uve_file_system_tests_basic.uvebundle";
    std::filesystem::remove(bundlePath);

    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{4001};
    entry.sourcePath = source;
    entry.virtualName = "meshes/cube.uvemodel";
    ASSERT_TRUE(assetBundle.PackUVE({entry}, bundlePath));

    fileSystem.MountBundleUVE("", bundlePath, 0);

    const std::optional<std::vector<std::byte>> data = fileSystem.ReadFileUVE("meshes/cube.uvemodel");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(ToStringUVE(*data), "bundled content");

    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST_F(FileSystemUVETest, ReadFileUVE_DirectoryMountOverridesBundleMount_WhenHigherPriority) {
    const std::filesystem::path looseDir = "uve_file_system_tests_override_loose";
    std::filesystem::remove_all(looseDir);
    std::filesystem::create_directories(looseDir);
    WriteFixtureFileUVE(looseDir / "shared.txt", "loose override");

    const std::filesystem::path bundleSource = "uve_file_system_tests_override_bundle_source.txt";
    WriteFixtureFileUVE(bundleSource, "bundle content");
    const std::filesystem::path bundlePath = "uve_file_system_tests_override.uvebundle";
    std::filesystem::remove(bundlePath);
    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{5001};
    entry.sourcePath = bundleSource;
    entry.virtualName = "shared.txt";
    ASSERT_TRUE(assetBundle.PackUVE({entry}, bundlePath));

    fileSystem.MountBundleUVE("", bundlePath, 0);
    fileSystem.MountDirectoryUVE("", looseDir, 10);

    const std::optional<std::vector<std::byte>> data = fileSystem.ReadFileUVE("shared.txt");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(ToStringUVE(*data), "loose override");

    std::filesystem::remove_all(looseDir);
    std::filesystem::remove(bundleSource);
    std::filesystem::remove(bundlePath);
}

TEST_F(FileSystemUVETest, ReadFileUVE_BundleMountOverridesDirectoryMount_WhenHigherPriority) {
    const std::filesystem::path looseDir = "uve_file_system_tests_underride_loose";
    std::filesystem::remove_all(looseDir);
    std::filesystem::create_directories(looseDir);
    WriteFixtureFileUVE(looseDir / "shared.txt", "loose content");

    const std::filesystem::path bundleSource = "uve_file_system_tests_underride_bundle_source.txt";
    WriteFixtureFileUVE(bundleSource, "bundle wins content");
    const std::filesystem::path bundlePath = "uve_file_system_tests_underride.uvebundle";
    std::filesystem::remove(bundlePath);
    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{5002};
    entry.sourcePath = bundleSource;
    entry.virtualName = "shared.txt";
    ASSERT_TRUE(assetBundle.PackUVE({entry}, bundlePath));

    fileSystem.MountDirectoryUVE("", looseDir, 0);
    fileSystem.MountBundleUVE("", bundlePath, 10);

    const std::optional<std::vector<std::byte>> data = fileSystem.ReadFileUVE("shared.txt");
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(ToStringUVE(*data), "bundle wins content");

    std::filesystem::remove_all(looseDir);
    std::filesystem::remove(bundleSource);
    std::filesystem::remove(bundlePath);
}

TEST_F(FileSystemUVETest, WriteFileUVE_WritesIntoHighestPriorityDirectoryMount) {
    const std::filesystem::path lowDir = "uve_file_system_tests_write_low";
    const std::filesystem::path highDir = "uve_file_system_tests_write_high";
    std::filesystem::remove_all(lowDir);
    std::filesystem::remove_all(highDir);
    std::filesystem::create_directories(lowDir);
    std::filesystem::create_directories(highDir);

    fileSystem.MountDirectoryUVE("", lowDir, 0);
    fileSystem.MountDirectoryUVE("", highDir, 10);

    ASSERT_TRUE(fileSystem.WriteFileUVE("out/data.txt", ToBytesUVE("written content")));

    EXPECT_TRUE(std::filesystem::exists(highDir / "out" / "data.txt"));
    EXPECT_FALSE(std::filesystem::exists(lowDir / "out" / "data.txt"));
    EXPECT_EQ(ReadRealFileAsStringUVE(highDir / "out" / "data.txt"), "written content");

    std::filesystem::remove_all(lowDir);
    std::filesystem::remove_all(highDir);
}

TEST_F(FileSystemUVETest, WriteFileUVE_OnlyBundleMountMatches_ReturnsFalseAndLogsError) {
    const std::filesystem::path source = "uve_file_system_tests_write_bundle_source.txt";
    WriteFixtureFileUVE(source, "data");
    const std::filesystem::path bundlePath = "uve_file_system_tests_write.uvebundle";
    std::filesystem::remove(bundlePath);
    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{6001};
    entry.sourcePath = source;
    entry.virtualName = "shared.txt";
    ASSERT_TRUE(assetBundle.PackUVE({entry}, bundlePath));
    fileSystem.MountBundleUVE("", bundlePath, 0);

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    EXPECT_FALSE(fileSystem.WriteFileUVE("shared.txt", ToBytesUVE("attempted write")));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("no writable") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST_F(FileSystemUVETest, UnmountUVE_RemovesMount_SubsequentReadFails) {
    const std::filesystem::path dir = "uve_file_system_tests_unmount";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    WriteFixtureFileUVE(dir / "f.txt", "data");

    const MountHandleUVE handle = fileSystem.MountDirectoryUVE("", dir, 0);
    ASSERT_TRUE(fileSystem.HasFileUVE("f.txt"));

    fileSystem.UnmountUVE(handle);
    EXPECT_FALSE(fileSystem.HasFileUVE("f.txt"));

    std::filesystem::remove_all(dir);
}

TEST_F(FileSystemUVETest, ResolveRealPathUVE_DirectoryMount_ReturnsRealPathAndEmptyWhenUnresolved) {
    const std::filesystem::path dir = "uve_file_system_tests_resolve";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    WriteFixtureFileUVE(dir / "f.txt", "data");
    fileSystem.MountDirectoryUVE("", dir, 0);

    EXPECT_EQ(fileSystem.ResolveRealPathUVE("f.txt"), dir / "f.txt");
    EXPECT_TRUE(fileSystem.ResolveRealPathUVE("missing.txt").empty());

    std::filesystem::remove_all(dir);
}

TEST_F(FileSystemUVETest, ResolveRealPathUVE_BundleBackedVirtualPath_ReturnsEmptyPath) {
    const std::filesystem::path source = "uve_file_system_tests_resolve_bundle_source.txt";
    WriteFixtureFileUVE(source, "data");
    const std::filesystem::path bundlePath = "uve_file_system_tests_resolve.uvebundle";
    std::filesystem::remove(bundlePath);
    AssetBundleEntryUVE entry;
    entry.guid = AssetGuidUVE{7001};
    entry.sourcePath = source;
    entry.virtualName = "shared.txt";
    ASSERT_TRUE(assetBundle.PackUVE({entry}, bundlePath));
    fileSystem.MountBundleUVE("", bundlePath, 0);

    EXPECT_TRUE(fileSystem.ResolveRealPathUVE("shared.txt").empty());

    std::filesystem::remove(source);
    std::filesystem::remove(bundlePath);
}

TEST_F(FileSystemUVETest, ReadFileUVE_NoMountResolves_ReturnsNulloptAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    EXPECT_FALSE(fileSystem.ReadFileUVE("nonexistent.txt").has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("no mount resolves") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

} // namespace
} // namespace UVE::Asset::Tests
