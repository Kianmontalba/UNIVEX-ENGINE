//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <vector>

namespace UVE::Asset {

/// BlobAssetUVE is the reference "loadable" asset type for AssetManagerUVE: a `.uve*` file's raw
/// payload bytes, with no format-specific parsing applied. It exists so this increment can prove
/// AssetManagerUVE's async-load/ref-count/GC pipeline end-to-end with a real, generic asset kind
/// instead of inventing a fake mesh/texture format — every future binary asset type (mesh,
/// texture, audio) will start from exactly these bytes before its own format-specific parse step.
using BlobAssetUVE = std::vector<std::byte>;

} // namespace UVE::Asset
