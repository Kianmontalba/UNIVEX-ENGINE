// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers bounded raw shader-source importers for the unambiguous one-stage `.vert`, `.frag`,
/// and `.comp` extensions. Each source is copied into a validated `.uveshader` envelope with the
/// stage inferred from its extension; combined `.glsl` files remain outside this bridge because the
/// project convention uses them for dual-stage built-in programs. The importer owns no shader
/// compilation, include resolution, GPU resource, or asset-database state.
void RegisterShaderSourceImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
