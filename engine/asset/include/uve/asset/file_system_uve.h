//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <memory>

#include "uve/asset/i_asset_bundle_uve.h"
#include "uve/asset/i_file_system_uve.h"

namespace UVE::Asset {

/// FileSystemUVE is the concrete, engine-standard implementation of IFileSystemUVE. Bundle-backed
/// mounts read entries via the injected `IAssetBundleUVE&` (dependency injection, same shape as
/// `AssetManagerUVE(IThreadPoolUVE&, IEventSystemUVE&, ...)`) — no bundle-parsing logic is
/// duplicated here.
class FileSystemUVE final : public IFileSystemUVE {
public:
    explicit FileSystemUVE(IAssetBundleUVE& assetBundle);
    ~FileSystemUVE() override;

    FileSystemUVE(const FileSystemUVE&) = delete;
    FileSystemUVE& operator=(const FileSystemUVE&) = delete;

    MountHandleUVE MountDirectoryUVE(std::string virtualPrefix, std::filesystem::path realDirectory,
                                      int priority) override;
    MountHandleUVE MountBundleUVE(std::string virtualPrefix, std::filesystem::path bundlePath,
                                   int priority) override;
    void UnmountUVE(MountHandleUVE handle) override;

    [[nodiscard]] bool HasFileUVE(std::string_view virtualPath) const override;
    [[nodiscard]] std::optional<std::vector<std::byte>> ReadFileUVE(std::string_view virtualPath) const override;
    [[nodiscard]] bool WriteFileUVE(std::string_view virtualPath, const std::vector<std::byte>& data) override;
    [[nodiscard]] std::filesystem::path ResolveRealPathUVE(std::string_view virtualPath) const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
