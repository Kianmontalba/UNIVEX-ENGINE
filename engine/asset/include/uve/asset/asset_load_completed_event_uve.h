// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset {

/// Queued (via IEventSystemUVE::QueueEvent(), safe to call from the IThreadPoolUVE worker that
/// ran the load) once AssetManagerUVE finishes loading `guid`, whether it succeeded or failed —
/// lets interested systems react without polling AssetHandleUVE::IsReadyUVE() every frame.
struct AssetLoadCompletedEventUVE final {
    AssetGuidUVE guid;
    bool success = false;
};

} // namespace UVE::Asset
