// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/project_file_index_uve.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "uve/asset/i_asset_database_uve.h"

namespace UVE::Asset {
namespace {

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

[[nodiscard]] bool ProjectFileEntryLessUVE(const ProjectFileEntryUVE& left, const ProjectFileEntryUVE& right) {
    if (left.kind != right.kind) {
        return left.kind == ProjectFileEntryKindUVE::Directory;
    }
    return left.relativePath.generic_string() < right.relativePath.generic_string();
}

[[nodiscard]] std::filesystem::path NormalizeProjectContentRootUVE(std::filesystem::path path) {
    path = std::move(path).lexically_normal();
    // `lexically_normal()` deliberately retains a trailing separator on a relative directory such
    // as `assets/`. Remove only that empty final component so path iteration cannot mistake it for
    // a root-boundary mismatch, while preserving absolute filesystem roots such as `/`.
    if (path.has_relative_path() && path.filename().empty()) {
        path = path.parent_path();
    }
    return path;
}

} // namespace

struct ProjectFileIndexUVE::ImplUVE {
    explicit ImplUVE(std::filesystem::path configuredContentRoot)
        : contentRoot(NormalizeProjectContentRootUVE(std::move(configuredContentRoot))) {
        snapshot.contentRoot = contentRoot;
    }

    std::filesystem::path contentRoot;
    mutable std::mutex mutex;
    ProjectFileSnapshotUVE snapshot;
};

ProjectFileIndexUVE::ProjectFileIndexUVE(std::filesystem::path contentRoot)
    : m_impl(std::make_unique<ImplUVE>(std::move(contentRoot))) {}

ProjectFileIndexUVE::~ProjectFileIndexUVE() = default;

bool ProjectFileIndexUVE::RefreshUVE(const IAssetDatabaseUVE& assetDatabase) {
    const std::filesystem::path configuredContentRoot = m_impl->contentRoot;
    ProjectFileSnapshotUVE refreshedSnapshot;
    refreshedSnapshot.contentRoot = configuredContentRoot;

    // An empty configured root never means the process working directory. It is
    // a deliberate disabled/empty browser state, identical to a missing root.
    if (configuredContentRoot.empty()) {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        refreshedSnapshot.refreshGeneration = m_impl->snapshot.refreshGeneration + 1U;
        m_impl->snapshot = std::move(refreshedSnapshot);
        return true;
    }

    std::error_code errorCode;
    const std::filesystem::path absoluteContentRoot =
        std::filesystem::absolute(configuredContentRoot, errorCode).lexically_normal();
    if (errorCode) {
        return false;
    }

    const std::filesystem::file_status rootStatus = std::filesystem::symlink_status(absoluteContentRoot, errorCode);
    if (errorCode && errorCode != std::errc::no_such_file_or_directory) {
        return false;
    }
    if (errorCode == std::errc::no_such_file_or_directory || !std::filesystem::exists(rootStatus)) {
        errorCode.clear();
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        refreshedSnapshot.refreshGeneration = m_impl->snapshot.refreshGeneration + 1U;
        m_impl->snapshot = std::move(refreshedSnapshot);
        return true;
    }
    if (std::filesystem::is_symlink(rootStatus) || !std::filesystem::is_directory(rootStatus)) {
        return false;
    }
    refreshedSnapshot.contentRootExists = true;

    std::map<std::string, AssetGuidUVE> registeredAssetsByRelativePath;
    for (const AssetRecordUVE& record : assetDatabase.GetRegisteredAssetsUVE()) {
        const std::optional<std::filesystem::path> relativePath =
            MakeRootRelativePathUVE(record.path, absoluteContentRoot);
        if (!relativePath.has_value()) {
            continue;
        }
        registeredAssetsByRelativePath.emplace(relativePath->generic_string(), record.guid);
    }

    std::filesystem::recursive_directory_iterator iterator(
        absoluteContentRoot, std::filesystem::directory_options::skip_permission_denied, errorCode);
    if (errorCode) {
        return false;
    }

    const std::filesystem::recursive_directory_iterator end;
    while (iterator != end) {
        const std::filesystem::directory_entry entry = *iterator;
        const std::filesystem::file_status entryStatus = entry.symlink_status(errorCode);
        if (errorCode) {
            return false;
        }

        if (std::filesystem::is_symlink(entryStatus)) {
            iterator.disable_recursion_pending();
        } else if (std::filesystem::is_directory(entryStatus) || std::filesystem::is_regular_file(entryStatus)) {
            const std::optional<std::filesystem::path> relativePath =
                MakeRootRelativePathUVE(entry.path(), absoluteContentRoot);
            if (!relativePath.has_value()) {
                return false;
            }

            ProjectFileEntryUVE indexedEntry;
            indexedEntry.relativePath = *relativePath;
            indexedEntry.kind = std::filesystem::is_directory(entryStatus) ? ProjectFileEntryKindUVE::Directory
                                                                            : ProjectFileEntryKindUVE::File;
            if (indexedEntry.kind == ProjectFileEntryKindUVE::File) {
                const auto registeredIt = registeredAssetsByRelativePath.find(indexedEntry.relativePath.generic_string());
                if (registeredIt != registeredAssetsByRelativePath.end()) {
                    indexedEntry.registeredAssetGuid = registeredIt->second;
                }
            }
            refreshedSnapshot.entries.push_back(std::move(indexedEntry));
        } else {
            iterator.disable_recursion_pending();
        }

        iterator.increment(errorCode);
        if (errorCode) {
            return false;
        }
    }

    std::sort(refreshedSnapshot.entries.begin(), refreshedSnapshot.entries.end(), ProjectFileEntryLessUVE);
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        refreshedSnapshot.refreshGeneration = m_impl->snapshot.refreshGeneration + 1U;
        m_impl->snapshot = std::move(refreshedSnapshot);
    }
    return true;
}

ProjectFileSnapshotUVE ProjectFileIndexUVE::GetSnapshotUVE() const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->snapshot;
}

} // namespace UVE::Asset
