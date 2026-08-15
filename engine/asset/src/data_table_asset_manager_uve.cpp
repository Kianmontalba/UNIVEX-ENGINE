// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_asset_manager_uve.h"

#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/data_table_uve.h"
#include "uve/asset/i_asset_manager_uve.h"

namespace UVE::Asset {

void RegisterDataTableAssetLoaderUVE(IAssetManagerUVE& assetManager) {
    assetManager.RegisterLoaderUVE<DataTableUVE>(LoadDataTableAssetUVE);
}

} // namespace UVE::Asset
