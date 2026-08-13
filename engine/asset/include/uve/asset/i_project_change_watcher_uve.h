// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/i_project_file_index_uve.h"

namespace UVE::Asset {

class IAssetDatabaseUVE;
class IDerivedArtifactCacheUVE;

/// Physical project-content changes normalized by the portable watcher. A rename is deliberately
/// represented as a remove-plus-create pair in v1; no fragile platform-specific rename pairing is
/// inferred.
enum class ProjectFileChangeKindUVE : std::uint8_t {
    Created,
    Modified,
    Removed,
};

/// One copied project-content change. Removed entries retain `registeredAssetGuid` from the
/// watcher's own last-successful baseline so presentation never relies on a possibly stale current
/// AssetDatabaseUVE lookup after the file has disappeared.
struct ProjectFileChangeUVE final {
    std::uint64_t sequence = 0U;
    std::filesystem::path relativePath{};
    ProjectFileChangeKindUVE kind = ProjectFileChangeKindUVE::Modified;
    ProjectFileEntryKindUVE entryKind = ProjectFileEntryKindUVE::File;
    std::optional<AssetGuidUVE> registeredAssetGuid;
    std::size_t staleArtifactCount = 0U;
};

/// Immutable copied change-journal snapshot. `rescanRequired` remains set after journal retention
/// overflows until an editor caller successfully replaces the full ProjectFileIndex snapshot and
/// explicitly acknowledges the rescan boundary.
struct ProjectChangeSnapshotUVE final {
    std::filesystem::path contentRoot{};
    std::vector<ProjectFileChangeUVE> changes;
    std::uint64_t latestSequence = 0U;
    std::uint64_t successfulScanGeneration = 0U;
    bool rescanRequired = false;
    std::optional<std::string> lastScanDiagnostic;
};

/// Project-content polling journal distinct from IHotReloadUVE. It observes ordinary entries only
/// beneath one configured root, never follows symlinks, and can mark matching derived import
/// artifacts stale. It never calls an importer, reloads runtime assets, changes AssetDatabaseUVE,
/// mutates a scene, or performs filesystem content writes. Thread-safe: polling and snapshot/
/// acknowledgement calls may be made concurrently; snapshots are always copied.
class IProjectChangeWatcherUVE {
public:
    virtual ~IProjectChangeWatcherUVE() = default;

    /// Accumulates `deltaTimeSeconds` and performs at most one portable baseline scan when the
    /// configured interval has elapsed, or immediately when no baseline exists yet. Returns true
    /// only when a successful scan replaced the baseline; failed scans retain the last good
    /// baseline and report a copied diagnostic through GetSnapshotUVE().
    [[nodiscard]] virtual bool PollUVE(double deltaTimeSeconds, const IAssetDatabaseUVE& assetDatabase,
                                       IDerivedArtifactCacheUVE& derivedArtifactCache) = 0;

    /// Performs one immediate scan, bypassing interval accumulation. This exists for deterministic
    /// tests and explicit editor review; it does not refresh ProjectFileIndexUVE or enqueue imports.
    [[nodiscard]] virtual bool PollNowUVE(const IAssetDatabaseUVE& assetDatabase,
                                          IDerivedArtifactCacheUVE& derivedArtifactCache) = 0;

    /// Returns a copied journal and diagnostic snapshot without filesystem I/O. Thread-safe.
    [[nodiscard]] virtual ProjectChangeSnapshotUVE GetSnapshotUVE() const = 0;

    /// Removes retained ordinary journal records with sequence <= `sequence`. It never clears an
    /// overflow boundary (`rescanRequired`); only AcknowledgeRescanUVE() can do that after a full
    /// successful ProjectFileIndexUVE refresh.
    virtual void AcknowledgeThroughUVE(std::uint64_t sequence) = 0;

    /// Clears a prior journal-overflow boundary and its retained journal. Callers must invoke this
    /// only after a successful full project-index refresh; the watcher cannot itself refresh the
    /// index because it owns no ProjectFileIndexUVE dependency.
    virtual void AcknowledgeRescanUVE() = 0;
};

} // namespace UVE::Asset
