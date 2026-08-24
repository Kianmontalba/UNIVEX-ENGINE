// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw TGA source importer for `.tga`. The importer owns only capped source
/// reading, TGA-to-RGBA8 decoding, and atomic `.uvetex` publication; AssetImporterUVE owns database
/// registration.
void RegisterTgaImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
