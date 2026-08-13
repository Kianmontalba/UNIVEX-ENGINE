// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

class IAssetDatabaseUVE;

/// Describes the physical kind of one project-content entry retained by a
/// ProjectFileSnapshotUVE. Symlinks are deliberately absent: ProjectFileIndexUVE
/// never follows or exposes them in order to preserve its configured root boundary.
enum class ProjectFileEntryKindUVE : std::uint8_t {
    Directory,
    File,
};

/// Immutable presentation data for one ordinary directory or file physically
/// enumerated beneath the configured project-content root. relativePath is
/// normalized relative to that root; registeredAssetGuid is populated only when
/// the existing AssetDatabase contains the same normalized in-root path.
struct ProjectFileEntryUVE final {
    std::filesystem::path relativePath;
    ProjectFileEntryKindUVE kind = ProjectFileEntryKindUVE::File;
    std::optional<AssetGuidUVE> registeredAssetGuid;
};

/// A copied, deterministic project-content tree snapshot. A missing content
/// root is a valid successfully refreshed empty snapshot; inaccessible roots or
/// iterator failures retain the prior snapshot and cause RefreshUVE() to return
/// false. refreshGeneration increases only after a successful replacement.
struct ProjectFileSnapshotUVE final {
    std::filesystem::path contentRoot;
    std::vector<ProjectFileEntryUVE> entries;
    bool contentRootExists = false;
    std::uint64_t refreshGeneration = 0;
};

/// Read-only project-content index used by the editor Asset Browser. Refreshes
/// occur only on explicit caller request; reads return copied cached snapshots
/// and never enumerate the filesystem. Implementations must not create,
/// import, rename, delete, write, or follow symlinks anywhere under the root.
/// Thread-safety: callers may refresh and read concurrently.
class IProjectFileIndexUVE {
public:
    virtual ~IProjectFileIndexUVE() = default;

    /// Re-enumerates ordinary files/directories directly under contentRoot and
    /// its ordinary subdirectories, correlating the result with a copied
    /// AssetDatabase registry snapshot. Returns true only when the cached
    /// snapshot was successfully replaced. A non-existent root is a successful
    /// empty snapshot; symlink roots, inaccessible roots, and traversal errors
    /// return false without replacing the previous successful snapshot.
    virtual bool RefreshUVE(const IAssetDatabaseUVE& assetDatabase) = 0;

    /// Returns a copied cached snapshot without filesystem I/O. Thread-safe.
    [[nodiscard]] virtual ProjectFileSnapshotUVE GetSnapshotUVE() const = 0;
};

} // namespace UVE::Asset
