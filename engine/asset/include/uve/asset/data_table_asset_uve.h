// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <filesystem>

#include "uve/asset/data_table_uve.h"

namespace UVE::Asset {

/// Persists one validated DataTableUVE as a JSON payload inside the universal `.uve*` envelope.
/// SaveUVE never mutates the caller; LoadUVE validates into a temporary table before replacement.
/// The functions are synchronous and not thread-safe with respect to the supplied DataTableUVE.
[[nodiscard]] bool SaveDataTableAssetUVE(const DataTableUVE& table, const std::filesystem::path& path);

/// Loads a DataTableUVE only from an envelope tagged AssetKindUVE::DataTable. The destination is
/// unchanged when the file is missing, malformed, the wrong asset kind, or an invalid table.
[[nodiscard]] bool LoadDataTableAssetUVE(const std::filesystem::path& path, DataTableUVE& table);

} // namespace UVE::Asset
