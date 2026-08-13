// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/asset_import_queue_uve.h"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <mutex>
#include <utility>
#include <vector>

namespace UVE::Asset {
namespace {

[[nodiscard]] std::filesystem::path NormalizePathUVE(const std::filesystem::path& path) {
    std::error_code errorCode;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode);
    return errorCode ? path.lexically_normal() : absolutePath.lexically_normal();
}

[[nodiscard]] bool IsCacheRecordValidUVE(const DerivedArtifactCacheRecordUVE& record,
                                         const AssetImportRequestUVE& request,
                                         const AssetContentFingerprintUVE& sourceFingerprint,
                                         IAssetDatabaseUVE& assetDatabase,
                                         AssetContentFingerprintUVE& outDestinationFingerprint) {
    if (record.schemaVersion != kDerivedArtifactCacheSchemaVersionUVE ||
        record.sourcePath != request.sourcePath || record.destinationPath != request.destinationPath ||
        record.sourceFingerprint != sourceFingerprint || record.settingsVersion != request.settingsVersion ||
        record.assetGuid == kInvalidAssetGuidUVE) {
        return false;
    }

    const std::optional<AssetContentFingerprintUVE> destinationFingerprint =
        ComputeAssetContentFingerprintUVE(request.destinationPath);
    if (!destinationFingerprint.has_value() || *destinationFingerprint != record.destinationFingerprint) {
        return false;
    }

    if (NormalizePathUVE(assetDatabase.ResolveUVE(record.assetGuid)) != request.destinationPath) {
        return false;
    }

    outDestinationFingerprint = *destinationFingerprint;
    return true;
}

} // namespace

struct AssetImportQueueUVE::ImplUVE final {
    ImplUVE(IAssetImporterUVE& inImporter, IAssetDatabaseUVE& inAssetDatabase,
            IDerivedArtifactCacheUVE& inDerivedArtifactCache)
        : importer(inImporter), assetDatabase(inAssetDatabase), derivedArtifactCache(inDerivedArtifactCache) {}

    [[nodiscard]] AssetImportJobUVE* FindJobUVE(const AssetImportJobIdUVE id) {
        const auto it = std::find_if(jobs.begin(), jobs.end(), [id](const AssetImportJobUVE& job) {
            return job.id == id;
        });
        return it == jobs.end() ? nullptr : &(*it);
    }

    IAssetImporterUVE& importer;
    IAssetDatabaseUVE& assetDatabase;
    IDerivedArtifactCacheUVE& derivedArtifactCache;
    mutable std::mutex mutex;
    std::vector<AssetImportJobUVE> jobs;
    std::deque<AssetImportJobIdUVE> queuedIds;
    std::uint64_t nextJobId = 1U;
};

AssetImportQueueUVE::AssetImportQueueUVE(IAssetImporterUVE& importer, IAssetDatabaseUVE& assetDatabase,
                                         IDerivedArtifactCacheUVE& derivedArtifactCache)
    : m_impl(std::make_unique<ImplUVE>(importer, assetDatabase, derivedArtifactCache)) {}

AssetImportQueueUVE::~AssetImportQueueUVE() = default;

std::optional<AssetImportJobIdUVE> AssetImportQueueUVE::EnqueueUVE(AssetImportRequestUVE request) {
    if (request.sourcePath.empty() || request.destinationPath.empty() || !request.settings) {
        return std::nullopt;
    }

    request.sourcePath = NormalizePathUVE(request.sourcePath);
    request.destinationPath = NormalizePathUVE(request.destinationPath);
    if (request.sourcePath.empty() || request.destinationPath.empty()) {
        return std::nullopt;
    }
    if (request.settingsVersion.empty()) {
        request.settingsVersion = request.settings->GetCacheVersionUVE();
    }
    if (request.settingsVersion.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->nextJobId == kInvalidAssetImportJobIdUVE.value) {
        return std::nullopt;
    }

    const AssetImportJobIdUVE id{m_impl->nextJobId++};
    AssetImportJobUVE job;
    job.id = id;
    job.request = std::move(request);
    m_impl->jobs.push_back(std::move(job));
    m_impl->queuedIds.push_back(id);
    return id;
}

