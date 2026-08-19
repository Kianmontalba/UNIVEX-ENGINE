// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/audio/wav_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Audio::Tests {
namespace {

void AppendTagUVE(std::vector<std::byte>& bytes, const char* tag) {
    for (std::size_t index = 0U; index < 4U; ++index) {
        bytes.push_back(std::byte{static_cast<unsigned char>(tag[index])});
    }
}

void AppendU16UVE(std::vector<std::byte>& bytes, const std::uint16_t value) {
    bytes.push_back(std::byte{static_cast<unsigned char>(value & 0xFFU)});
    bytes.push_back(std::byte{static_cast<unsigned char>((value >> 8U) & 0xFFU)});
}

void AppendU32UVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
    }
}

[[nodiscard]] std::vector<std::byte> MakePcmWavUVE(const std::uint32_t dataBytes = 8U) {
    std::vector<std::byte> bytes;
    AppendTagUVE(bytes, "RIFF");
    AppendU32UVE(bytes, 36U + dataBytes);
    AppendTagUVE(bytes, "WAVE");
    AppendTagUVE(bytes, "fmt ");
    AppendU32UVE(bytes, 16U);
    AppendU16UVE(bytes, 1U);
    AppendU16UVE(bytes, 2U);
    AppendU32UVE(bytes, 48000U);
    AppendU32UVE(bytes, 192000U);
    AppendU16UVE(bytes, 4U);
    AppendU16UVE(bytes, 16U);
    AppendTagUVE(bytes, "data");
    AppendU32UVE(bytes, dataBytes);
    bytes.resize(bytes.size() + dataBytes, std::byte{0x00});
    return bytes;
}

TEST(WavMetadataUVETest, ParseWavMetadataUVE_ReturnsCoherentPcmFacts) {
    const std::optional<WavMetadataUVE> metadata = ParseWavMetadataUVE(MakePcmWavUVE());
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->audioFormat, 1U);
    EXPECT_EQ(metadata->channels, 2U);
    EXPECT_EQ(metadata->sampleRate, 48000U);
    EXPECT_EQ(metadata->byteRate, 192000U);
    EXPECT_EQ(metadata->blockAlign, 4U);
    EXPECT_EQ(metadata->bitsPerSample, 16U);
    EXPECT_EQ(metadata->dataBytes, 8U);
    EXPECT_NEAR(metadata->durationSeconds, 8.0 / 192000.0, 1e-8);
}

TEST(WavMetadataUVETest, ParseWavMetadataUVE_SkipsPaddedJunkChunk) {
    std::vector<std::byte> bytes;
    AppendTagUVE(bytes, "RIFF");
    AppendU32UVE(bytes, 44U);
    AppendTagUVE(bytes, "WAVE");
    AppendTagUVE(bytes, "JUNK");
    AppendU32UVE(bytes, 1U);
    bytes.push_back(std::byte{0x7F});
    bytes.push_back(std::byte{0x00});
    const std::vector<std::byte> pcm = MakePcmWavUVE();
    bytes.insert(bytes.end(), pcm.begin() + 12, pcm.end());
    ASSERT_TRUE(ParseWavMetadataUVE(bytes).has_value());
}

TEST(WavMetadataUVETest, ParseWavMetadataUVE_RejectsMalformedOrIncoherentInputs) {
    std::vector<std::byte> badMagic = MakePcmWavUVE();
    badMagic[0] = std::byte{'X'};
    EXPECT_FALSE(ParseWavMetadataUVE(badMagic).has_value());

    std::vector<std::byte> badAlignment = MakePcmWavUVE();
    badAlignment[32] = std::byte{0x03};
    EXPECT_FALSE(ParseWavMetadataUVE(badAlignment).has_value());

    std::vector<std::byte> truncated = MakePcmWavUVE();
    truncated.pop_back();
    EXPECT_FALSE(ParseWavMetadataUVE(truncated).has_value());
}

} // namespace
} // namespace UVE::Audio::Tests
