// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/save/save_payload_migration_uve.h"

#include <algorithm>

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
    if (payload.empty()) {
        diagnostics.status = SaveMigrationStatusUVE::InvalidPayload;
        diagnostics.reason = BoundedReasonUVE("save payload is empty");
        return diagnostics;
    }

    diagnostics.status = SaveMigrationStatusUVE::NotRequired;
    return diagnostics;
}

} // namespace UVE::Save
