//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_guid_uve.h"

#include <limits>
#include <random>

namespace UVE::Asset {

namespace {
[[nodiscard]] std::mt19937_64& GetRandomEngineUVE() {
    thread_local std::mt19937_64 engine{std::random_device{}()};
    return engine;
}
} // namespace

AssetGuidUVE GenerateAssetGuidUVE() {
    std::uniform_int_distribution<std::uint64_t> distribution(1, std::numeric_limits<std::uint64_t>::max());
    return AssetGuidUVE{distribution(GetRandomEngineUVE())};
}

} // namespace UVE::Asset
