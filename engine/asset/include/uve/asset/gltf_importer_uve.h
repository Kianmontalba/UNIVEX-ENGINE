// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw glTF/GLB source importer. It supports one mesh with one TRIANGLES
/// primitive, one buffer, POSITION plus optional NORMAL/TEXCOORD_0 and indices, safe relative or
/// data-URI buffer bytes, and persists one validated MeshAssetUVE as a .uvemodel destination. It
/// leaves AssetImporterUVE responsible for database registration and owns no scene assembly, materials,
/// images, skins, LODs, VFS mounts, GPU resources, or background work.
void RegisterGltfImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
