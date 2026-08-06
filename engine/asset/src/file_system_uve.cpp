// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/file_system_uve.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <mutex>
#include <optional>
#include <system_error>
#include <utility>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

enum class MountKindUVE { Directory, Bundle };

struct MountRecordUVE {
    MountHandleUVE handle{};
    MountKindUVE kind{};
    std::string prefix;
    std::filesystem::path realPathOrBundlePath;
    int priority = 0;
};

/// True iff `virtualPath` matches `prefix` on a whole-segment boundary: either exactly equal, or
/// `virtualPath` starts with `prefix + "/"`. An empty `prefix` (root mount) matches everything.
[[nodiscard]] bool MatchesPrefixUVE(std::string_view virtualPath, std::string_view prefix) {
    if (prefix.empty()) {
        return true;
    }
    if (virtualPath.size() < prefix.size() || virtualPath.substr(0, prefix.size()) != prefix) {
        return false;
    }
    return virtualPath.size() == prefix.size() || virtualPath[prefix.size()] == '/';
}

/// Returns `virtualPath` with `prefix` (and any single separating '/') removed from the front.
[[nodiscard]] std::string_view StripPrefixUVE(std::string_view virtualPath, std::string_view prefix) {
    if (prefix.empty()) {
        return virtualPath;
    }
    std::string_view remainder = virtualPath.substr(prefix.size());
    if (!remainder.empty() && remainder.front() == '/') {
        remainder.remove_prefix(1);
    }
    return remainder;
}

/// Every mount whose prefix matches `virtualPath` (optionally restricted to `kindFilter`),
/// copied out and sorted by descending priority (ties broken by most-recently-mounted first).
[[nodiscard]] std::vector<MountRecordUVE> FindMatchingMountsUVE(const std::vector<MountRecordUVE>& mounts,
                                                                 std::string_view virtualPath,
                                                                 std::optional<MountKindUVE> kindFilter = {}) {
    std::vector<MountRecordUVE> matches;
    for (const MountRecordUVE& record : mounts) {
        if (kindFilter.has_value() && record.kind != *kindFilter) {
            continue;
        }
        if (MatchesPrefixUVE(virtualPath, record.prefix)) {
            matches.push_back(record);
        }
    }
    std::sort(matches.begin(), matches.end(), [](const MountRecordUVE& lhs, const MountRecordUVE& rhs) {
        if (lhs.priority != rhs.priority) {
            return lhs.priority > rhs.priority;
        }
        return lhs.handle > rhs.handle;
    });
    return matches;
}

/// Reads `path`'s full contents into a byte buffer. Returns `std::nullopt` if the file can't be
/// opened or a read error occurs partway through — never throws.
[[nodiscard]] std::optional<std::vector<std::byte>> ReadRealFileUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> data(static_cast<std::size_t>(size));
    if (!data.empty()) {
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) {
            return std::nullopt;
        }
    }
    return data;
}

} // namespace

struct FileSystemUVE::ImplUVE {
    IAssetBundleUVE& assetBundle;
    mutable std::mutex mutex;
    std::vector<MountRecordUVE> mounts;
    MountHandleUVE nextHandle = 1;
};

FileSystemUVE::FileSystemUVE(IAssetBundleUVE& assetBundle) : m_impl(std::make_unique<ImplUVE>(assetBundle)) {}

FileSystemUVE::~FileSystemUVE() = default;

MountHandleUVE FileSystemUVE::MountDirectoryUVE(std::string virtualPrefix, std::filesystem::path realDirectory,
                                                 int priority) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const MountHandleUVE handle = m_impl->nextHandle++;
    m_impl->mounts.push_back(MountRecordUVE{handle, MountKindUVE::Directory, std::move(virtualPrefix),
                                             std::move(realDirectory), priority});
    return handle;
}

MountHandleUVE FileSystemUVE::MountBundleUVE(std::string virtualPrefix, std::filesystem::path bundlePath,
                                              int priority) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const MountHandleUVE handle = m_impl->nextHandle++;
    m_impl->mounts.push_back(
        MountRecordUVE{handle, MountKindUVE::Bundle, std::move(virtualPrefix), std::move(bundlePath), priority});
    return handle;
}

