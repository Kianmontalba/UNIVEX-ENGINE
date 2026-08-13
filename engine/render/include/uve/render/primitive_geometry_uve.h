// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>

#include "uve/asset/mesh_asset_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"

namespace UVE::Render {

/// Immutable CPU-side geometry for one renderer-owned primitive. It deliberately uses the same
/// MeshVertexUVE binary layout as asset-backed meshes so the existing GPU vertex layout and tangent
/// generation path stay canonical.
struct PrimitiveGeometryUVE final {
    std::vector<Asset::MeshVertexUVE> vertices;
    std::vector<std::uint32_t> indices;
    Math::AabbUVE localBounds;
};

/// Returns deterministic unit-scale primitive geometry. Cube and Plane use a half extent of 0.5;
/// UV Sphere uses radius 0.5 with a duplicated UV seam and per-sector pole vertices. The returned
/// cache is immutable, process-local, and never serialized into a scene document.
[[nodiscard]] const PrimitiveGeometryUVE& GetPrimitiveGeometryUVE(Scene::PrimitiveMeshKindUVE kind) noexcept;

} // namespace UVE::Render
