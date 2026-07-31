//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

namespace UVE::Asset {

/// The lifecycle state of one AssetManagerUVE-tracked asset record.
enum class AssetLoadStateUVE {
    /// No load has ever been requested for this GUID (not currently used as a stored state —
    /// a record is only created once a load is requested — but named for completeness/clarity
    /// at call sites that query state for a GUID with no record at all).
    NotLoaded,
    /// A load job has been submitted to the thread pool and has not yet completed.
    Loading,
    /// The load completed successfully; the record's data pointer is valid.
    Loaded,
    /// The load ran and failed (missing file, envelope validation failure, or the registered
    /// loader function itself returned false); the record's data pointer is null.
    Failed,
};

} // namespace UVE::Asset
