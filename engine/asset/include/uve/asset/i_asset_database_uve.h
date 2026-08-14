// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <filesystem>
#include <vector>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

/// An immutable copy of one registered project asset. `path` is lexically normalized for stable
/// editor presentation; it remains a registry record only and does not imply that the file exists
/// or is loaded. Value type; safe to retain after the AssetDatabase mutex is released.
struct AssetRecordUVE final {
    AssetGuidUVE guid{};
    std::filesystem::path path{};
};

/// IAssetDatabaseUVE is the minimal GUID-to-file-path registry the master spec requires scenes
/// to use instead of embedding file-path dependencies directly ("No file path dependencies in
/// scenes" — Part 7.4). This is a deliberately small placeholder: registering a path and
/// resolving a GUID back to one, persisted to disk. The full AssetManagerUVE (async loading,
/// reference counting, garbage collection, importers) is Part 7.4's own future increment.
/// Thread-safety: implementations must be safe to call from any thread concurrently, guarded by
/// an internal mutex (mirrors IConfigManagerUVE's contract).
class IAssetDatabaseUVE {
public:
    virtual ~IAssetDatabaseUVE() = default;

    /// Loads the GUID<->path registry from `path`. Missing file: logs a Warning, leaves the
    /// registry empty, returns false (not fatal — a first-run project has no registry yet).
    /// Malformed JSON: logs an Error, leaves the previous registry untouched, returns false.
    virtual bool LoadUVE(const std::filesystem::path& path) = 0;

    /// Saves the registry to the path most recently passed to LoadUVE()/SaveUVE(path). Returns
    /// false (logging the target path and failure reason) if no path is known yet, or if the
    /// file can't be opened for writing. Never fatal.
    virtual bool SaveUVE() = 0;

    /// Saves the registry to `path`, remembering it as the new default target for SaveUVE().
    virtual bool SaveUVE(const std::filesystem::path& path) = 0;

    /// Returns the existing GUID registered for `assetPath`, or generates and registers a fresh
    /// one if its normalized identity is not yet known. Identity is lexical only, so equivalent
    /// `.`/`..` spellings resolve to one GUID without resolving relative paths through the process
    /// current working directory. Relative and absolute forms remain distinct because this API has
    /// no explicit project-root dependency. The persisted registry schema remains a GUID-to-path
    /// map; legacy lexical-alias records stay resolvable, while future equivalent-path registration
    /// deterministically selects the smallest existing GUID for that lexical identity.
    [[nodiscard]] virtual AssetGuidUVE RegisterUVE(const std::filesystem::path& assetPath) = 0;

    /// Returns the lexically normalized stored path registered for `guid`, or an empty path if
    /// `guid` is unknown. It is a persisted path representation, not the normalized lookup key.
    [[nodiscard]] virtual std::filesystem::path ResolveUVE(AssetGuidUVE guid) const = 0;

    /// True iff `guid` is currently registered.
    [[nodiscard]] virtual bool HasGuidUVE(AssetGuidUVE guid) const = 0;

    /// Returns immutable copies of every registered asset in deterministic lexical path order,
    /// using GUID value as the tie-breaker. The list is a registry snapshot, not a filesystem scan:
    /// first-run projects may return an empty list, and callers must not infer loading or file
    /// existence from an entry. Thread-safe.
    [[nodiscard]] virtual std::vector<AssetRecordUVE> GetRegisteredAssetsUVE() const = 0;
};

} // namespace UVE::Asset
