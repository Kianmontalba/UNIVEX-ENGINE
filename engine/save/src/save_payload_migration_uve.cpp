// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/save/save_payload_migration_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Save {

namespace {

[[nodiscard]] std::string BoundedReasonUVE(const char* const reason) {
    std::string bounded(reason);
    if (bounded.size() > kMaximumSaveMigrationReasonBytesUVE) {
        bounded.resize(kMaximumSaveMigrationReasonBytesUVE);
    }
    return bounded;
}

} // namespace

SaveMigrationRegistrationResultUVE SavePayloadMigrationRegistryUVE::RegisterUVE(
    const std::uint32_t sourceSchemaVersion, const std::uint32_t targetSchemaVersion,
    SavePayloadMigrationTransformUVE transform) {
    if (targetSchemaVersion == 0U || sourceSchemaVersion == targetSchemaVersion) {
        return {SaveMigrationRegistrationCodeUVE::InvalidVersionRange,
                "Save migration source and target versions must differ and target must be non-zero."};
    }
    if (!transform) {
        return {SaveMigrationRegistrationCodeUVE::MissingTransform,
                "Save migration registration requires a callable transform."};
    }
    const auto duplicate = std::find_if(m_entries.cbegin(), m_entries.cend(),
                                        [sourceSchemaVersion, targetSchemaVersion](const EntryUVE& entry) {
                                            return entry.sourceSchemaVersion == sourceSchemaVersion &&
                                                   entry.targetSchemaVersion == targetSchemaVersion;
                                        });
    if (duplicate != m_entries.cend()) {
        return {SaveMigrationRegistrationCodeUVE::DuplicateTransform,
                "A save migration transform for this source/target pair is already registered."};
    }
    if (m_entries.size() >= kMaximumSaveMigrationTransformsUVE) {
        return {SaveMigrationRegistrationCodeUVE::CapacityExceeded,
                "The bounded save migration registry has reached its maximum capacity."};
    }
    m_entries.push_back(EntryUVE{sourceSchemaVersion, targetSchemaVersion, std::move(transform)});
    return {SaveMigrationRegistrationCodeUVE::Accepted, "Save migration transform registered."};
}

