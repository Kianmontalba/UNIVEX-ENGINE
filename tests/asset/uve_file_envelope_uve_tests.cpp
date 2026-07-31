//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/uve_file_envelope_uve.h"

#include <algorithm>
#include <cstddef>
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

[[nodiscard]] std::vector<std::byte> MakePayloadUVE(std::string_view text) {
    const auto* const bytes = reinterpret_cast<const std::byte*>(text.data());
    return std::vector<std::byte>(bytes, bytes + text.size());
}

TEST(UveFileEnvelopeUVETest, WriteThenRead_RoundTripsPayloadAndAssetKind) {
    const std::filesystem::path path = "uve_file_envelope_tests_roundtrip.uveblob";
    std::filesystem::remove(path);

    const std::vector<std::byte> payload = MakePayloadUVE("hello universe");
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, payload));

    const auto result = ReadUveFileUVE(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first.assetType, AssetKindUVE::Blob);
    EXPECT_EQ(result->first.compressionMethod, 0U);
    EXPECT_EQ(result->second, payload);

    std::filesystem::remove(path);
}

TEST(UveFileEnvelopeUVETest, WriteThenRead_EmptyPayload_RoundTrips) {
    const std::filesystem::path path = "uve_file_envelope_tests_empty.uveblob";
    std::filesystem::remove(path);

    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Bundle, {}));
    const auto result = ReadUveFileUVE(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first.assetType, AssetKindUVE::Bundle);
    EXPECT_TRUE(result->second.empty());

    std::filesystem::remove(path);
}

TEST(UveFileEnvelopeUVETest, ReadUveFileUVE_MissingFile_ReturnsNulloptAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const auto result = ReadUveFileUVE("uve_file_envelope_tests_nonexistent.uveblob");
    EXPECT_FALSE(result.has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(UveFileEnvelopeUVETest, ReadUveFileUVE_BadMagic_ReturnsNulloptAndLogsError) {
    const std::filesystem::path path = "uve_file_envelope_tests_bad_magic.uveblob";
    {
        std::ofstream file(path, std::ios::binary);
        file << "NOT A VALID UVE FILE";
    }

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const auto result = ReadUveFileUVE(path);
    EXPECT_FALSE(result.has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("bad magic") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(UveFileEnvelopeUVETest, ReadUveFileUVE_UnsupportedCompression_ReturnsNulloptAndLogsError) {
    const std::filesystem::path path = "uve_file_envelope_tests_bad_compression.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("data")));

    // Corrupt the compressionMethod field (bytes [12, 16) - magic[4] + version[4] + assetType[4])
    // to a never-implemented value, simulating a future/unsupported file.
    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        ASSERT_TRUE(file.is_open());
        const std::uint32_t badCompression = 99;
        file.seekp(12);
        file.write(reinterpret_cast<const char*>(&badCompression), sizeof(badCompression));
    }

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const auto result = ReadUveFileUVE(path);
    EXPECT_FALSE(result.has_value());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("compression") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
