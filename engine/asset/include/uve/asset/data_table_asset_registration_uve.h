// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <filesystem>
#include <optional>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

class IAssetDatabaseUVE;

/// Validates one envelope-backed `.uvetable` file and registers its lexical path in the supplied
/// asset database. Returns the existing or newly generated GUID; returns nullopt without changing
/// the database when the extension, envelope, asset kind, or table payload is invalid.
[[nodiscard]] std::optional<AssetGuidUVE> RegisterDataTableAssetUVE(IAssetDatabaseUVE& database,
                                                                     const std::filesystem::path& path);

} // namespace UVE::Asset
