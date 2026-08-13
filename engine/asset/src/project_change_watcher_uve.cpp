// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/project_change_watcher_uve.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_derived_artifact_cache_uve.h"

namespace UVE::Asset {
namespace {

struct BaselineEntryUVE final {
    std::filesystem::path relativePath{};
    ProjectFileEntryKindUVE kind = ProjectFileEntryKindUVE::File;
    std::optional<AssetGuidUVE> registeredAssetGuid;
    std::optional<AssetContentFingerprintUVE> contentFingerprint;
};

[[nodiscard]] std::optional<std::filesystem::path> MakeRootRelativePathUVE(const std::filesystem::path& path,
                                                                             const std::filesystem::path& root) {
    std::error_code errorCode;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode).lexically_normal();
    if (errorCode) {
        return std::nullopt;
    }

    auto rootIt = root.begin();
    auto pathIt = absolutePath.begin();
    for (; rootIt != root.end(); ++rootIt, ++pathIt) {
        if (pathIt == absolutePath.end() || *rootIt != *pathIt) {
            return std::nullopt;
        }
    }

    std::filesystem::path relativePath;
    for (; pathIt != absolutePath.end(); ++pathIt) {
        relativePath /= *pathIt;
    }
    if (relativePath.empty() || relativePath == ".") {
        return std::nullopt;
    }
    return relativePath.lexically_normal();
}

[[nodiscard]] bool PhysicalIdentityMatchesUVE(const BaselineEntryUVE& left,
                                              const BaselineEntryUVE& right) noexcept {
    return left.kind == right.kind && left.contentFingerprint == right.contentFingerprint;
}

[[nodiscard]] bool ChangeLessUVE(const ProjectFileChangeUVE& left,
                                 const ProjectFileChangeUVE& right) {
    if (left.relativePath.generic_string() != right.relativePath.generic_string()) {
        return left.relativePath.generic_string() < right.relativePath.generic_string();
    }
    if (left.kind != right.kind) {
        return static_cast<std::uint8_t>(left.kind) < static_cast<std::uint8_t>(right.kind);
    }
    return static_cast<std::uint8_t>(left.entryKind) < static_cast<std::uint8_t>(right.entryKind);
}

[[nodiscard]] std::optional<std::map<std::string, BaselineEntryUVE>>
BuildBaselineUVE(const std::filesystem::path& configuredContentRoot, const IAssetDatabaseUVE& assetDatabase,
                 std::string& outDiagnostic) {
    outDiagnostic.clear();
    std::map<std::string, BaselineEntryUVE> baseline;
    if (configuredContentRoot.empty()) {
        return baseline;
    }

    std::error_code errorCode;
    const std::filesystem::path absoluteContentRoot =
        std::filesystem::absolute(configuredContentRoot, errorCode).lexically_normal();
    if (errorCode) {
        outDiagnostic = "Unable to resolve the configured project content root: " + errorCode.message();
        return std::nullopt;
    }

    const std::filesystem::file_status rootStatus = std::filesystem::symlink_status(absoluteContentRoot, errorCode);
    if (errorCode && errorCode != std::errc::no_such_file_or_directory) {
        outDiagnostic = "Unable to inspect the project content root: " + errorCode.message();
        return std::nullopt;
    }
    if (errorCode == std::errc::no_such_file_or_directory || !std::filesystem::exists(rootStatus)) {
        return baseline;
    }
    if (std::filesystem::is_symlink(rootStatus)) {
        outDiagnostic = "The project content root is a symlink and cannot be watched.";
        return std::nullopt;
    }
    if (!std::filesystem::is_directory(rootStatus)) {
        outDiagnostic = "The project content root is not a directory.";
        return std::nullopt;
    }

    std::map<std::string, AssetGuidUVE> registeredAssetsByRelativePath;
    for (const AssetRecordUVE& record : assetDatabase.GetRegisteredAssetsUVE()) {
        const std::optional<std::filesystem::path> relativePath =
            MakeRootRelativePathUVE(record.path, absoluteContentRoot);
        if (relativePath.has_value()) {
            registeredAssetsByRelativePath.emplace(relativePath->generic_string(), record.guid);
        }
    }

    std::filesystem::recursive_directory_iterator iterator(
        absoluteContentRoot, std::filesystem::directory_options::skip_permission_denied, errorCode);
    if (errorCode) {
        outDiagnostic = "Unable to enumerate the project content root: " + errorCode.message();
        return std::nullopt;
    }

    const std::filesystem::recursive_directory_iterator end;
    while (iterator != end) {
        const std::filesystem::directory_entry entry = *iterator;
        const std::filesystem::file_status entryStatus = entry.symlink_status(errorCode);
        if (errorCode) {
            outDiagnostic = "Unable to inspect a project content entry: " + errorCode.message();
            return std::nullopt;
        }

        if (std::filesystem::is_symlink(entryStatus)) {
            iterator.disable_recursion_pending();
        } else if (std::filesystem::is_directory(entryStatus) || std::filesystem::is_regular_file(entryStatus)) {
            const std::optional<std::filesystem::path> relativePath =
                MakeRootRelativePathUVE(entry.path(), absoluteContentRoot);
            if (!relativePath.has_value()) {
                outDiagnostic = "A project content entry escaped the configured root boundary.";
                return std::nullopt;
            }

            BaselineEntryUVE baselineEntry;
            baselineEntry.relativePath = *relativePath;
            baselineEntry.kind = std::filesystem::is_directory(entryStatus) ? ProjectFileEntryKindUVE::Directory
                                                                            : ProjectFileEntryKindUVE::File;
            if (baselineEntry.kind == ProjectFileEntryKindUVE::File) {
                baselineEntry.contentFingerprint = ComputeAssetContentFingerprintUVE(entry.path());
                if (!baselineEntry.contentFingerprint.has_value()) {
                    outDiagnostic = "Unable to fingerprint a project content file.";
                    return std::nullopt;
                }
                const auto registeredIt = registeredAssetsByRelativePath.find(relativePath->generic_string());
                if (registeredIt != registeredAssetsByRelativePath.end()) {
                    baselineEntry.registeredAssetGuid = registeredIt->second;
                }
            }
            baseline.emplace(relativePath->generic_string(), std::move(baselineEntry));
        } else {
            iterator.disable_recursion_pending();
        }

        iterator.increment(errorCode);
        if (errorCode) {
            outDiagnostic = "Unable to continue project content enumeration: " + errorCode.message();
            return std::nullopt;
        }
    }
    return baseline;
}

} // namespace