bool AssetImportQueueUVE::TickUVE() {
    AssetImportJobIdUVE id;
    AssetImportRequestUVE request;
    std::uint32_t attempt = 0U;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        if (m_impl->queuedIds.empty()) {
            return false;
        }
        id = m_impl->queuedIds.front();
        m_impl->queuedIds.pop_front();
        AssetImportJobUVE* const job = m_impl->FindJobUVE(id);
        if (job == nullptr || job->state != AssetImportJobStateUVE::Queued) {
            return false;
        }
        job->state = AssetImportJobStateUVE::Running;
        ++job->attemptCount;
        attempt = job->attemptCount;
        request = job->request;
    }

    const auto completeFailure = [this, id, attempt, &request](const AssetImportDiagnosticCodeUVE code,
                                                                 std::string message) {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        AssetImportJobUVE* const job = m_impl->FindJobUVE(id);
        if (job == nullptr) {
            return;
        }
        job->state = AssetImportJobStateUVE::Failed;
        job->cacheHit = false;
        job->resultGuid.reset();
        job->diagnostics.push_back(AssetImportDiagnosticUVE{AssetImportDiagnosticSeverityUVE::Error, code,
                                                             std::move(message), request.sourcePath,
                                                             request.destinationPath, attempt});
    };

    const std::optional<AssetContentFingerprintUVE> sourceFingerprint =
        ComputeAssetContentFingerprintUVE(request.sourcePath);
    if (!sourceFingerprint.has_value()) {
        completeFailure(AssetImportDiagnosticCodeUVE::SourceFingerprintFailed,
                        "Unable to fingerprint the queued import source.");
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        if (AssetImportJobUVE* const job = m_impl->FindJobUVE(id); job != nullptr) {
            job->sourceFingerprint = *sourceFingerprint;
        }
    }

    AssetContentFingerprintUVE cachedDestinationFingerprint;
    const std::optional<DerivedArtifactCacheRecordUVE> cachedRecord =
        m_impl->derivedArtifactCache.LoadImportRecordUVE(request.destinationPath);
    if (cachedRecord.has_value() &&
        IsCacheRecordValidUVE(*cachedRecord, request, *sourceFingerprint, m_impl->assetDatabase,
                              cachedDestinationFingerprint)) {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        if (AssetImportJobUVE* const job = m_impl->FindJobUVE(id); job != nullptr) {
            job->destinationFingerprint = cachedDestinationFingerprint;
            job->resultGuid = cachedRecord->assetGuid;
            job->cacheHit = true;
            job->state = AssetImportJobStateUVE::Succeeded;
        }
        return true;
    }

    const AssetGuidUVE assetGuid =
        m_impl->importer.ImportUVE(request.sourcePath, request.destinationPath, m_impl->assetDatabase, *request.settings);
    if (assetGuid == kInvalidAssetGuidUVE) {
        completeFailure(AssetImportDiagnosticCodeUVE::ImporterFailed,
                        "The registered importer failed or no importer accepted this source extension.");
        return true;
    }

    const std::optional<AssetContentFingerprintUVE> destinationFingerprint =
        ComputeAssetContentFingerprintUVE(request.destinationPath);
    if (!destinationFingerprint.has_value()) {
        completeFailure(AssetImportDiagnosticCodeUVE::DestinationFingerprintFailed,
                        "Importer returned a GUID but the destination file could not be fingerprinted.");
        return true;
    }

    const DerivedArtifactCacheRecordUVE cacheRecord{kDerivedArtifactCacheSchemaVersionUVE,
                                                     request.sourcePath,
                                                     request.destinationPath,
                                                     *sourceFingerprint,
                                                     *destinationFingerprint,
                                                     request.settingsVersion,
                                                     assetGuid};
    const bool cacheStored = m_impl->derivedArtifactCache.StoreImportRecordUVE(request.destinationPath, cacheRecord);

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    AssetImportJobUVE* const job = m_impl->FindJobUVE(id);
    if (job == nullptr) {
        return true;
    }
    job->destinationFingerprint = *destinationFingerprint;
    job->resultGuid = assetGuid;
    job->cacheHit = false;
    job->state = AssetImportJobStateUVE::Succeeded;
    if (!cacheStored) {
        job->diagnostics.push_back(AssetImportDiagnosticUVE{AssetImportDiagnosticSeverityUVE::Warning,
                                                             AssetImportDiagnosticCodeUVE::CacheWriteFailed,
                                                             "Import succeeded but derived cache metadata could not be written.",
                                                             request.sourcePath, request.destinationPath, attempt});
    }
    return true;
}

bool AssetImportQueueUVE::RetryUVE(const AssetImportJobIdUVE id) {
    if (id == kInvalidAssetImportJobIdUVE) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    AssetImportJobUVE* const job = m_impl->FindJobUVE(id);
    if (job == nullptr || job->state != AssetImportJobStateUVE::Failed) {
        return false;
    }
    job->state = AssetImportJobStateUVE::Queued;
    job->sourceFingerprint.reset();
    job->destinationFingerprint.reset();
    job->resultGuid.reset();
    job->cacheHit = false;
    job->diagnostics.clear();
    m_impl->queuedIds.push_back(id);
    return true;
}

std::vector<AssetImportJobUVE> AssetImportQueueUVE::GetJobsUVE() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->jobs;
}

} // namespace UVE::Asset
