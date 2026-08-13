// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

/// Bump this value whenever the persisted import-cache metadata schema changes incompatibly.
inline constexpr std::uint32_t kDerivedArtifactCacheSchemaVersionUVE = 1U;

/// Persistent metadata proving that one importer result matches a complete source byte stream,
/// complete destination byte stream, importer settings version, and registered asset identity.
/// The cache remains metadata-only: it never becomes a second AssetDatabase GUID registry.
struct DerivedArtifactCacheRecordUVE final {
    std::uint32_t schemaVersion = kDerivedArtifactCacheSchemaVersionUVE;
    std::filesystem::path sourcePath{};
    std::filesystem::path destinationPath{};
    AssetContentFingerprintUVE sourceFingerprint{};
    AssetContentFingerprintUVE destinationFingerprint{};
    std::string settingsVersion{};
    AssetGuidUVE assetGuid{};
};

/// I/O abstraction for project-local generated import metadata. Implementations accept only
/// `destinationPath` as an identity key and must retain artifacts exclusively under their configured
/// cache root; a caller can never make them write beside sources, destinations, or arbitrary paths.
class IDerivedArtifactCacheUVE {
public:
    virtual ~IDerivedArtifactCacheUVE() = default;

    /// Returns a copied, fully parsed record for the destination identity, or std::nullopt for a
    /// missing, unreadable, malformed, unsupported-schema, or otherwise invalid artifact. A cache
    /// miss is ordinary control flow and must not mutate files or AssetDatabaseUVE.
    [[nodiscard]] virtual std::optional<DerivedArtifactCacheRecordUVE>
    LoadImportRecordUVE(const std::filesystem::path& destinationPath) const = 0;

    /// Persists one metadata record at the deterministic artifact location associated with
    /// `destinationPath`. Implementations may create only their own configured cache directories.
    /// Returns false without modifying the previous valid artifact when the record is invalid or
    /// cannot be written atomically.
    [[nodiscard]] virtual bool StoreImportRecordUVE(const std::filesystem::path& destinationPath,
                                                     const DerivedArtifactCacheRecordUVE& record) = 0;

    /// Exposes the normalized configured cache root for diagnostics and tests. Returning a value
    /// does not imply that the root currently exists; it is created lazily only on successful store.
    [[nodiscard]] virtual std::filesystem::path GetCacheRootUVE() const = 0;
};

} // namespace UVE::Asset
