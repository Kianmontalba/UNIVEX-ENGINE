// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "uve/save/game_state_metadata_uve.h"

namespace UVE::Save {

inline constexpr std::size_t kMaximumSaveMigrationReasonBytesUVE = 256U;

enum class SaveMigrationStatusUVE : std::uint8_t {
    NotRequired = 0,
    Migrated = 1,
    UnsupportedSourceVersion = 2,
    UnsupportedTargetVersion = 3,
    InvalidPayload = 4,
};

struct SaveMigrationDiagnosticsUVE final {
    SaveMigrationStatusUVE status = SaveMigrationStatusUVE::NotRequired;
    std::uint32_t sourceSchemaVersion = kCurrentSavePayloadSchemaVersionUVE;
    std::uint32_t targetSchemaVersion = kCurrentSavePayloadSchemaVersionUVE;
    std::string reason;

    [[nodiscard]] bool SucceededUVE() const noexcept {
        return status == SaveMigrationStatusUVE::NotRequired || status == SaveMigrationStatusUVE::Migrated;
    }
};

/// Bounded schema-dispatch seam for the fixed `.uvesave` payload. Version 1 is currently the only
/// supported layout, so current payloads pass through unchanged and unsupported versions fail with
/// copied diagnostics. Future migrations must transform the payload here before scene deserialization;
/// compression, encryption, cloud sync, and gameplay-domain transforms remain outside this seam.
[[nodiscard]] SaveMigrationDiagnosticsUVE MigrateSavePayloadUVE(std::uint32_t sourceSchemaVersion,
                                                               std::uint32_t targetSchemaVersion,
                                                               std::vector<std::byte>& payload);

} // namespace UVE::Save
