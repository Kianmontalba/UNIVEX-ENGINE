//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Asset {

/// Opaque handle to one active mount, returned by MountDirectoryUVE()/MountBundleUVE() and
/// consumed by UnmountUVE(). Never zero (the sentinel "no mount" value); never reused within one
/// IFileSystemUVE instance's lifetime.
using MountHandleUVE = std::uint32_t;

/// IFileSystemUVE is the engine's virtual file system (the spec's Part 7.8 "Cross-platform file
/// I/O, virtual file system (VFS)"): zero or more mounts (real directories, or `.uvebundle`
/// archives) are searched, in descending-priority order, to resolve a virtual path. A virtual
/// path is a forward-slash-separated string with no leading slash (e.g.
/// `"textures/rock.uvetex"`); mount prefixes use the same shape (`""` = root). A virtual path
/// matches a mount iff it equals the mount's prefix or starts with `prefix + "/"` — matching is
/// always on whole path segments, so a mount at `"tex"` never matches `"textures/rock.uvetex"`.
/// This lets a loose directory mount transparently override (or be overridden by) a bundle
/// mount at the same or a covering prefix, simply by priority — the classic PAK-file/mod-override
/// pattern. Deliberately standalone this increment: `AssetDatabaseUVE`, `uve_file_envelope_uve`,
/// `AssetImporterUVE`, and `SceneSerializerUVE` are unaffected and still take a raw
/// `std::filesystem::path` directly — this VFS proves itself against `AssetBundleUVE` only; wider
/// adoption is future-increment work.
/// Thread-safety: thread-safe. Every method is guarded by an internal mutex, matching
/// `AssetDatabaseUVE`'s contract.
class IFileSystemUVE {
public:
    virtual ~IFileSystemUVE() = default;

    /// Mounts `realDirectory` under `virtualPrefix` for both reading and writing. Returns a
    /// handle for later UnmountUVE(). Does not require `realDirectory` to exist yet (a mount can
    /// be created before its target directory is, e.g. for a not-yet-populated user-data mount).
    /// The returned handle is intentionally not `[[nodiscard]]` (matching
    /// `IEventSystemUVE::SubscribeUVE()`'s precedent) — a caller mounting for the lifetime of the
    /// filesystem instance (e.g. a permanent content directory) has no need to keep it.
    virtual MountHandleUVE MountDirectoryUVE(std::string virtualPrefix, std::filesystem::path realDirectory,
                                              int priority) = 0;

    /// Mounts the entries inside the `.uvebundle` at `bundlePath` under `virtualPrefix`, keyed by
    /// each entry's `AssetBundleEntryUVE::virtualName` (or its GUID's hex string, if it had
    /// none). Read-only — WriteFileUVE() never targets a bundle mount. `bundlePath` is validated
    /// lazily on each read (not at mount time), matching `AssetBundleUVE`'s own error-handling
    /// contract.
    virtual MountHandleUVE MountBundleUVE(std::string virtualPrefix, std::filesystem::path bundlePath,
                                           int priority) = 0;

    /// Removes the mount identified by `handle`. A handle already unmounted (or never valid) is
    /// a safe no-op.
    virtual void UnmountUVE(MountHandleUVE handle) = 0;

    /// True iff `virtualPath` resolves against at least one current mount.
    [[nodiscard]] virtual bool HasFileUVE(std::string_view virtualPath) const = 0;

    /// Resolves `virtualPath` against every matching mount in descending-priority order and
    /// returns the first hit's bytes. Returns `std::nullopt` (logging the reason) if no mount
    /// resolves it, or if the resolving mount's underlying file/bundle can't actually be read.
    [[nodiscard]] virtual std::optional<std::vector<std::byte>> ReadFileUVE(std::string_view virtualPath) const = 0;

    /// Writes `data` to `virtualPath` inside the highest-priority Directory mount whose prefix
    /// matches (Bundle mounts are never write targets). Returns false (logging the reason) if no
    /// Directory mount matches, or if the underlying file can't be written.
    [[nodiscard]] virtual bool WriteFileUVE(std::string_view virtualPath, const std::vector<std::byte>& data) = 0;

    /// Returns the real on-disk path `virtualPath` would resolve to, considering only Directory
    /// mounts (a Bundle-backed virtual path has no real path of its own). Returns an empty path
    /// if no Directory mount matches.
    [[nodiscard]] virtual std::filesystem::path ResolveRealPathUVE(std::string_view virtualPath) const = 0;
};

} // namespace UVE::Asset