struct ProjectChangeWatcherUVE::ImplUVE final {
    ImplUVE(std::filesystem::path configuredContentRoot, const double configuredPollIntervalSeconds,
            const std::size_t configuredJournalCapacity)
        : contentRoot(std::move(configuredContentRoot).lexically_normal()),
          pollIntervalSeconds(std::max(0.0, configuredPollIntervalSeconds)),
          journalCapacity(configuredJournalCapacity) {
        snapshot.contentRoot = contentRoot;
    }

    std::filesystem::path contentRoot;
    double pollIntervalSeconds = 0.0;
    double accumulatedSeconds = 0.0;
    std::size_t journalCapacity = 0U;
    bool baselineInitialized = false;
    std::map<std::string, BaselineEntryUVE> baseline;
    mutable std::mutex mutex;
    ProjectChangeSnapshotUVE snapshot;
};

ProjectChangeWatcherUVE::ProjectChangeWatcherUVE(std::filesystem::path contentRoot,
                                                 const double pollIntervalSeconds,
                                                 const std::size_t journalCapacity)
    : m_impl(std::make_unique<ImplUVE>(std::move(contentRoot), pollIntervalSeconds, journalCapacity)) {}

ProjectChangeWatcherUVE::~ProjectChangeWatcherUVE() = default;

