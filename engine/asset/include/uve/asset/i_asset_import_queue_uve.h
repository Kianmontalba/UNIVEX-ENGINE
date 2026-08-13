// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// Strong identifier for one queue-owned import job. Zero is never assigned and is invalid.
struct AssetImportJobIdUVE final {
    std::uint64_t value = 0U;

    [[nodiscard]] bool operator==(const AssetImportJobIdUVE&) const noexcept = default;
};

inline constexpr AssetImportJobIdUVE kInvalidAssetImportJobIdUVE{};

/// The only lifecycle states available to a v1 import job. Terminal states stay observable until
/// callers explicitly retry a failed job or the queue itself is destroyed.
enum class AssetImportJobStateUVE {
    Queued,
    Running,
    Succeeded,
    Failed,
};

enum class AssetImportDiagnosticSeverityUVE {
    Warning,
    Error,
};

/// Stable category for structured import diagnostics. Human-readable messages remain supplemental;
/// tests and future UI should prefer this code for deterministic branching.
enum class AssetImportDiagnosticCodeUVE {
    InvalidRequest,
    SourceFingerprintFailed,
    DestinationFingerprintFailed,
    ImporterFailed,
    CacheWriteFailed,
};

/// Read-only import request data retained by a queued job. `settings` is shared immutable ownership
/// so an enqueue caller may release its original handle without invalidating a later tick.
struct AssetImportRequestUVE final {
    std::filesystem::path sourcePath{};
    std::filesystem::path destinationPath{};
    std::shared_ptr<const AssetImportSettingsUVE> settings{};
    std::string settingsVersion{};
};

struct AssetImportDiagnosticUVE final {
    AssetImportDiagnosticSeverityUVE severity = AssetImportDiagnosticSeverityUVE::Error;
    AssetImportDiagnosticCodeUVE code = AssetImportDiagnosticCodeUVE::InvalidRequest;
    std::string message{};
    std::filesystem::path sourcePath{};
    std::filesystem::path destinationPath{};
    std::uint32_t attempt = 0U;
};

/// Copied observable state of one queue-owned import job. It is safe to retain and edit after
/// GetJobsUVE() returns; edits cannot modify queue internals or importer/cache ownership.
struct AssetImportJobUVE final {
    AssetImportJobIdUVE id{};
    AssetImportRequestUVE request{};
    AssetImportJobStateUVE state = AssetImportJobStateUVE::Queued;
    std::uint32_t attemptCount = 0U;
    std::optional<AssetContentFingerprintUVE> sourceFingerprint;
    std::optional<AssetContentFingerprintUVE> destinationFingerprint;
    std::optional<AssetGuidUVE> resultGuid;
    bool cacheHit = false;
    std::vector<AssetImportDiagnosticUVE> diagnostics;
};

/// Main-thread deterministic import scheduler. Enqueueing and retrying mutate queue state only;
/// exactly one oldest queued job executes synchronously per TickUVE() call. Retry appends a failed
/// job to the FIFO tail, preserving all jobs already waiting ahead of it. The interface deliberately
/// has no worker, cancellation, filesystem watching, or automatic reimport policy in v1.
class IAssetImportQueueUVE {
public:
    virtual ~IAssetImportQueueUVE() = default;

    /// Normalizes and queues a copied request without touching source, destination, cache, or
    /// AssetDatabaseUVE. Returns std::nullopt for empty paths or missing immutable settings.
    [[nodiscard]] virtual std::optional<AssetImportJobIdUVE>
    EnqueueUVE(AssetImportRequestUVE request) = 0;

    /// Synchronously processes at most one FIFO job. Returns true only when a queued job was
    /// transitioned through Running to Succeeded or Failed; returns false when no queued job exists.
    [[nodiscard]] virtual bool TickUVE() = 0;

    /// Requeues one failed job at the FIFO tail while preserving its stable id. Result data and
    /// diagnostics are cleared; attemptCount increments only when the retry later executes.
    [[nodiscard]] virtual bool RetryUVE(AssetImportJobIdUVE id) = 0;

    /// Returns all jobs in creation order as copied snapshots, never as references to internals.
    [[nodiscard]] virtual std::vector<AssetImportJobUVE> GetJobsUVE() const = 0;
};

} // namespace UVE::Asset
