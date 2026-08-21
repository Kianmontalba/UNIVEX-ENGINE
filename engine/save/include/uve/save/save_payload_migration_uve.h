// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "uve/save/game_state_metadata_uve.h"

namespace UVE::Save {

inline constexpr std::size_t kMaximumSaveMigrationReasonBytesUVE = 256U;
inline constexpr std::size_t kMaximumSaveMigrationPayloadBytesUVE = 64U * 1024U * 1024U;
inline constexpr std::size_t kMaximumSaveMigrationTransformsUVE = 32U;

using SavePayloadMigrationTransformUVE =
    std::function<bool(std::vector<std::byte>& payload, std::string& failureReason)>;

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
    std::size_t appliedStepCount = 0U;
    std::string reason;

    [[nodiscard]] bool SucceededUVE() const noexcept {
        return status == SaveMigrationStatusUVE::NotRequired || status == SaveMigrationStatusUVE::Migrated;
    }
};

enum class SaveMigrationRegistrationCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidVersionRange,
    MissingTransform,
    DuplicateTransform,
    CapacityExceeded,
};

struct SaveMigrationRegistrationResultUVE final {
    SaveMigrationRegistrationCodeUVE code = SaveMigrationRegistrationCodeUVE::Accepted;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == SaveMigrationRegistrationCodeUVE::Accepted;
    }
};

class SavePayloadMigrationRegistryUVE final {
public:
    SavePayloadMigrationRegistryUVE() = default;
    SavePayloadMigrationRegistryUVE(const SavePayloadMigrationRegistryUVE&) = delete;
    SavePayloadMigrationRegistryUVE& operator=(const SavePayloadMigrationRegistryUVE&) = delete;

    [[nodiscard]] SaveMigrationRegistrationResultUVE RegisterUVE(
        std::uint32_t sourceSchemaVersion, std::uint32_t targetSchemaVersion,
        SavePayloadMigrationTransformUVE transform);

    [[nodiscard]] SaveMigrationDiagnosticsUVE MigrateUVE(
        std::uint32_t sourceSchemaVersion, std::uint32_t targetSchemaVersion,
        std::vector<std::byte>& payload) const;

    [[nodiscard]] std::size_t GetTransformCountUVE() const noexcept;

private:
    struct EntryUVE final {
        std::uint32_t sourceSchemaVersion = 0U;
        std::uint32_t targetSchemaVersion = 0U;
        SavePayloadMigrationTransformUVE transform;
    };

    std::vector<EntryUVE> m_entries;
};

/// Bounded schema-dispatch seam for the fixed `.uvesave` payload. Current-version payloads pass
/// through unchanged; registered transforms can be composed through a deterministic shortest path
/// of at most the bounded registry capacity, with failure-atomic staging before scene deserialization.
/// Compression, encryption, cloud sync, and gameplay-domain transforms remain outside this seam.
[[nodiscard]] SaveMigrationDiagnosticsUVE MigrateSavePayloadUVE(std::uint32_t sourceSchemaVersion,
                                                               std::uint32_t targetSchemaVersion,
                                                               std::vector<std::byte>& payload);

} // namespace UVE::Save
