// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/uve_file_envelope_uve.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
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

TEST(UveFileEnvelopeUVETest, WriteThenRead_SaveAssetKind_RoundTrips) {
    const std::filesystem::path path = "uve_file_envelope_tests_save.uvesave";
    std::filesystem::remove(path);

    const std::vector<std::byte> payload = MakePayloadUVE("save payload bytes");
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Save, payload));

    const auto result = ReadUveFileUVE(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first.assetType, AssetKindUVE::Save);
    EXPECT_EQ(result->second, payload);

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

TEST(UveFileEnvelopeUVETest, ReadUveFileUVE_OversizedDeclaredPayload_ReturnsNulloptWithoutAllocating) {
    const std::filesystem::path path = "uve_file_envelope_tests_oversized_payload.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        ASSERT_TRUE(file.is_open());
        const std::uint64_t oversizedPayload = std::numeric_limits<std::uint64_t>::max();
        file.seekp(16);
        file.write(reinterpret_cast<const char*>(&oversizedPayload), sizeof(oversizedPayload));
    }

    EXPECT_FALSE(ReadUveFileUVE(path).has_value());
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
