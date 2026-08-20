// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/save/save_payload_compression_uve.h"
#include <gtest/gtest.h>
namespace UVE::Save::Tests {
namespace {
TEST(SavePayloadCompressionUVETest, RepetitivePayloadCompressesAndRoundTrips) {
    const std::vector<std::byte> payload(1024U, std::byte{0x2A});
    const std::vector<std::byte> compressed = CompressSavePayloadUVE(payload);
    ASSERT_LT(compressed.size(), payload.size());
    std::vector<std::byte> restored;
    ASSERT_TRUE(DecompressSavePayloadUVE(compressed, restored));
    EXPECT_EQ(restored, payload);
}
TEST(SavePayloadCompressionUVETest, IncompressiblePayloadFallsBackToRawAndRoundTrips) {
    std::vector<std::byte> payload;
    for (std::uint32_t value = 0U; value < 64U; ++value) {
        payload.push_back(static_cast<std::byte>(value));
    }
    const std::vector<std::byte> encoded = CompressSavePayloadUVE(payload);
    EXPECT_EQ(encoded, payload);
    std::vector<std::byte> restored;
    ASSERT_TRUE(DecompressSavePayloadUVE(encoded, restored));
    EXPECT_EQ(restored, payload);
}
TEST(SavePayloadCompressionUVETest, CorruptCompressedPayloadFailsAtomically) {
    const std::vector<std::byte> payload(128U, std::byte{0x7F});
    std::vector<std::byte> compressed = CompressSavePayloadUVE(payload);
    ASSERT_LT(compressed.size(), payload.size());
    compressed.pop_back();
    std::vector<std::byte> restored{std::byte{0x11}};
    EXPECT_FALSE(DecompressSavePayloadUVE(compressed, restored));
    EXPECT_EQ(restored, std::vector<std::byte>{std::byte{0x11}});
}
TEST(SavePayloadCompressionUVETest, CorruptCompressedByteFailsFingerprintValidationAtomically) {
    const std::vector<std::byte> payload(128U, std::byte{0x7F});
    std::vector<std::byte> compressed = CompressSavePayloadUVE(payload);
    ASSERT_LT(compressed.size(), payload.size());
    compressed.back() = std::byte{0x7E};
    std::vector<std::byte> restored{std::byte{0x11}};
    EXPECT_FALSE(DecompressSavePayloadUVE(compressed, restored));
    EXPECT_EQ(restored, std::vector<std::byte>{std::byte{0x11}});
}
TEST(SavePayloadCompressionUVETest, LegacyCompressedEnvelopeRemainsReadable) {
    std::vector<std::byte> legacy{std::byte{'U'}, std::byte{'V'}, std::byte{'S'}, std::byte{'C'}};
    const std::uint64_t expectedSize = 4U;
    const auto* sizeBytes = reinterpret_cast<const std::byte*>(&expectedSize);
    legacy.insert(legacy.end(), sizeBytes, sizeBytes + sizeof(expectedSize));
    legacy.push_back(std::byte{4U});
    legacy.push_back(std::byte{'a'});

    std::vector<std::byte> restored;
    ASSERT_TRUE(DecompressSavePayloadUVE(legacy, restored));
    EXPECT_EQ(restored, (std::vector<std::byte>{std::byte{'a'}, std::byte{'a'}, std::byte{'a'}, std::byte{'a'}}));
}
TEST(SavePayloadCompressionUVETest, OversizedPayloadIsRejected) {
    const std::vector<std::byte> payload(kMaximumCompressedSavePayloadBytesUVE + 1U, std::byte{0});
    EXPECT_TRUE(CompressSavePayloadUVE(payload).empty());
    std::vector<std::byte> restored;
    EXPECT_FALSE(DecompressSavePayloadUVE(payload, restored));
}
} // namespace
} // namespace UVE::Save::Tests
