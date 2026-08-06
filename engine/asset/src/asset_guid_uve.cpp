// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
