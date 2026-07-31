//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/mesh_asset_uve.h"

#include <cstring>

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

} // namespace

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
