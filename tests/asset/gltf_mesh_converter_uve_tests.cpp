// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/gltf_mesh_converter_uve.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {
namespace {

void AppendU16LittleEndianUVE(std::vector<std::byte>& bytes, const std::uint16_t value) {
    bytes.push_back(std::byte{static_cast<unsigned char>(value & 0xFFU)});
    bytes.push_back(std::byte{static_cast<unsigned char>((value >> 8U) & 0xFFU)});
}

void AppendFloatLittleEndianUVE(std::vector<std::byte>& bytes, const float value) {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((bits >> shift) & 0xFFU)});
    }
}

void AppendVector3UVE(std::vector<std::byte>& bytes, const float x, const float y, const float z) {
    AppendFloatLittleEndianUVE(bytes, x);
    AppendFloatLittleEndianUVE(bytes, y);
    AppendFloatLittleEndianUVE(bytes, z);
}

GltfAccessorViewUVE MakeAccessorUVE(const std::vector<std::byte>& bytes, const std::uint64_t count,
                                    const GltfComponentTypeUVE componentType) {
    return GltfAccessorViewUVE{std::span<const std::byte>{bytes.data(), bytes.size()}, 0U, count, 0U, componentType};
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_ExplicitAttributesAndU16Indices) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        AppendVector3UVE(bytes, 2.0F, 2.0F, 2.0F);
        return bytes;
    }();
    const std::vector<std::byte> normals = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
        AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
        AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
        AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
        return bytes;
    }();
    const std::vector<std::byte> texcoords = [&] {
        std::vector<std::byte> bytes;
        AppendFloatLittleEndianUVE(bytes, 0.0F); AppendFloatLittleEndianUVE(bytes, 0.0F);
        AppendFloatLittleEndianUVE(bytes, 1.0F); AppendFloatLittleEndianUVE(bytes, 0.0F);
        AppendFloatLittleEndianUVE(bytes, 0.0F); AppendFloatLittleEndianUVE(bytes, 1.0F);
        AppendFloatLittleEndianUVE(bytes, 2.0F); AppendFloatLittleEndianUVE(bytes, 2.0F);
        return bytes;
    }();
    const std::vector<std::byte> indices = [&] {
        std::vector<std::byte> bytes;
        AppendU16LittleEndianUVE(bytes, 0U); AppendU16LittleEndianUVE(bytes, 1U); AppendU16LittleEndianUVE(bytes, 2U);
        return bytes;
    }();

    const GltfPrimitiveSourceUVE source{
        MakeAccessorUVE(positions, 4U, GltfComponentTypeUVE::Float),
        MakeAccessorUVE(normals, 4U, GltfComponentTypeUVE::Float),
        MakeAccessorUVE(texcoords, 4U, GltfComponentTypeUVE::Float),
        MakeAccessorUVE(indices, 3U, GltfComponentTypeUVE::UnsignedShort),
        4U,
    };
    MeshAssetUVE mesh;
    ASSERT_TRUE(ConvertGltfPrimitiveUVE(source, mesh));
    ASSERT_EQ(mesh.vertices.size(), 4U);
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
    EXPECT_EQ(mesh.vertices[1].position, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
    EXPECT_EQ(mesh.vertices[0].normal, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_FLOAT_EQ(mesh.vertices[1].u, 1.0F);
    EXPECT_FLOAT_EQ(mesh.vertices[2].v, 1.0F);
    EXPECT_EQ(mesh.localBounds,
              (Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{2.0F, 2.0F, 2.0F}}));
    EXPECT_NEAR(Math::LengthUVE(mesh.vertices[0].tangent), 1.0F, 0.0001F);
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_MissingNormalsAccumulatesFaceNormal) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        return bytes;
    }();
    MeshAssetUVE mesh;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float), std::nullopt,
                                        std::nullopt, std::nullopt, 4U};
    ASSERT_TRUE(ConvertGltfPrimitiveUVE(source, mesh));
    for (const MeshVertexUVE& vertex : mesh.vertices) {
        EXPECT_EQ(vertex.normal, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    }
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_MissingIndicesGeneratesSequentialIndices) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        return bytes;
    }();
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float), std::nullopt,
                                        std::nullopt, std::nullopt, 4U};
    MeshAssetUVE mesh;
    ASSERT_TRUE(ConvertGltfPrimitiveUVE(source, mesh));
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_NonTrianglesModeRejects) {
    const std::vector<std::byte> positions(36U);
    MeshAssetUVE mesh;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float), std::nullopt,
                                        std::nullopt, std::nullopt, 5U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, mesh));
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_NonFinitePositionPreservesOutput) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, std::numeric_limits<float>::quiet_NaN(), 1.0F, 0.0F);
        return bytes;
    }();
    MeshAssetUVE output;
    output.vertices = {MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F,
                                     0.0F}};
    output.indices = {0U};
    output.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}};
    const MeshAssetUVE original = output;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float), std::nullopt,
                                        std::nullopt, std::nullopt, 4U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
    EXPECT_EQ(output.localBounds, original.localBounds);
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_OverflowedFiniteNormalPreservesOutput) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        return bytes;
    }();
    const std::vector<std::byte> normals = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, std::numeric_limits<float>::max(), 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        return bytes;
    }();
    MeshAssetUVE output;
    output.vertices = {MeshVertexUVE{Math::Vector3UVE{7.0F, 8.0F, 9.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F,
                                     0.0F}};
    output.indices = {0U};
    const MeshAssetUVE original = output;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float),
                                        MakeAccessorUVE(normals, 3U, GltfComponentTypeUVE::Float), std::nullopt,
                                        std::nullopt, 4U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_OverflowedGeneratedNormalAccumulationPreservesOutput) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0e19F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 3.0e19F, 0.0F);
        AppendVector3UVE(bytes, 1.0e19F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 3.0e19F, 0.0F);
        return bytes;
    }();
    const std::vector<std::byte> indices = [&] {
        std::vector<std::byte> bytes;
        for (const std::uint16_t index : {std::uint16_t{0U}, std::uint16_t{1U}, std::uint16_t{2U},
                                           std::uint16_t{0U}, std::uint16_t{3U}, std::uint16_t{4U}}) {
            AppendU16LittleEndianUVE(bytes, index);
        }
        return bytes;
    }();
    MeshAssetUVE output;
    output.vertices = {MeshVertexUVE{Math::Vector3UVE{7.0F, 8.0F, 9.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F,
                                     0.0F}};
    output.indices = {0U};
    const MeshAssetUVE original = output;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 5U, GltfComponentTypeUVE::Float), std::nullopt,
                                        MakeAccessorUVE(indices, 6U, GltfComponentTypeUVE::UnsignedShort), std::nullopt,
                                        4U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_OutOfBoundsIndexPreservesOutput) {
    const std::vector<std::byte> positions = [&] {
        std::vector<std::byte> bytes;
        AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
        AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
        return bytes;
    }();
    const std::vector<std::byte> indices = [&] {
        std::vector<std::byte> bytes;
        AppendU16LittleEndianUVE(bytes, 0U); AppendU16LittleEndianUVE(bytes, 1U); AppendU16LittleEndianUVE(bytes, 3U);
        return bytes;
    }();
    MeshAssetUVE output;
    output.vertices = {MeshVertexUVE{Math::Vector3UVE{7.0F, 8.0F, 9.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F,
                                     0.0F}};
    output.indices = {0U};
    const MeshAssetUVE original = output;
    const GltfPrimitiveSourceUVE source{
        MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::Float), std::nullopt, std::nullopt,
        MakeAccessorUVE(indices, 3U, GltfComponentTypeUVE::UnsignedShort), 4U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
}

TEST(GltfPrimitiveConverterUVETest, ConvertGltfPrimitiveUVE_WrongPositionComponentTypeRejects) {
    const std::vector<std::byte> positions(12U);
    MeshAssetUVE mesh;
    const GltfPrimitiveSourceUVE source{MakeAccessorUVE(positions, 3U, GltfComponentTypeUVE::UnsignedShort),
                                        std::nullopt, std::nullopt, std::nullopt, 4U};
    EXPECT_FALSE(ConvertGltfPrimitiveUVE(source, mesh));
}

} // namespace
} // namespace UVE::Asset::Tests
