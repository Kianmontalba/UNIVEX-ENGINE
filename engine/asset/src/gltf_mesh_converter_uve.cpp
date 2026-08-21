// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/gltf_mesh_converter_uve.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

#include "uve/math/vector3_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::uint64_t kMaximumPrimitiveIndicesUVE = kMaximumGltfAccessorElementsUVE * 3U;
constexpr float kMinimumNormalLengthSquaredUVE = 1.0e-12F;

[[nodiscard]] std::uint64_t ComponentSizeUVE(const GltfComponentTypeUVE componentType) noexcept {
    switch (componentType) {
    case GltfComponentTypeUVE::UnsignedByte:
        return 1U;
    case GltfComponentTypeUVE::UnsignedShort:
        return 2U;
    case GltfComponentTypeUVE::UnsignedInt:
    case GltfComponentTypeUVE::Float:
        return 4U;
    }
    return 0U;
}

[[nodiscard]] bool ValidateAccessorUVE(const GltfAccessorViewUVE& accessor, const std::uint64_t elementSize,
                                       const std::uint64_t maximumElements) noexcept {
    if (ComponentSizeUVE(accessor.componentType) == 0U || accessor.elementCount == 0U ||
        accessor.elementCount > maximumElements) {
        return false;
    }
    const std::uint64_t stride = accessor.byteStride == 0U ? elementSize : accessor.byteStride;
    if (stride < elementSize || stride > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    return ValidateGltfAccessorSpanUVE(accessor.buffer.size(), accessor.byteOffset, accessor.elementCount, stride,
                                       elementSize, maximumElements);
}

[[nodiscard]] const std::byte* AccessorElementUVE(const GltfAccessorViewUVE& accessor,
                                                   const std::size_t elementIndex,
                                                   const std::uint64_t elementSize) noexcept {
    const std::uint64_t stride = accessor.byteStride == 0U ? elementSize : accessor.byteStride;
    const std::uint64_t offset = accessor.byteOffset + static_cast<std::uint64_t>(elementIndex) * stride;
    if (offset > accessor.buffer.size() || accessor.buffer.size() - static_cast<std::size_t>(offset) <
                                              static_cast<std::size_t>(elementSize)) {
        return nullptr;
    }
    return accessor.buffer.data() + static_cast<std::size_t>(offset);
}

[[nodiscard]] std::uint32_t ReadU32LittleEndianUVE(const std::byte* bytes) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[0])) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[3])) << 24U);
}

[[nodiscard]] std::uint16_t ReadU16LittleEndianUVE(const std::byte* bytes) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[0])) |
           static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[1]) << 8U);
}

[[nodiscard]] float ReadFloatLittleEndianUVE(const std::byte* bytes) noexcept {
    return std::bit_cast<float>(ReadU32LittleEndianUVE(bytes));
}

[[nodiscard]] UVE::Math::Vector3UVE ReadVector3UVE(const GltfAccessorViewUVE& accessor,
                                                   const std::size_t index) noexcept {
    const std::byte* const bytes = AccessorElementUVE(accessor, index, 12U);
    return UVE::Math::Vector3UVE{ReadFloatLittleEndianUVE(bytes), ReadFloatLittleEndianUVE(bytes + 4U),
                                 ReadFloatLittleEndianUVE(bytes + 8U)};
}

[[nodiscard]] std::uint32_t ReadIndexUVE(const GltfAccessorViewUVE& accessor,
                                         const std::size_t index) noexcept {
    const std::byte* const bytes = AccessorElementUVE(accessor, index, ComponentSizeUVE(accessor.componentType));
    switch (accessor.componentType) {
    case GltfComponentTypeUVE::UnsignedByte:
        return std::to_integer<std::uint8_t>(bytes[0]);
    case GltfComponentTypeUVE::UnsignedShort:
        return ReadU16LittleEndianUVE(bytes);
    case GltfComponentTypeUVE::UnsignedInt:
        return ReadU32LittleEndianUVE(bytes);
    case GltfComponentTypeUVE::Float:
        return 0U;
    }
    return 0U;
}

[[nodiscard]] bool IsFiniteVectorUVE(const UVE::Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool HasUsableNormalLengthUVE(const UVE::Math::Vector3UVE& value) noexcept {
    const float lengthSquared = UVE::Math::LengthSquaredUVE(value);
    return std::isfinite(lengthSquared) && lengthSquared > kMinimumNormalLengthSquaredUVE;
}

[[nodiscard]] UVE::Math::Vector3UVE NormalizeOrFallbackUVE(const UVE::Math::Vector3UVE& value) noexcept {
    if (!HasUsableNormalLengthUVE(value)) {
        return UVE::Math::Vector3UVE{0.0F, 1.0F, 0.0F};
    }
    return UVE::Math::NormalizeUVE(value);
}

} // namespace

