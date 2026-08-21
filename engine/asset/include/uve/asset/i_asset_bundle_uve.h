// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

inline constexpr std::size_t kMaximumAssetBundleEntriesUVE = 4096U;
inline constexpr std::size_t kMaximumAssetBundleEntryNameBytesUVE = 256U;

/// One asset to pack into a bundle: its GUID, the loose file on disk to read its bytes from, and
/// an optional `virtualName` used as the on-disk entry name (and, for `IFileSystemUVE`-mounted
/// bundles, the name a virtual path resolves against). Left empty, the entry's name falls back
/// to its GUID's hex string (`UnpackUVE`'s original, still-supported behavior).
struct AssetBundleEntryUVE {
    AssetGuidUVE guid;
    std::filesystem::path sourcePath;
    std::string virtualName;
};

/// IAssetBundleUVE packs multiple assets' raw file bytes into a single `.uvebundle` file (the
/// spec's "pack assets for builds - streaming, DLC", Part 7.4) and unpacks one back into loose
/// files, or reads a single entry's bytes directly without unpacking. `PackUVE`/`UnpackUVE` are
/// the whole-bundle operations; `HasEntryUVE`/`ReadEntryUVE` are the on-demand, no-temp-files
/// primitives `IFileSystemUVE`'s bundle-backed mounts use to serve one virtual path's bytes.
/// Thread-safety: not thread-safe — matches SceneSerializerUVE's own "not thread-safe, stateless,
/// dependencies passed per-call" contract; every method does direct file I/O with no shared
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
    /// after its `virtualName` (or its GUID's hex string, if empty), creating `outputDirectory`
    /// if it doesn't exist. Returns false (logging the reason) on any failure — bad magic,
    /// truncated/malformed bundle contents, a non-Bundle asset type, or a file-system write error.
    [[nodiscard]] virtual bool UnpackUVE(const std::filesystem::path& bundlePath,
                                          const std::filesystem::path& outputDirectory) = 0;

    /// True iff `bundlePath` is a valid bundle containing an entry named `entryName`. Returns
    /// false (logging the reason) if `bundlePath` itself is invalid/unreadable/not a bundle —
    /// never crashes.
    [[nodiscard]] virtual bool HasEntryUVE(const std::filesystem::path& bundlePath,
                                            std::string_view entryName) const = 0;

    /// Reads one entry's bytes directly from `bundlePath` by name, without extracting any other
    /// entry or writing anything to disk. Returns `std::nullopt` (logging the reason) if
    /// `bundlePath` is invalid/unreadable/not a bundle, or if no entry named `entryName` exists.
    [[nodiscard]] virtual std::optional<std::vector<std::byte>>
    ReadEntryUVE(const std::filesystem::path& bundlePath, std::string_view entryName) const = 0;
};

} // namespace UVE::Asset