SaveMigrationDiagnosticsUVE SavePayloadMigrationRegistryUVE::MigrateUVE(
    const std::uint32_t sourceSchemaVersion, const std::uint32_t targetSchemaVersion,
    std::vector<std::byte>& payload) const {
    SaveMigrationDiagnosticsUVE diagnostics;
    diagnostics.sourceSchemaVersion = sourceSchemaVersion;
    diagnostics.targetSchemaVersion = targetSchemaVersion;

    if (sourceSchemaVersion == targetSchemaVersion &&
        sourceSchemaVersion == kCurrentSavePayloadSchemaVersionUVE) {
        return MigrateSavePayloadUVE(sourceSchemaVersion, targetSchemaVersion, payload);
    }
    if (sourceSchemaVersion == targetSchemaVersion) {
        diagnostics.status = SaveMigrationStatusUVE::UnsupportedSourceVersion;
        diagnostics.reason = BoundedReasonUVE("source save payload schema version is unsupported");
        return diagnostics;
    }

    const std::size_t invalidIndex = std::numeric_limits<std::size_t>::max();
    std::vector<std::uint32_t> discoveredVersions{sourceSchemaVersion};
    std::vector<std::size_t> predecessorVersion{invalidIndex};
    std::vector<std::size_t> predecessorEntry{invalidIndex};
    std::size_t queueIndex = 0U;
    std::size_t targetVersionIndex = invalidIndex;
    while (queueIndex < discoveredVersions.size() && targetVersionIndex == invalidIndex) {
        const std::uint32_t currentVersion = discoveredVersions[queueIndex];
        for (std::size_t entryIndex = 0U; entryIndex < m_entries.size(); ++entryIndex) {
            const EntryUVE& entry = m_entries[entryIndex];
            if (entry.sourceSchemaVersion != currentVersion ||
                std::find(discoveredVersions.cbegin(), discoveredVersions.cend(), entry.targetSchemaVersion) !=
                    discoveredVersions.cend()) {
                continue;
            }
            discoveredVersions.push_back(entry.targetSchemaVersion);
            predecessorVersion.push_back(queueIndex);
            predecessorEntry.push_back(entryIndex);
            if (entry.targetSchemaVersion == targetSchemaVersion) {
                targetVersionIndex = discoveredVersions.size() - 1U;
                break;
            }
        }
        ++queueIndex;
    }

    if (targetVersionIndex == invalidIndex) {
        diagnostics.status = targetSchemaVersion == kCurrentSavePayloadSchemaVersionUVE
                                 ? SaveMigrationStatusUVE::UnsupportedSourceVersion
                                 : SaveMigrationStatusUVE::UnsupportedTargetVersion;
        diagnostics.reason = BoundedReasonUVE(targetSchemaVersion == kCurrentSavePayloadSchemaVersionUVE
                                                  ? "no migration path reaches the current save schema version"
                                                  : "no migration path reaches the requested save schema version");
        return diagnostics;
    }
    if (payload.empty() || payload.size() > kMaximumSaveMigrationPayloadBytesUVE) {
        diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
        diagnostics.reason = BoundedReasonUVE(payload.empty() ? "save payload is empty"
                                                               : "save payload exceeds migration cap");
        return diagnostics;
    }

    std::vector<std::size_t> path;
    for (std::size_t versionIndex = targetVersionIndex;
         predecessorEntry[versionIndex] != invalidIndex;
         versionIndex = predecessorVersion[versionIndex]) {
        path.push_back(predecessorEntry[versionIndex]);
    }
    std::reverse(path.begin(), path.end());

    std::vector<std::byte> migratedPayload = payload;
    std::string failureReason;
    for (const std::size_t entryIndex : path) {
        failureReason.clear();
        try {
            if (!m_entries[entryIndex].transform(migratedPayload, failureReason)) {
                diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
                diagnostics.reason = failureReason.empty()
                                         ? BoundedReasonUVE("save migration transform rejected payload")
                                         : std::move(failureReason);
                if (diagnostics.reason.size() > kMaximumSaveMigrationReasonBytesUVE) {
                    diagnostics.reason.resize(kMaximumSaveMigrationReasonBytesUVE);
                }
                return diagnostics;
            }
        } catch (...) {
            diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
            diagnostics.reason = BoundedReasonUVE("save migration transform raised an exception");
            return diagnostics;
        }
        if (migratedPayload.empty() || migratedPayload.size() > kMaximumSaveMigrationPayloadBytesUVE) {
            diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
            diagnostics.reason = BoundedReasonUVE(migratedPayload.empty()
                                                       ? "save migration transform produced an empty payload"
                                                       : "save migration transform exceeded migration cap");
            return diagnostics;
        }
        ++diagnostics.appliedStepCount;
    }
    payload.swap(migratedPayload);
    diagnostics.status = SaveMigrationStatusUVE::Migrated;
    diagnostics.reason = BoundedReasonUVE("save payload migrated successfully");
    return diagnostics;
}

std::size_t SavePayloadMigrationRegistryUVE::GetTransformCountUVE() const noexcept {
    return m_entries.size();
}

SaveMigrationDiagnosticsUVE MigrateSavePayloadUVE(const std::uint32_t sourceSchemaVersion,
                                                   const std::uint32_t targetSchemaVersion,
                                                   std::vector<std::byte>& payload) {
    SaveMigrationDiagnosticsUVE diagnostics;
    diagnostics.sourceSchemaVersion = sourceSchemaVersion;
    diagnostics.targetSchemaVersion = targetSchemaVersion;

    if (targetSchemaVersion != kCurrentSavePayloadSchemaVersionUVE) {
        diagnostics.status = SaveMigrationStatusUVE::UnsupportedTargetVersion;
        diagnostics.reason = BoundedReasonUVE("target save payload schema version is unsupported");
        return diagnostics;
    }
    if (sourceSchemaVersion != kCurrentSavePayloadSchemaVersionUVE) {
        diagnostics.status = SaveMigrationStatusUVE::UnsupportedSourceVersion;
        diagnostics.reason = BoundedReasonUVE("source save payload schema version is unsupported");
        return diagnostics;
    }
    if (payload.empty() || payload.size() > kMaximumSaveMigrationPayloadBytesUVE) {
        diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
        diagnostics.reason = BoundedReasonUVE(payload.empty() ? "save payload is empty"
                                                               : "save payload exceeds migration cap");
        return diagnostics;
    }

    diagnostics.status = SaveMigrationStatusUVE::NotRequired;
    return diagnostics;
}

} // namespace UVE::Save
