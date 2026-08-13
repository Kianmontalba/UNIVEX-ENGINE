// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>

#include "uve/asset/i_project_change_watcher_uve.h"

namespace UVE::Asset {

/// Portable, interval-driven project-content change journal. The watcher compares complete
/// symlink-safe snapshots only when due, publishes ordered copied deltas, and marks derived cache
/// records stale for observed regular-file source changes. It owns no OS watcher handle, importer,
/// runtime reload policy, or ProjectFileIndexUVE; native backends may later implement the same
/// interface. Thread-safe: all state is protected by implementation-private synchronization.
class ProjectChangeWatcherUVE final : public IProjectChangeWatcherUVE {
public:
    ProjectChangeWatcherUVE(std::filesystem::path contentRoot, double pollIntervalSeconds,
                            std::size_t journalCapacity);
    ~ProjectChangeWatcherUVE() override;

    ProjectChangeWatcherUVE(const ProjectChangeWatcherUVE&) = delete;
    ProjectChangeWatcherUVE& operator=(const ProjectChangeWatcherUVE&) = delete;

    [[nodiscard]] bool PollUVE(double deltaTimeSeconds, const IAssetDatabaseUVE& assetDatabase,
                                IDerivedArtifactCacheUVE& derivedArtifactCache) override;
    [[nodiscard]] bool PollNowUVE(const IAssetDatabaseUVE& assetDatabase,
                                   IDerivedArtifactCacheUVE& derivedArtifactCache) override;
    [[nodiscard]] ProjectChangeSnapshotUVE GetSnapshotUVE() const override;
    void AcknowledgeThroughUVE(std::uint64_t sequence) override;
    void AcknowledgeRescanUVE() override;

private:
    /// Performs one complete baseline comparison while m_impl's mutex is already held. Both
    /// interval-driven and explicit polls share this routine so they cannot drift semantically.
    [[nodiscard]] bool ScanNowLockedUVE(const IAssetDatabaseUVE& assetDatabase,
                                        IDerivedArtifactCacheUVE& derivedArtifactCache);

    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
