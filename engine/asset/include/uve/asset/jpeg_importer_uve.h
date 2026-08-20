// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw JPEG source importer for `.jpg` and `.jpeg`. The importer reads capped
/// source bytes, decodes supported baseline/progressive JPEG through DecodeJpegRgba8ImageUVE, and
/// persists one validated RGBA8 TextureAssetUVE as a `.uvetex` destination. It leaves AssetImporterUVE
/// responsible for database registration and owns no VFS mounts, GPU resources, background work, or
/// texture compression/streaming policy.
void RegisterJpegImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
