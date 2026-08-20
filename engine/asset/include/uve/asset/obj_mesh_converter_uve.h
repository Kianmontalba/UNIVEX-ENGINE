// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "uve/asset/mesh_asset_uve.h"

namespace UVE::Asset {

inline constexpr std::size_t kMaximumObjMeshSourceBytesUVE = 64U * 1024U * 1024U;
inline constexpr std::uint32_t kMaximumObjMeshVerticesUVE = 1'000'000U;

/// Converts a bounded OBJ source into a failure-atomic CPU MeshAssetUVE. The v1 path supports
/// position/UV/normal declarations, `v`, `v/vt`, `v//vn`, and `v/vt/vn` face tokens, fan-triangulates
/// polygons, computes finite fallback face normals when `vn` is absent, and publishes copied local
/// bounds. It does not load MTL files, resolve external textures, preserve groups/materials, or own
/// filesystem, importer-registration, or GPU resources.
[[nodiscard]] bool ConvertObjMeshUVE(std::string_view source, MeshAssetUVE& outMesh);

} // namespace UVE::Asset
