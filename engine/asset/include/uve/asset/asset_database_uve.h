// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/asset/i_asset_database_uve.h"

namespace UVE::Asset {

/// AssetDatabaseUVE is the concrete, engine-standard implementation of IAssetDatabaseUVE,
/// backed by nlohmann::json exactly like ConfigManagerUVE (engine/config/) — the JSON library
/// is entirely confined to asset_database_uve.cpp via the private ImplUVE PIMPL, so this header
/// (and every consumer) never sees nlohmann::json. The on-disk format is a flat JSON object
/// mapping each GUID's hex string to its registered file path.
/// Thread-safety: thread-safe. Every method is guarded by an internal mutex — safe to call
/// concurrently from any thread.
class AssetDatabaseUVE final : public IAssetDatabaseUVE {
public:
    AssetDatabaseUVE();
    ~AssetDatabaseUVE() override;

    AssetDatabaseUVE(const AssetDatabaseUVE&) = delete;
    AssetDatabaseUVE& operator=(const AssetDatabaseUVE&) = delete;

    bool LoadUVE(const std::filesystem::path& path) override;
    bool SaveUVE() override;
    bool SaveUVE(const std::filesystem::path& path) override;

    [[nodiscard]] AssetGuidUVE RegisterUVE(const std::filesystem::path& assetPath) override;
    [[nodiscard]] std::filesystem::path ResolveUVE(AssetGuidUVE guid) const override;
    [[nodiscard]] bool HasGuidUVE(AssetGuidUVE guid) const override;
    [[nodiscard]] std::vector<AssetRecordUVE> GetRegisteredAssetsUVE() const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
