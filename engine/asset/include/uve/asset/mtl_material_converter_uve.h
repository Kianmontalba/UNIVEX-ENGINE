// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <string_view>

#include "uve/asset/material_asset_uve.h"

namespace UVE::Asset {

inline constexpr std::size_t kMaximumMtlMaterialSourceBytesUVE = 64U * 1024U;

/// Converts one bounded MTL material declaration into a failure-atomic MaterialAssetUVE. The v1
/// path accepts one `newmtl` block, validated Kd/Ka/Ks/Ke/Tf vector properties, validated Ns/Ni/d/Tr/
/// illum scalar properties, and strict map directives as syntax-only evidence. It maps Kd to albedo,
/// Ks to metallic, d/Tr to opacity policy, and Ke to emissive color. It does not fabricate texture or
/// shader GUIDs, load external files, preserve unsupported map options, or own importer/GPU state.
[[nodiscard]] bool ConvertMtlMaterialUVE(std::string_view source, MaterialAssetUVE& outMaterial);

} // namespace UVE::Asset
