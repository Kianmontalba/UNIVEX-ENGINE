// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Asset {

/// One vertex of a MeshAssetUVE: position, normal, and a single UV set. Deliberately minimal —
/// no tangents or skin weights yet, even though the spec's `.uvemodel` format mentions them —
/// same "will be extended once a real consumer needs it" scope reduction already used for
/// `Scene::MeshComponentUVE`'s own placeholder doc comment. Skinned meshes/LODs are
/// future-increment work.
struct MeshVertexUVE {
    Math::Vector3UVE position;
    Math::Vector3UVE normal;
    float u = 0.0F;
    float v = 0.0F;
};

/// The CPU-side, engine-native representation of a `.uvemodel` asset (Part 2's file-format
/// table): triangle vertex/index data plus a precomputed local-space bounding box (used for
/// frustum culling once `MeshRendererUVE` exists, Increment 13). Purely CPU-side data — turning
/// this into GPU buffers is a future increment's concern (Renderer3DUVE's resource cache); this
/// type has no dependency on `engine/render` at all.
struct MeshAssetUVE {
    std::vector<MeshVertexUVE> vertices;
    std::vector<std::uint32_t> indices;
    Math::AabbUVE localBounds;
};

/// Loads `path` as a `.uve*` envelope with `AssetKindUVE::Mesh`, filling `outMesh`. Returns false
/// (logging the reason) if the file is missing/malformed, isn't actually a Mesh asset, or its
/// index data is structurally invalid (any index `>= outMesh.vertices.size()`, which would cause
/// out-of-bounds reads once a future increment consumes this data) — matches the signature
/// `IAssetManagerUVE::RegisterLoaderUVE<MeshAssetUVE>()` expects.
[[nodiscard]] bool LoadMeshAssetUVE(const std::filesystem::path& path, MeshAssetUVE& outMesh);

/// Writes `mesh` to `path` as a `.uve*` envelope with `AssetKindUVE::Mesh`. Returns false (logging
/// the reason) if the file can't be written.
[[nodiscard]] bool SaveMeshAssetUVE(const MeshAssetUVE& mesh, const std::filesystem::path& path);

} // namespace UVE::Asset
