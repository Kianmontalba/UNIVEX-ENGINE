//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <vector>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

/// One asset to pack into a bundle: its GUID (used as the on-disk filename when unpacked) and
/// the loose file on disk to read its bytes from.
struct AssetBundleEntryUVE {
    AssetGuidUVE guid;
    std::filesystem::path sourcePath;
};

/// IAssetBundleUVE packs multiple assets' raw file bytes into a single `.uvebundle` file (the
/// spec's "pack assets for builds - streaming, DLC", Part 7.4) and unpacks one back into loose
/// files. Deliberately does not integrate with IAssetManagerUVE this increment — transparently
/// loading an asset from inside a mounted bundle (so game code never cares whether an asset is
/// loose or bundled) needs a virtual file system, which is Part 7.8's FileSystemUVE and doesn't
/// exist yet. Bundling stands alone: pack N assets, unpack them, get byte-identical files back.
/// Thread-safety: not thread-safe — matches SceneSerializerUVE's own "not thread-safe, stateless,
/// dependencies passed per-call" contract; Pack/Unpack both do direct file I/O with no shared
/// state to guard.
class IAssetBundleUVE {
public:
    virtual ~IAssetBundleUVE() = default;

    /// Packs every entry's file bytes into `bundlePath` as a single `.uve*` envelope
    /// (`AssetKindUVE::Bundle`). Returns false (logging the reason) if any source file can't be
    /// opened, or if the bundle file itself can't be written — never partially writes a corrupt
    /// bundle (the whole payload is built in memory first).
    [[nodiscard]] virtual bool PackUVE(const std::vector<AssetBundleEntryUVE>& entries,
                                        const std::filesystem::path& bundlePath) = 0;

    /// Extracts every entry from `bundlePath` into `outputDirectory`, one file per entry named
    /// after its GUID's hex string, creating `outputDirectory` if it doesn't exist. Returns false
    /// (logging the reason) on any failure — bad magic, truncated/malformed bundle contents, a
    /// non-Bundle asset type, or a file-system write error.
    [[nodiscard]] virtual bool UnpackUVE(const std::filesystem::path& bundlePath,
                                          const std::filesystem::path& outputDirectory) = 0;
};

} // namespace UVE::Asset
