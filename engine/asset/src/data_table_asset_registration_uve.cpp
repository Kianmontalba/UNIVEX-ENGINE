// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_asset_registration_uve.h"

#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

std::optional<AssetGuidUVE> RegisterDataTableAssetUVE(IAssetDatabaseUVE& database,
                                                      const std::filesystem::path& path) {
    if (path.empty() || path.extension() != ".uvetable") {
        UVE_ERROR("DataTableAssetRegistrationUVE: path must use the .uvetable extension");
        return std::nullopt;
    }

    DataTableUVE validatedTable;
    if (!LoadDataTableAssetUVE(path, validatedTable)) {
        UVE_ERROR("DataTableAssetRegistrationUVE: rejected invalid data-table asset at {}", path.string());
        return std::nullopt;
    }

    const AssetGuidUVE guid = database.RegisterUVE(path);
    if (guid == kInvalidAssetGuidUVE) {
        UVE_ERROR("DataTableAssetRegistrationUVE: database returned an invalid GUID for {}", path.string());
        return std::nullopt;
    }
    return guid;
}

} // namespace UVE::Asset
