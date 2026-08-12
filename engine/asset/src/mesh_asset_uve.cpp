// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/mesh_asset_uve.h"

#include <cmath>
#include <cstring>
#include <span>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

void AppendBytesUVE(std::vector<std::byte>& buffer, const void* data, std::size_t size) {
    const auto* const bytes = static_cast<const std::byte*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

void AppendUint32UVE(std::vector<std::byte>& buffer, std::uint32_t value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

void AppendFloatUVE(std::vector<std::byte>& buffer, float value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

[[nodiscard]] bool ReadUint32FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                            std::uint32_t& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

[[nodiscard]] bool ReadFloatFromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                           float& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

[[nodiscard]] bool ReadVector3FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                             Math::Vector3UVE& outValue) {
    return ReadFloatFromBufferUVE(buffer, offset, outValue.x) && ReadFloatFromBufferUVE(buffer, offset, outValue.y) &&
           ReadFloatFromBufferUVE(buffer, offset, outValue.z);
}

void AppendVector3UVE(std::vector<std::byte>& buffer, const Math::Vector3UVE& value) {
    AppendFloatUVE(buffer, value.x);
    AppendFloatUVE(buffer, value.y);
    AppendFloatUVE(buffer, value.z);
}

[[nodiscard]] Math::Vector3UVE DeterministicTangentFallbackUVE(const Math::Vector3UVE& normal) noexcept {
    const Math::Vector3UVE axis = std::abs(normal.y) < 0.999F ? Math::Vector3UVE{0.0F, 1.0F, 0.0F}
                                                               : Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    const Math::Vector3UVE tangent = Math::CrossUVE(axis, normal);
    if (Math::LengthSquaredUVE(tangent) <= 0.00000001F) {
        return Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    }
    return Math::NormalizeUVE(tangent);
}

} // namespace

void GenerateMeshTangentsUVE(std::span<MeshVertexUVE> vertices, std::span<const std::uint32_t> indices) {
    std::vector<Math::Vector3UVE> tangentSums(vertices.size());
    std::vector<Math::Vector3UVE> bitangentSums(vertices.size());

    for (std::size_t indexOffset = 0; indexOffset + 2U < indices.size(); indexOffset += 3U) {
        const std::uint32_t firstIndex = indices[indexOffset];
        const std::uint32_t secondIndex = indices[indexOffset + 1U];
        const std::uint32_t thirdIndex = indices[indexOffset + 2U];
        if (firstIndex >= vertices.size() || secondIndex >= vertices.size() || thirdIndex >= vertices.size()) {
            continue;
        }

        const MeshVertexUVE& first = vertices[firstIndex];
        const MeshVertexUVE& second = vertices[secondIndex];
        const MeshVertexUVE& third = vertices[thirdIndex];
        const Math::Vector3UVE positionEdgeOne = second.position - first.position;
        const Math::Vector3UVE positionEdgeTwo = third.position - first.position;
        const float uEdgeOne = second.u - first.u;
        const float vEdgeOne = second.v - first.v;
        const float uEdgeTwo = third.u - first.u;
        const float vEdgeTwo = third.v - first.v;
        const float determinant = uEdgeOne * vEdgeTwo - vEdgeOne * uEdgeTwo;
        if (std::abs(determinant) <= 0.00000001F) {
            continue;
        }

        const float inverseDeterminant = 1.0F / determinant;
        const Math::Vector3UVE triangleTangent =
            (positionEdgeOne * vEdgeTwo - positionEdgeTwo * vEdgeOne) * inverseDeterminant;
        const Math::Vector3UVE triangleBitangent =
            (positionEdgeTwo * uEdgeOne - positionEdgeOne * uEdgeTwo) * inverseDeterminant;
        tangentSums[firstIndex] += triangleTangent;
        tangentSums[secondIndex] += triangleTangent;
        tangentSums[thirdIndex] += triangleTangent;
        bitangentSums[firstIndex] += triangleBitangent;
        bitangentSums[secondIndex] += triangleBitangent;
        bitangentSums[thirdIndex] += triangleBitangent;
    }

    for (std::size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex) {
        MeshVertexUVE& vertex = vertices[vertexIndex];
        const Math::Vector3UVE normal = Math::LengthSquaredUVE(vertex.normal) > 0.00000001F
                                            ? Math::NormalizeUVE(vertex.normal)
                                            : Math::Vector3UVE{0.0F, 1.0F, 0.0F};
        Math::Vector3UVE tangent = tangentSums[vertexIndex] - normal * Math::DotUVE(normal, tangentSums[vertexIndex]);
        if (Math::LengthSquaredUVE(tangent) <= 0.00000001F) {
            tangent = DeterministicTangentFallbackUVE(normal);
        } else {
            tangent = Math::NormalizeUVE(tangent);
        }

        vertex.tangent = tangent;
        vertex.tangentHandedness =
            Math::DotUVE(Math::CrossUVE(normal, tangent), bitangentSums[vertexIndex]) < 0.0F ? -1.0F : 1.0F;
    }
}

bool LoadMeshAssetUVE(const std::filesystem::path& path, MeshAssetUVE& outMesh) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value()) {
        return false; // ReadUveFileUVE already logged the specific reason.
    }
    if (file->first.assetType != AssetKindUVE::Mesh) {
        UVE_ERROR("MeshAssetUVE: \"{}\" is not a mesh file (asset type {})", path.string(),
                   static_cast<std::uint32_t>(file->first.assetType));
        return false;
    }

    const std::vector<std::byte>& payload = file->second;
    std::size_t offset = 0;

    std::uint32_t vertexCount = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, vertexCount)) {
        UVE_ERROR("MeshAssetUVE: \"{}\" has a truncated vertex count", path.string());
        return false;
    }

    std::vector<MeshVertexUVE> vertices;
    vertices.reserve(vertexCount);
    for (std::uint32_t index = 0; index < vertexCount; ++index) {
        MeshVertexUVE vertex;
        if (!ReadVector3FromBufferUVE(payload, offset, vertex.position) ||
            !ReadVector3FromBufferUVE(payload, offset, vertex.normal) ||
            !ReadFloatFromBufferUVE(payload, offset, vertex.u) || !ReadFloatFromBufferUVE(payload, offset, vertex.v)) {
            UVE_ERROR("MeshAssetUVE: \"{}\" has truncated vertex data", path.string());
            return false;
        }
        vertices.push_back(vertex);
    }

    std::uint32_t indexCount = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, indexCount)) {
        UVE_ERROR("MeshAssetUVE: \"{}\" has a truncated index count", path.string());
        return false;
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(indexCount);
    for (std::uint32_t index = 0; index < indexCount; ++index) {
        std::uint32_t value = 0;
        if (!ReadUint32FromBufferUVE(payload, offset, value)) {
            UVE_ERROR("MeshAssetUVE: \"{}\" has truncated index data", path.string());
            return false;
        }
        if (value >= vertices.size()) {
            UVE_ERROR("MeshAssetUVE: \"{}\" has an out-of-bounds index {} (only {} vertices)", path.string(), value,
                       vertices.size());
            return false;
        }
        indices.push_back(value);
    }

    Math::AabbUVE localBounds;
    if (!ReadVector3FromBufferUVE(payload, offset, localBounds.min) ||
        !ReadVector3FromBufferUVE(payload, offset, localBounds.max)) {
        UVE_ERROR("MeshAssetUVE: \"{}\" has a truncated local bounds", path.string());
        return false;
    }

    GenerateMeshTangentsUVE(vertices, indices);
    outMesh.vertices = std::move(vertices);
    outMesh.indices = std::move(indices);
    outMesh.localBounds = localBounds;
    return true;
}

bool SaveMeshAssetUVE(const MeshAssetUVE& mesh, const std::filesystem::path& path) {
    std::vector<std::byte> payload;
    AppendUint32UVE(payload, static_cast<std::uint32_t>(mesh.vertices.size()));
    for (const MeshVertexUVE& vertex : mesh.vertices) {
        AppendVector3UVE(payload, vertex.position);
        AppendVector3UVE(payload, vertex.normal);
        AppendFloatUVE(payload, vertex.u);
        AppendFloatUVE(payload, vertex.v);
    }

    AppendUint32UVE(payload, static_cast<std::uint32_t>(mesh.indices.size()));
    for (std::uint32_t index : mesh.indices) {
        AppendUint32UVE(payload, index);
    }

    AppendVector3UVE(payload, mesh.localBounds.min);
    AppendVector3UVE(payload, mesh.localBounds.max);

    return WriteUveFileUVE(path, AssetKindUVE::Mesh, payload);
}

} // namespace UVE::Asset
