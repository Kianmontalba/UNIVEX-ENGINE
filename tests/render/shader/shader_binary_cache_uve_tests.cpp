// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "shader_binary_cache_uve.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Render::Shader::Detail::Tests {
namespace {

TEST(ShaderBinaryCacheUVETest, GetCacheFilePathUVE_IncludesPlatformSubdirectoryAndHexHash) {
    const std::filesystem::path path = GetCacheFilePathUVE("shader_cache/", 0x0123456789ABCDEFULL);
    const std::string pathString = path.generic_string();

    EXPECT_NE(pathString.find("shader_cache/"), std::string::npos);
    EXPECT_NE(pathString.find("0123456789abcdef.uveshadercache"), std::string::npos);
}

TEST(ShaderBinaryCacheUVETest, GetCacheFilePathUVE_DifferentHashes_ProduceDifferentPaths) {
    EXPECT_NE(GetCacheFilePathUVE("shader_cache/", 1), GetCacheFilePathUVE("shader_cache/", 2));
}

TEST(ShaderBinaryCacheUVETest, ReadCacheEntryUVE_MissingFile_ReturnsNullopt) {
    EXPECT_FALSE(ReadCacheEntryUVE("uve_shader_binary_cache_tests_does_not_exist.uveshadercache").has_value());
}

TEST(ShaderBinaryCacheUVETest, WriteThenReadCacheEntryUVE_RoundTripsPayloadAndFormat) {
    const std::filesystem::path path = "uve_shader_binary_cache_tests_round_trip/linux/aaaa.uveshadercache";
    std::filesystem::remove_all("uve_shader_binary_cache_tests_round_trip");

    const std::vector<std::byte> payload = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0xFF}};
    ASSERT_TRUE(WriteCacheEntryUVE(path, 0xCAFEBABEU, payload));

    const std::optional<CacheEntryUVE> entry = ReadCacheEntryUVE(path);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->glBinaryFormat, 0xCAFEBABEU);
    ASSERT_EQ(entry->payload.size(), payload.size());
    EXPECT_EQ(entry->payload, payload);

    std::filesystem::remove_all("uve_shader_binary_cache_tests_round_trip");
}

TEST(ShaderBinaryCacheUVETest, WriteCacheEntryUVE_CreatesParentDirectoriesAsNeeded) {
    const std::filesystem::path path = "uve_shader_binary_cache_tests_mkdirs/nested/dir/x.uveshadercache";
    std::filesystem::remove_all("uve_shader_binary_cache_tests_mkdirs");

    const std::vector<std::byte> payload = {std::byte{0x7A}};
    EXPECT_TRUE(WriteCacheEntryUVE(path, 1, payload));
    EXPECT_TRUE(std::filesystem::exists(path));

    std::filesystem::remove_all("uve_shader_binary_cache_tests_mkdirs");
}

TEST(ShaderBinaryCacheUVETest, ReadCacheEntryUVE_BadMagic_ReturnsNulloptRatherThanCrashing) {
    const std::filesystem::path path = "uve_shader_binary_cache_tests_bad_magic.uveshadercache";
    {
        std::ofstream file(path, std::ios::binary);
        file << "NOT-A-VALID-CACHE-FILE-HEADER";
    }

    EXPECT_FALSE(ReadCacheEntryUVE(path).has_value());
    std::filesystem::remove(path);
}

TEST(ShaderBinaryCacheUVETest, ReadCacheEntryUVE_TruncatedPayload_ReturnsNulloptRatherThanCrashing) {
    const std::filesystem::path path = "uve_shader_binary_cache_tests_truncated/linux/x.uveshadercache";
    std::filesystem::remove_all("uve_shader_binary_cache_tests_truncated");
    ASSERT_TRUE(WriteCacheEntryUVE(path, 1, std::vector<std::byte>{std::byte{1}, std::byte{2}, std::byte{3}}));

    // Truncate the file so its declared payload length no longer matches what's actually on disk.
    std::filesystem::resize_file(path, std::filesystem::file_size(path) - 2);

    EXPECT_FALSE(ReadCacheEntryUVE(path).has_value());
    std::filesystem::remove_all("uve_shader_binary_cache_tests_truncated");
}

TEST(ShaderBinaryCacheUVETest, WriteThenReadCacheEntryUVE_EmptyPayload_RoundTrips) {
    const std::filesystem::path path = "uve_shader_binary_cache_tests_empty_payload/linux/x.uveshadercache";
    std::filesystem::remove_all("uve_shader_binary_cache_tests_empty_payload");
    ASSERT_TRUE(WriteCacheEntryUVE(path, 42, {}));

    const std::optional<CacheEntryUVE> entry = ReadCacheEntryUVE(path);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->glBinaryFormat, 42U);
    EXPECT_TRUE(entry->payload.empty());

    std::filesystem::remove_all("uve_shader_binary_cache_tests_empty_payload");
}

} // namespace
} // namespace UVE::Render::Shader::Detail::Tests
