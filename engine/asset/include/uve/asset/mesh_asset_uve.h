// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Asset {

/// One vertex of a MeshAssetUVE: position, normal, one UV set, and a tangent-space basis
/// direction. `tangent`/`tangentHandedness` are runtime-derived rather than serialized in the
/// current `.uvemodel` payload, preserving compatibility with existing meshes while providing the
/// canonical material shader the TBN data normal mapping needs. Skinned meshes/LODs remain
/// future-increment work.
struct MeshVertexUVE {
    Math::Vector3UVE position;
    Math::Vector3UVE normal;
    float u = 0.0F;
    float v = 0.0F;
    Math::Vector3UVE tangent{1.0F, 0.0F, 0.0F};
    float tangentHandedness = 1.0F;
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

/// Derives a normalized tangent and handedness for every vertex from indexed triangles and UVs.
/// Degenerate triangles/UVs receive a deterministic orthogonal fallback. Returns false before
/// mutating the caller-owned vertices when a finite triangle-derived tangent or bitangent
/// accumulator overflows; callers converting assets can therefore reject the candidate
/// failure-atomically. The function does not serialize data or mutate indices. Thread-safety:
/// operates only on caller-owned spans.
[[nodiscard]] bool TryGenerateMeshTangentsUVE(std::span<MeshVertexUVE> vertices,
                                               std::span<const std::uint32_t> indices);

/// Derives tangents through TryGenerateMeshTangentsUVE and applies deterministic orthogonal
/// fallbacks if malformed runtime geometry cannot produce a finite tangent basis. Runtime callers
/// that can reject an asset should use TryGenerateMeshTangentsUVE instead.
void GenerateMeshTangentsUVE(std::span<MeshVertexUVE> vertices, std::span<const std::uint32_t> indices);

/// Loads `path` as a `.uve*` envelope with `AssetKindUVE::Mesh`, filling `outMesh`. Returns false
/// (logging the reason) if the file is missing/malformed, isn't actually a Mesh asset, its
/// serialized vertex/index counts do not fit the remaining payload before allocation, or its
/// index data is structurally invalid (any index `>= outMesh.vertices.size()`, which would cause
/// out-of-bounds reads once a future increment consumes this data). Candidate vectors and tangent
/// fields remain private until every validation step succeeds — matching the signature
/// `IAssetManagerUVE::RegisterLoaderUVE<MeshAssetUVE>()` expects.
[[nodiscard]] bool LoadMeshAssetUVE(const std::filesystem::path& path, MeshAssetUVE& outMesh);

/// Writes `mesh` to `path` as a `.uve*` envelope with `AssetKindUVE::Mesh`. Returns false (logging
/// the reason) if the file can't be written.
[[nodiscard]] bool SaveMeshAssetUVE(const MeshAssetUVE& mesh, const std::filesystem::path& path);

} // namespace UVE::Asset