void FileSystemUVE::UnmountUVE(MountHandleUVE handle) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::erase_if(m_impl->mounts, [handle](const MountRecordUVE& record) { return record.handle == handle; });
}

bool FileSystemUVE::HasFileUVE(std::string_view virtualPath) const {
    std::vector<MountRecordUVE> matches;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        matches = FindMatchingMountsUVE(m_impl->mounts, virtualPath);
    }

    for (const MountRecordUVE& record : matches) {
        const std::string_view remainder = StripPrefixUVE(virtualPath, record.prefix);
        if (record.kind == MountKindUVE::Directory) {
            const std::filesystem::path realPath = record.realPathOrBundlePath / std::string(remainder);
            if (std::filesystem::exists(realPath)) {
                return true;
            }
        } else if (m_impl->assetBundle.HasEntryUVE(record.realPathOrBundlePath, remainder)) {
            return true;
        }
    }
    return false;
}

std::optional<std::vector<std::byte>> FileSystemUVE::ReadFileUVE(std::string_view virtualPath) const {
    std::vector<MountRecordUVE> matches;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        matches = FindMatchingMountsUVE(m_impl->mounts, virtualPath);
    }

    for (const MountRecordUVE& record : matches) {
        const std::string_view remainder = StripPrefixUVE(virtualPath, record.prefix);
        if (record.kind == MountKindUVE::Directory) {
            const std::filesystem::path realPath = record.realPathOrBundlePath / std::string(remainder);
            std::optional<std::vector<std::byte>> data = ReadRealFileUVE(realPath);
            if (data.has_value()) {
                return data;
            }
        } else {
            std::optional<std::vector<std::byte>> data =
                m_impl->assetBundle.ReadEntryUVE(record.realPathOrBundlePath, remainder);
            if (data.has_value()) {
                return data;
            }
        }
    }

    UVE_ERROR("FileSystemUVE: no mount resolves virtual path \"{}\"", virtualPath);
    return std::nullopt;
}

bool FileSystemUVE::WriteFileUVE(std::string_view virtualPath, const std::vector<std::byte>& data) {
    std::vector<MountRecordUVE> matches;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        matches = FindMatchingMountsUVE(m_impl->mounts, virtualPath, MountKindUVE::Directory);
    }
    if (matches.empty()) {
        UVE_ERROR("FileSystemUVE: no writable (directory) mount for virtual path \"{}\"", virtualPath);
        return false;
    }

    const MountRecordUVE& target = matches.front();
    const std::string_view remainder = StripPrefixUVE(virtualPath, target.prefix);
    const std::filesystem::path realPath = target.realPathOrBundlePath / std::string(remainder);

    if (realPath.has_parent_path()) {
        std::error_code errorCode;
        std::filesystem::create_directories(realPath.parent_path(), errorCode);
        if (errorCode && !std::filesystem::exists(realPath.parent_path())) {
            UVE_ERROR("FileSystemUVE: failed to create directory \"{}\": {}", realPath.parent_path().string(),
                       errorCode.message());
            return false;
        }
    }

    std::ofstream file(realPath, std::ios::binary);
    if (!file.is_open()) {
        UVE_ERROR("FileSystemUVE: failed to open \"{}\" for writing", realPath.string());
        return false;
    }
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!file.good()) {
        UVE_ERROR("FileSystemUVE: failed to write \"{}\"", realPath.string());
        return false;
    }
    return true;
}

std::filesystem::path FileSystemUVE::ResolveRealPathUVE(std::string_view virtualPath) const {
    std::vector<MountRecordUVE> matches;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        matches = FindMatchingMountsUVE(m_impl->mounts, virtualPath, MountKindUVE::Directory);
    }

    for (const MountRecordUVE& record : matches) {
        const std::string_view remainder = StripPrefixUVE(virtualPath, record.prefix);
        const std::filesystem::path realPath = record.realPathOrBundlePath / std::string(remainder);
        if (std::filesystem::exists(realPath)) {
            return realPath;
        }
    }
    return {};
}

} // namespace UVE::Asset
