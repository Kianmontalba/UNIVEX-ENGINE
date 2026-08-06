// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
    [[nodiscard]] bool HasEntryUVE(const std::filesystem::path& bundlePath,
                                    std::string_view entryName) const override;
    [[nodiscard]] std::optional<std::vector<std::byte>>
    ReadEntryUVE(const std::filesystem::path& bundlePath, std::string_view entryName) const override;
};

} // namespace UVE::Asset
