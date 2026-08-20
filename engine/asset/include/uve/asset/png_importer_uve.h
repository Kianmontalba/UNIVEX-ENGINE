// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw PNG source importer. The importer reads a caller-owned filesystem path,
/// decodes supported PNG forms through DecodePngRgba8ImageUVE, persists one validated RGBA8
/// TextureAssetUVE as a .uvetex destination, and leaves AssetImporterUVE responsible for database
/// registration. It owns no VFS mounts, GPU resources, background work, or asset-manager state.
void RegisterPngImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
