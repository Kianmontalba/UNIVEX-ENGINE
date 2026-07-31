//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_manager_uve.h"

namespace UVE::Asset {

/// IHotReloadUVE detects on-disk changes to currently-loaded assets and triggers
/// AssetManagerUVE::ReloadUVE() for them, without any OS-native file-watching backend (none
/// exists in engine/platform/ yet, and cross-platform native watching is its own future
/// increment) — instead, it polls tracked assets' mtimes on a configurable interval.
/// Thread-safety: not thread-safe — intended to be driven by a single caller (EngineCoreUVE::
/// Update(), on the main/engine thread) via PollUVE(); TrackUVE()/UntrackUVE() are called by
/// AssetManagerUVE from inside its own thread-safe operations (LoadErased()/CollectGarbageUVE()),
/// so implementations must guard their internal tracked-set with a mutex even though PollUVE()
/// itself is main-thread-only.
class IHotReloadUVE {
public:
    virtual ~IHotReloadUVE() = default;

    /// Starts tracking `guid` for on-disk changes, recording its current mtime as the baseline.
    /// Called by AssetManagerUVE once a load completes successfully.
    virtual void TrackUVE(AssetGuidUVE guid) = 0;

    /// Stops tracking `guid`. Called by AssetManagerUVE when an asset is unloaded via
    /// CollectGarbageUVE().
    virtual void UntrackUVE(AssetGuidUVE guid) = 0;

    /// Accumulates `deltaTimeSeconds`; once the configured poll interval has elapsed, checks
    /// every tracked GUID's resolved path (via `assetDatabase`) for an mtime change since it was
    /// last observed and calls `assetManager.ReloadUVE()` for each one that changed. A tracked
    /// file that no longer exists logs a warning and is skipped for this poll (never crashes,
    /// never untracks it — the file may reappear).
    virtual void PollUVE(IAssetManagerUVE& assetManager, IAssetDatabaseUVE& assetDatabase,
                          double deltaTimeSeconds) = 0;
};

} // namespace UVE::Asset
