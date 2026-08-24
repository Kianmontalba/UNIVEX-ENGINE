// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/derived_artifact_cache_uve.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace UVE::Asset {
namespace {

constexpr std::uint64_t kFnvOffsetBasisUVE = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrimeUVE = 1099511628211ULL;

[[nodiscard]] std::filesystem::path NormalizePathUVE(const std::filesystem::path& path) {
    std::error_code errorCode;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode);
    return errorCode ? path.lexically_normal() : absolutePath.lexically_normal();
}

[[nodiscard]] std::string ArtifactFileNameUVE(const std::filesystem::path& destinationPath) {
    const std::string identity = NormalizePathUVE(destinationPath).generic_string();
    std::uint64_t hash = kFnvOffsetBasisUVE;
    for (const char character : identity) {
        hash ^= static_cast<unsigned char>(character);
        hash *= kFnvPrimeUVE;
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str() + ".uveimportcache";
}

[[nodiscard]] nlohmann::json FingerprintToJsonUVE(const AssetContentFingerprintUVE& fingerprint) {
    return nlohmann::json{{"hash", fingerprint.hash}, {"byteCount", fingerprint.byteCount}};
}

[[nodiscard]] AssetContentFingerprintUVE FingerprintFromJsonUVE(const nlohmann::json& document) {
    return AssetContentFingerprintUVE{document.at("hash").get<std::uint64_t>(),
                                      document.at("byteCount").get<std::uint64_t>()};
}

[[nodiscard]] nlohmann::json RecordToJsonUVE(const DerivedArtifactCacheRecordUVE& record) {
    return nlohmann::json{{"schemaVersion", record.schemaVersion},
                          {"sourcePath", NormalizePathUVE(record.sourcePath).generic_string()},
                          {"destinationPath", NormalizePathUVE(record.destinationPath).generic_string()},
                          {"sourceFingerprint", FingerprintToJsonUVE(record.sourceFingerprint)},
                          {"destinationFingerprint", FingerprintToJsonUVE(record.destinationFingerprint)},
                          {"settingsVersion", record.settingsVersion},
                          {"assetGuid", record.assetGuid.value},
                          {"stale", record.stale}};
}

[[nodiscard]] std::optional<DerivedArtifactCacheRecordUVE>
RecordFromJsonUVE(const nlohmann::json& document) {
    try {
        DerivedArtifactCacheRecordUVE record;
        record.schemaVersion = document.at("schemaVersion").get<std::uint32_t>();
        if (record.schemaVersion != kDerivedArtifactCacheSchemaVersionUVE) {
            return std::nullopt;
        }
        record.sourcePath = NormalizePathUVE(document.at("sourcePath").get<std::string>());
        record.destinationPath = NormalizePathUVE(document.at("destinationPath").get<std::string>());
        record.sourceFingerprint = FingerprintFromJsonUVE(document.at("sourceFingerprint"));
        record.destinationFingerprint = FingerprintFromJsonUVE(document.at("destinationFingerprint"));
        record.settingsVersion = document.at("settingsVersion").get<std::string>();
        record.assetGuid = AssetGuidUVE{document.at("assetGuid").get<std::uint64_t>()};
        record.stale = document.value("stale", false);
        if (record.sourcePath.empty() || record.destinationPath.empty() || record.settingsVersion.empty() ||
            record.assetGuid == kInvalidAssetGuidUVE) {
            return std::nullopt;
        }
        return record;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

[[nodiscard]] bool WriteRecordAtomicallyUVE(const std::filesystem::path& artifactPath,
                                             const DerivedArtifactCacheRecordUVE& record) {
    std::filesystem::path temporaryPath = artifactPath;
    temporaryPath += ".tmp";
    std::error_code errorCode;
    std::filesystem::remove(temporaryPath, errorCode);
    errorCode.clear();

    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }
        file << RecordToJsonUVE(record).dump(2) << '\n';
        file.flush();
        if (!file.good()) {
            file.close();
            std::filesystem::remove(temporaryPath, errorCode);
            return false;
        }
    }

    std::filesystem::rename(temporaryPath, artifactPath, errorCode);
    if (errorCode) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

} // namespace

struct DerivedArtifactCacheUVE::ImplUVE final {
    explicit ImplUVE(std::filesystem::path configuredRoot) : cacheRoot(NormalizePathUVE(configuredRoot)) {}

    std::filesystem::path cacheRoot;
    mutable std::mutex mutex;
};

DerivedArtifactCacheUVE::DerivedArtifactCacheUVE(std::filesystem::path cacheRoot)
    : m_impl(std::make_unique<ImplUVE>(std::move(cacheRoot))) {}

DerivedArtifactCacheUVE::~DerivedArtifactCacheUVE() = default;

std::optional<DerivedArtifactCacheRecordUVE>
DerivedArtifactCacheUVE::LoadImportRecordUVE(const std::filesystem::path& destinationPath) const {
    if (destinationPath.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    const std::filesystem::path artifactPath = m_impl->cacheRoot / ArtifactFileNameUVE(destinationPath);
    std::ifstream file(artifactPath);
    if (!file.is_open()) {
        return std::nullopt;
    }

    try {
        nlohmann::json document;
        file >> document;
        std::optional<DerivedArtifactCacheRecordUVE> record = RecordFromJsonUVE(document);
        if (!record.has_value() || record->destinationPath != NormalizePathUVE(destinationPath)) {
            return std::nullopt;
        }
        return record;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

bool DerivedArtifactCacheUVE::StoreImportRecordUVE(const std::filesystem::path& destinationPath,
                                                    const DerivedArtifactCacheRecordUVE& record) {
    if (destinationPath.empty() || record.schemaVersion != kDerivedArtifactCacheSchemaVersionUVE ||
        record.sourcePath.empty() || record.destinationPath.empty() || record.settingsVersion.empty() ||
        record.assetGuid == kInvalidAssetGuidUVE || record.stale ||
        NormalizePathUVE(destinationPath) != NormalizePathUVE(record.destinationPath)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::error_code errorCode;
    const std::filesystem::file_status rootStatus = std::filesystem::symlink_status(m_impl->cacheRoot, errorCode);
    if (!errorCode && std::filesystem::is_symlink(rootStatus)) {
        return false;
    }
    errorCode.clear();
    if (!std::filesystem::exists(m_impl->cacheRoot, errorCode)) {
        errorCode.clear();
        std::filesystem::create_directories(m_impl->cacheRoot, errorCode);
        if (errorCode) {
            return false;
        }
    } else if (errorCode || !std::filesystem::is_directory(m_impl->cacheRoot, errorCode) || errorCode) {
        return false;
    }

    const std::filesystem::path artifactPath = m_impl->cacheRoot / ArtifactFileNameUVE(destinationPath);
    return WriteRecordAtomicallyUVE(artifactPath, record);
}

std::size_t DerivedArtifactCacheUVE::MarkStaleForSourceUVE(const std::filesystem::path& sourcePath) {
    if (sourcePath.empty()) {
        return 0U;
    }

    const std::filesystem::path normalizedSourcePath = NormalizePathUVE(sourcePath);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::error_code errorCode;
    const std::filesystem::file_status rootStatus = std::filesystem::symlink_status(m_impl->cacheRoot, errorCode);
    if (errorCode || std::filesystem::is_symlink(rootStatus) || !std::filesystem::is_directory(rootStatus)) {
        return 0U;
    }

    std::size_t markedCount = 0U;
    std::filesystem::directory_iterator iterator(m_impl->cacheRoot,
                                                 std::filesystem::directory_options::skip_permission_denied,
                                                 errorCode);
    if (errorCode) {
        return 0U;
    }
    const std::filesystem::directory_iterator end;
    while (iterator != end) {
        const std::filesystem::directory_entry entry = *iterator;
        const std::filesystem::file_status entryStatus = entry.symlink_status(errorCode);
        if (errorCode) {
            return markedCount;
        }
        if (std::filesystem::is_regular_file(entryStatus) && entry.path().extension() == ".uveimportcache") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                try {
                    nlohmann::json document;
                    file >> document;
                    std::optional<DerivedArtifactCacheRecordUVE> record = RecordFromJsonUVE(document);
                    if (record.has_value() && record->sourcePath == normalizedSourcePath && !record->stale) {
                        record->stale = true;
                        if (WriteRecordAtomicallyUVE(entry.path(), *record)) {
                            ++markedCount;
                        }
                    }
                } catch (const nlohmann::json::exception&) {
                    // A malformed artifact is already a cache miss; leave it untouched.
                }
            }
        }
        iterator.increment(errorCode);
        if (errorCode) {
            return markedCount;
        }
    }
    return markedCount;
}

std::filesystem::path DerivedArtifactCacheUVE::GetCacheRootUVE() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->cacheRoot;
}

} // namespace UVE::Asset
