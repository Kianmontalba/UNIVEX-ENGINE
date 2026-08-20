// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Registers the bounded raw OBJ source importer. The importer reads a caller-owned filesystem path,
/// converts supported source forms through ConvertObjMeshUVE, persists one validated MeshAssetUVE as
/// a .uvemodel destination, and leaves AssetImporterUVE responsible for database registration. It
/// owns no MTL files, VFS mounts, GPU resources, background work, or asset-manager state.
void RegisterObjImporterUVE(IAssetImporterUVE& importer);

} // namespace UVE::Asset
