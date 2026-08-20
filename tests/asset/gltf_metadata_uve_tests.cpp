// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/gltf_metadata_uve.h"
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <gtest/gtest.h>
namespace UVE::Asset::Tests {
namespace {
void AppendU32LEUVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
}
TEST(GltfMetadataUVETest, ClassifyGltfResourceUriUVE_AcceptsSafeRelativeAndDataUris) {
    EXPECT_EQ(ClassifyGltfResourceUriUVE("textures/albedo.png"), GltfResourceUriKindUVE::RelativePath);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("mesh\\lod0.bin"), GltfResourceUriKindUVE::RelativePath);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("data:image/png;base64,AAAA"), GltfResourceUriKindUVE::DataUri);
}

TEST(GltfMetadataUVETest, ClassifyGltfResourceUriUVE_RejectsUnsafeAndUnboundedUris) {
    EXPECT_EQ(ClassifyGltfResourceUriUVE(""), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("/textures/albedo.png"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("http://example.com/mesh.bin"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("textures/../mesh.bin"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("textures/%2e%2e/mesh.bin"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("textures//mesh.bin"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("textures/%ZZ/mesh.bin"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE("data:image/png;base64"), GltfResourceUriKindUVE::Invalid);
    EXPECT_EQ(ClassifyGltfResourceUriUVE(std::string("textures/mesh\0.bin", 18U)), GltfResourceUriKindUVE::Invalid);
    const std::string oversized(kMaximumGltfResourceUriBytesUVE + 1U, 'x');
    EXPECT_EQ(ClassifyGltfResourceUriUVE(oversized), GltfResourceUriKindUVE::Invalid);
}

TEST(GltfMetadataUVETest, DecodeGltfDataUriUVE_DecodesPercentAndBase64Payloads) {
    std::vector<std::byte> decoded;
    ASSERT_TRUE(DecodeGltfDataUriUVE("data:text/plain,hello%20world", decoded));
    const std::vector<std::byte> expectedText{std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'},
                                               std::byte{'o'}, std::byte{' '}, std::byte{'w'}, std::byte{'o'},
                                               std::byte{'r'}, std::byte{'l'}, std::byte{'d'}};
    EXPECT_EQ(decoded, expectedText);
    ASSERT_TRUE(DecodeGltfDataUriUVE("data:application/octet-stream;base64,SGVsbG8=", decoded));
    const std::vector<std::byte> expectedBase64{std::byte{'H'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}};
    EXPECT_EQ(decoded, expectedBase64);
}

TEST(GltfMetadataUVETest, DecodeGltfDataUriUVE_RejectsMalformedAndCappedPayloadsAtomically) {
    const std::vector<std::byte> original{std::byte{0xAA}, std::byte{0xBB}};
    std::vector<std::byte> decoded = original;
    EXPECT_FALSE(DecodeGltfDataUriUVE("data:text/plain,hello%2", decoded));
    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(DecodeGltfDataUriUVE("data:application/octet-stream;base64,SGVsbG8", decoded));
    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(DecodeGltfDataUriUVE("data:application/octet-stream;base64,AB==", decoded));
    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(DecodeGltfDataUriUVE("data:application/octet-stream;base64,AAB=", decoded));
    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(DecodeGltfDataUriUVE("data:text/plain,hello", decoded, 4U));
    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(DecodeGltfDataUriUVE("textures/mesh.bin", decoded));
    EXPECT_EQ(decoded, original);
    ASSERT_TRUE(DecodeGltfDataUriUVE("data:,", decoded, 0U));
    EXPECT_TRUE(decoded.empty());
}

TEST(GltfMetadataUVETest, ResolveGltfResourceVirtualPathUVE_ComposesBoundedVfsPaths) {
    std::string resolved = "unchanged";
    ASSERT_TRUE(ResolveGltfResourceVirtualPathUVE("models/characters/hero.gltf", "textures/albedo.png", resolved));
    EXPECT_EQ(resolved, "models/characters/textures/albedo.png");
    ASSERT_TRUE(ResolveGltfResourceVirtualPathUVE("scene.gltf", "mesh\\lod0.bin", resolved));
    EXPECT_EQ(resolved, "mesh/lod0.bin");
}

TEST(GltfMetadataUVETest, ResolveGltfResourceVirtualPathUVE_RejectsInvalidInputsAtomically) {
    const std::string original = "stable/path.bin";
    std::string resolved = original;
    EXPECT_FALSE(ResolveGltfResourceVirtualPathUVE("models/hero.gltf", "data:application/octet-stream,AAAA", resolved));
    EXPECT_EQ(resolved, original);
    EXPECT_FALSE(ResolveGltfResourceVirtualPathUVE("models/../hero.gltf", "mesh.bin", resolved));
    EXPECT_EQ(resolved, original);
    EXPECT_FALSE(ResolveGltfResourceVirtualPathUVE("/hero.gltf", "mesh.bin", resolved));
    EXPECT_EQ(resolved, original);
    const std::string oversizedCombined(kMaximumGltfResourceUriBytesUVE - 6U, 'a');
    EXPECT_FALSE(ResolveGltfResourceVirtualPathUVE("models/hero.gltf", oversizedCombined, resolved));
    EXPECT_EQ(resolved, original);
    const std::string oversizedResource(kMaximumGltfResourceUriBytesUVE + 1U, 'b');
    EXPECT_FALSE(ResolveGltfResourceVirtualPathUVE("hero.gltf", oversizedResource, resolved));
    EXPECT_EQ(resolved, original);
}

TEST(GltfMetadataUVETest, ValidateGltfAccessorSpanUVE_AcceptsBoundedTightAndStridedSpans) {
    EXPECT_TRUE(ValidateGltfAccessorSpanUVE(64U, 0U, 16U, 4U, 4U));
    EXPECT_TRUE(ValidateGltfAccessorSpanUVE(64U, 4U, 4U, 12U, 8U));
    EXPECT_TRUE(ValidateGltfAccessorSpanUVE(8U, 8U, 0U, 1U, 1U));
}

TEST(GltfMetadataUVETest, ValidateGltfAccessorSpanUVE_RejectsInvalidAndOverflowingSpans) {
    EXPECT_FALSE(ValidateGltfAccessorSpanUVE(64U, 65U, 1U, 4U, 4U));
    EXPECT_FALSE(ValidateGltfAccessorSpanUVE(64U, 0U, 2U, 3U, 4U));
    EXPECT_FALSE(ValidateGltfAccessorSpanUVE(64U, 60U, 2U, 4U, 4U));
    EXPECT_FALSE(ValidateGltfAccessorSpanUVE(64U, 0U, 1'000'001U, 4U, 4U));
    EXPECT_TRUE(ValidateGltfAccessorSpanUVE(64U, 0U, 2U, 4U, 4U, 2U));
    EXPECT_FALSE(ValidateGltfAccessorSpanUVE(std::numeric_limits<std::uint64_t>::max(), 0U,
                                             std::numeric_limits<std::uint64_t>::max(),
                                             std::numeric_limits<std::uint64_t>::max(), 1U,
                                             std::numeric_limits<std::uint64_t>::max()));
}

TEST(GltfMetadataUVETest, ParseGltfMetadataUVE_ReturnsCopiedJsonCounts) {
    const auto metadata = ParseGltfMetadataUVE(R"({"asset":{"version":"2.0"},"nodes":[{}],"meshes":[{},{}],"materials":[{}],"images":[{}],"buffers":[{},{}]})");
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->container, GltfContainerKindUVE::Json);
    EXPECT_EQ(metadata->nodeCount, 1U); EXPECT_EQ(metadata->meshCount, 2U); EXPECT_EQ(metadata->materialCount, 1U);
    EXPECT_EQ(metadata->imageCount, 1U); EXPECT_EQ(metadata->bufferCount, 2U); EXPECT_FALSE(metadata->hasBinaryChunk);
}
TEST(GltfMetadataUVETest, ParseGlbMetadataUVE_ValidatesHeaderJsonChunkAndBinaryTail) {
    const std::string json = R"({"asset":{"version":"2.0"},"nodes":[{}],"meshes":[]})";
    std::vector<std::byte> bytes;
    AppendU32LEUVE(bytes, 0x46546C67U); AppendU32LEUVE(bytes, 2U);
    AppendU32LEUVE(bytes, static_cast<std::uint32_t>(20U + json.size() + 8U));
    AppendU32LEUVE(bytes, static_cast<std::uint32_t>(json.size())); AppendU32LEUVE(bytes, 0x4E4F534AU);
    bytes.insert(bytes.end(), reinterpret_cast<const std::byte*>(json.data()), reinterpret_cast<const std::byte*>(json.data() + json.size()));
    AppendU32LEUVE(bytes, 0U); AppendU32LEUVE(bytes, 0x004E4942U);
    const auto metadata = ParseGlbMetadataUVE(bytes);
    ASSERT_TRUE(metadata.has_value()); EXPECT_EQ(metadata->container, GltfContainerKindUVE::Binary);
    EXPECT_EQ(metadata->nodeCount, 1U); EXPECT_TRUE(metadata->hasBinaryChunk);
}
TEST(GltfMetadataUVETest, ParseGltfMetadataUVE_RejectsWrongVersionAndMalformedGlb) {
    EXPECT_FALSE(ParseGltfMetadataUVE(R"({"asset":{"version":"1.0"}})").has_value());
    std::vector<std::byte> bad{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}};
    EXPECT_FALSE(ParseGlbMetadataUVE(bad).has_value());
}
} }
