// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw MTL source importer. The importer reads a caller-owned filesystem
/// path, converts one supported material block through ConvertMtlMaterialUVE, persists one validated
/// MaterialAssetUVE as a .uvemat destination, and leaves AssetImporterUVE responsible for database
/// registration. It owns no texture/shader resolution, VFS mounts, GPU resources, background work,
/// or asset-manager state.
void RegisterMtlImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
