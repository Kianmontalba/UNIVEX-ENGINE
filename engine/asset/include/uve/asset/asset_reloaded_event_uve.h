// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

/// Queued once HotReloadUVE detects an on-disk change to a tracked asset and dispatches a
/// reload for it. Signals only that a reload was triggered, not that it necessarily succeeded —
/// subscribe to AssetLoadCompletedEventUVE for the pass/fail outcome once the reload finishes.
struct AssetReloadedEventUVE final {
    AssetGuidUVE guid;
};

} // namespace UVE::Asset