bool ProjectChangeWatcherUVE::ScanNowLockedUVE(const IAssetDatabaseUVE& assetDatabase,
                                                IDerivedArtifactCacheUVE& derivedArtifactCache) {
    std::string diagnostic;
    std::optional<std::map<std::string, BaselineEntryUVE>> refreshedBaseline =
        BuildBaselineUVE(m_impl->contentRoot, assetDatabase, diagnostic);
    if (!refreshedBaseline.has_value()) {
        m_impl->snapshot.lastScanDiagnostic = std::move(diagnostic);
        return false;
    }

    if (!m_impl->baselineInitialized) {
        m_impl->baseline = std::move(*refreshedBaseline);
        m_impl->baselineInitialized = true;
        ++m_impl->snapshot.successfulScanGeneration;
        m_impl->snapshot.lastScanDiagnostic.reset();
        return true;
    }

    std::vector<ProjectFileChangeUVE> changes;
    for (const auto& [pathKey, previousEntry] : m_impl->baseline) {
        const auto currentIt = refreshedBaseline->find(pathKey);
        if (currentIt == refreshedBaseline->end()) {
            changes.push_back(ProjectFileChangeUVE{0U, previousEntry.relativePath, ProjectFileChangeKindUVE::Removed,
                                                   previousEntry.kind, previousEntry.registeredAssetGuid, 0U});
            continue;
        }
        if (!PhysicalIdentityMatchesUVE(previousEntry, currentIt->second)) {
            if (previousEntry.kind != currentIt->second.kind) {
                changes.push_back(ProjectFileChangeUVE{0U, previousEntry.relativePath,
                                                       ProjectFileChangeKindUVE::Removed, previousEntry.kind,
                                                       previousEntry.registeredAssetGuid, 0U});
                changes.push_back(ProjectFileChangeUVE{0U, currentIt->second.relativePath,
                                                       ProjectFileChangeKindUVE::Created, currentIt->second.kind,
                                                       currentIt->second.registeredAssetGuid, 0U});
            } else {
                changes.push_back(ProjectFileChangeUVE{0U, currentIt->second.relativePath,
                                                       ProjectFileChangeKindUVE::Modified, currentIt->second.kind,
                                                       currentIt->second.registeredAssetGuid, 0U});
            }
        }
    }
    for (const auto& [pathKey, currentEntry] : *refreshedBaseline) {
        if (!m_impl->baseline.contains(pathKey)) {
            changes.push_back(ProjectFileChangeUVE{0U, currentEntry.relativePath, ProjectFileChangeKindUVE::Created,
                                                   currentEntry.kind, currentEntry.registeredAssetGuid, 0U});
        }
    }
    std::sort(changes.begin(), changes.end(), ChangeLessUVE);

    for (ProjectFileChangeUVE& change : changes) {
        if (change.entryKind == ProjectFileEntryKindUVE::File) {
            change.staleArtifactCount = derivedArtifactCache.MarkStaleForSourceUVE(
                m_impl->contentRoot / change.relativePath);
        }
        change.sequence = ++m_impl->snapshot.latestSequence;
        if (m_impl->journalCapacity == 0U) {
            m_impl->snapshot.rescanRequired = true;
            continue;
        }
        if (m_impl->snapshot.changes.size() >= m_impl->journalCapacity) {
            m_impl->snapshot.changes.erase(m_impl->snapshot.changes.begin());
            m_impl->snapshot.rescanRequired = true;
        }
        m_impl->snapshot.changes.push_back(std::move(change));
    }

    m_impl->baseline = std::move(*refreshedBaseline);
    ++m_impl->snapshot.successfulScanGeneration;
    m_impl->snapshot.lastScanDiagnostic.reset();
    return true;
}

bool ProjectChangeWatcherUVE::PollUVE(const double deltaTimeSeconds, const IAssetDatabaseUVE& assetDatabase,
                                      IDerivedArtifactCacheUVE& derivedArtifactCache) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->accumulatedSeconds += std::max(0.0, deltaTimeSeconds);
    if (m_impl->baselineInitialized && m_impl->pollIntervalSeconds > 0.0 &&
        m_impl->accumulatedSeconds < m_impl->pollIntervalSeconds) {
        return false;
    }
    m_impl->accumulatedSeconds = 0.0;
    return ScanNowLockedUVE(assetDatabase, derivedArtifactCache);
}

bool ProjectChangeWatcherUVE::PollNowUVE(const IAssetDatabaseUVE& assetDatabase,
                                         IDerivedArtifactCacheUVE& derivedArtifactCache) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->accumulatedSeconds = 0.0;
    return ScanNowLockedUVE(assetDatabase, derivedArtifactCache);
}

ProjectChangeSnapshotUVE ProjectChangeWatcherUVE::GetSnapshotUVE() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->snapshot;
}

void ProjectChangeWatcherUVE::AcknowledgeThroughUVE(const std::uint64_t sequence) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto firstUnacknowledged = std::remove_if(
        m_impl->snapshot.changes.begin(), m_impl->snapshot.changes.end(),
        [sequence](const ProjectFileChangeUVE& change) { return change.sequence <= sequence; });
    m_impl->snapshot.changes.erase(firstUnacknowledged, m_impl->snapshot.changes.end());
}

void ProjectChangeWatcherUVE::AcknowledgeRescanUVE() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->snapshot.rescanRequired = false;
    m_impl->snapshot.changes.clear();
}

} // namespace UVE::Asset
