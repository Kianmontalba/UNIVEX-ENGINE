//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <filesystem>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

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
    /// one if `assetPath` isn't yet known — idempotent by path, so saving the same asset path
    /// twice never produces two different GUIDs for it.
    [[nodiscard]] virtual AssetGuidUVE RegisterUVE(const std::filesystem::path& assetPath) = 0;

    /// Returns the file path registered for `guid`, or an empty path if `guid` is unknown.
    [[nodiscard]] virtual std::filesystem::path ResolveUVE(AssetGuidUVE guid) const = 0;

    /// True iff `guid` is currently registered.
    [[nodiscard]] virtual bool HasGuidUVE(AssetGuidUVE guid) const = 0;
};

} // namespace UVE::Asset
