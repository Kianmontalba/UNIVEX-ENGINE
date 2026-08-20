// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "uve/asset/gltf_metadata_uve.h"
#include "uve/asset/mesh_asset_uve.h"

namespace UVE::Asset {

enum class GltfComponentTypeUVE : std::uint32_t {
    UnsignedByte = 5121U,
    UnsignedShort = 5123U,
    UnsignedInt = 5125U,
    Float = 5126U,
};

struct GltfAccessorViewUVE final {
    std::span<const std::byte> buffer;
    std::uint64_t byteOffset = 0U;
    std::uint64_t elementCount = 0U;
    std::uint64_t byteStride = 0U;
    GltfComponentTypeUVE componentType = GltfComponentTypeUVE::Float;
};

struct GltfPrimitiveSourceUVE final {
    GltfAccessorViewUVE positions;
    std::optional<GltfAccessorViewUVE> normals;
    std::optional<GltfAccessorViewUVE> texcoords0;
    std::optional<GltfAccessorViewUVE> indices;
    std::uint32_t mode = 4U; // glTF TRIANGLES.
};

/// Converts one bounded caller-owned glTF TRIANGLES primitive into a failure-atomic MeshAssetUVE.
/// Positions, normals, and texcoords0 use float32 accessor views; indices may use unsigned byte,
/// unsigned short, or unsigned int views. Accessor spans are checked through the existing overflow-safe
/// validator. Missing normals receive deterministic face-normal accumulation, missing indices produce
/// sequential triangle indices, and local bounds are copied. The converter performs no JSON parsing,
/// URI/file resolution, material/image decoding, filesystem I/O, or GPU allocation.
[[nodiscard]] bool ConvertGltfPrimitiveUVE(const GltfPrimitiveSourceUVE& source, MeshAssetUVE& outMesh);

} // namespace UVE::Asset