bool ConvertGltfPrimitiveUVE(const GltfPrimitiveSourceUVE& source, MeshAssetUVE& outMesh) {
    try {
        if (source.mode != 4U || source.positions.componentType != GltfComponentTypeUVE::Float ||
        source.positions.elementCount == 0U || source.positions.elementCount > kMaximumGltfAccessorElementsUVE ||
        !ValidateAccessorUVE(source.positions, 12U, kMaximumGltfAccessorElementsUVE)) {
        return false;
    }

    const std::size_t vertexCount = static_cast<std::size_t>(source.positions.elementCount);
    if (source.normals.has_value()) {
        if (source.normals->componentType != GltfComponentTypeUVE::Float ||
            source.normals->elementCount != source.positions.elementCount ||
            !ValidateAccessorUVE(*source.normals, 12U, kMaximumGltfAccessorElementsUVE)) {
            return false;
        }
    }
    if (source.texcoords0.has_value()) {
        if (source.texcoords0->componentType != GltfComponentTypeUVE::Float ||
            source.texcoords0->elementCount != source.positions.elementCount ||
            !ValidateAccessorUVE(*source.texcoords0, 8U, kMaximumGltfAccessorElementsUVE)) {
            return false;
        }
    }

    const std::uint64_t indexCount = source.indices.has_value() ? source.indices->elementCount
                                                                  : source.positions.elementCount;
    if (indexCount == 0U || indexCount % 3U != 0U || indexCount > kMaximumPrimitiveIndicesUVE) {
        return false;
    }
    if (source.indices.has_value()) {
        const std::uint64_t indexElementSize = ComponentSizeUVE(source.indices->componentType);
        if ((source.indices->componentType != GltfComponentTypeUVE::UnsignedByte &&
             source.indices->componentType != GltfComponentTypeUVE::UnsignedShort &&
             source.indices->componentType != GltfComponentTypeUVE::UnsignedInt) ||
            !ValidateAccessorUVE(*source.indices, indexElementSize, kMaximumPrimitiveIndicesUVE)) {
            return false;
        }
    }

    MeshAssetUVE candidate;
    candidate.vertices.resize(vertexCount);
    candidate.indices.resize(static_cast<std::size_t>(indexCount));
    bool hasBounds = false;
    for (std::size_t vertexIndex = 0U; vertexIndex < vertexCount; ++vertexIndex) {
        const UVE::Math::Vector3UVE position = ReadVector3UVE(source.positions, vertexIndex);
        if (!IsFiniteVectorUVE(position)) {
            return false;
        }
        candidate.vertices[vertexIndex].position = position;
        if (source.normals.has_value()) {
            const UVE::Math::Vector3UVE normal = ReadVector3UVE(*source.normals, vertexIndex);
            if (!IsFiniteVectorUVE(normal) || !HasUsableNormalLengthUVE(normal)) {
                return false;
            }
            candidate.vertices[vertexIndex].normal = UVE::Math::NormalizeUVE(normal);
        }
        if (source.texcoords0.has_value()) {
            const std::byte* const bytes = AccessorElementUVE(*source.texcoords0, vertexIndex, 8U);
            candidate.vertices[vertexIndex].u = ReadFloatLittleEndianUVE(bytes);
            candidate.vertices[vertexIndex].v = ReadFloatLittleEndianUVE(bytes + 4U);
            if (!std::isfinite(candidate.vertices[vertexIndex].u) ||
                !std::isfinite(candidate.vertices[vertexIndex].v)) {
                return false;
            }
        }
        if (!hasBounds) {
            candidate.localBounds.min = position;
            candidate.localBounds.max = position;
            hasBounds = true;
        } else {
            candidate.localBounds.min.x = std::min(candidate.localBounds.min.x, position.x);
            candidate.localBounds.min.y = std::min(candidate.localBounds.min.y, position.y);
            candidate.localBounds.min.z = std::min(candidate.localBounds.min.z, position.z);
            candidate.localBounds.max.x = std::max(candidate.localBounds.max.x, position.x);
            candidate.localBounds.max.y = std::max(candidate.localBounds.max.y, position.y);
            candidate.localBounds.max.z = std::max(candidate.localBounds.max.z, position.z);
        }
    }

    for (std::size_t index = 0U; index < candidate.indices.size(); ++index) {
        const std::uint32_t value = source.indices.has_value()
                                        ? ReadIndexUVE(*source.indices, index)
                                        : static_cast<std::uint32_t>(index);
        if (value >= vertexCount) {
            return false;
        }
        candidate.indices[index] = value;
    }

    std::vector<UVE::Math::Vector3UVE> normalAccumulation;
    if (!source.normals.has_value()) {
        normalAccumulation.resize(vertexCount);
        for (std::size_t triangle = 0U; triangle < candidate.indices.size(); triangle += 3U) {
            const auto& a = candidate.vertices[candidate.indices[triangle]].position;
            const auto& b = candidate.vertices[candidate.indices[triangle + 1U]].position;
            const auto& c = candidate.vertices[candidate.indices[triangle + 2U]].position;
            const UVE::Math::Vector3UVE faceNormal = UVE::Math::CrossUVE(b - a, c - a);
            if (!IsFiniteVectorUVE(faceNormal) || !HasUsableNormalLengthUVE(faceNormal)) {
                return false;
            }
            normalAccumulation[candidate.indices[triangle]] += faceNormal;
            normalAccumulation[candidate.indices[triangle + 1U]] += faceNormal;
            normalAccumulation[candidate.indices[triangle + 2U]] += faceNormal;
        }
        for (std::size_t vertexIndex = 0U; vertexIndex < vertexCount; ++vertexIndex) {
            candidate.vertices[vertexIndex].normal = NormalizeOrFallbackUVE(normalAccumulation[vertexIndex]);
        }
    }

    GenerateMeshTangentsUVE(candidate.vertices, candidate.indices);
    outMesh = std::move(candidate);
    return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace UVE::Asset
