//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/asset/i_asset_bundle_uve.h"

namespace UVE::Asset {

/// AssetBundleUVE is the concrete, engine-standard implementation of IAssetBundleUVE. Holds no
/// persistent members of its own (same "stateless, dependencies passed per-call" shape as
/// SceneSerializerUVE) — the payload layout (entry count, then per-entry
/// guid/name/data-length/data) is entirely private to asset_bundle_uve.cpp.
class AssetBundleUVE final : public IAssetBundleUVE {
public:
    AssetBundleUVE() = default;

    [[nodiscard]] bool PackUVE(const std::vector<AssetBundleEntryUVE>& entries,
                                const std::filesystem::path& bundlePath) override;
    [[nodiscard]] bool UnpackUVE(const std::filesystem::path& bundlePath,
                                  const std::filesystem::path& outputDirectory) override;
};

} // namespace UVE::Asset
