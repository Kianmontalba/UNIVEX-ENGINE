// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_pipeline_uve.h"

#include "uve/asset/data_table_asset_manager_uve.h"
#include "uve/asset/data_table_importer_uve.h"

namespace UVE::Asset {

void RegisterDataTablePipelineUVE(IAssetImporterUVE& importer, IAssetManagerUVE& assetManager) {
    RegisterDataTableImportersUVE(importer);
    RegisterDataTableAssetLoaderUVE(assetManager);
}

} // namespace UVE::Asset
