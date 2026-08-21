// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <filesystem>

#include "uve/asset/asset_guid_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Asset {

/// The CPU-side, engine-native representation of a `.uvemat` asset (Part 2's file-format table):
/// a PBR material — Albedo, Normal, Metallic, Roughness, AO, Emissive (Part 7.2's
/// `MaterialSystemUVE` workflow). Textures and shaders are referenced by `AssetGuidUVE`, never a
/// file path — matching the master spec's "No file path dependencies in scenes" rule
/// (`IAssetDatabaseUVE`'s own contract). `kInvalidAssetGuidUVE` on any texture reference means
/// "unset, use the corresponding flat value/no effect instead" (there's no dedicated
/// "has texture" flag — an invalid GUID already means exactly that). Referential integrity of
/// these GUIDs (whether the referenced asset is actually loaded) is deliberately not this type's
/// concern — a referenced texture/shader may simply not be loaded yet, which isn't an error here;
/// a future `MeshRendererUVE` (Increment 13) is what decides how to handle that at render time.
struct MaterialAssetUVE {
    Math::Vector3UVE albedoColor{1.0F, 1.0F, 1.0F};
    AssetGuidUVE albedoTexture;
    AssetGuidUVE normalTexture;
    float metallic = 0.0F;
    float roughness = 0.5F;
    AssetGuidUVE aoTexture;
    Math::Vector3UVE emissiveColor{0.0F, 0.0F, 0.0F};
    AssetGuidUVE vertexShader;
    AssetGuidUVE fragmentShader;
    /// Drives `RenderQueueUVE`'s opaque/transparent bucketing (Increment 13) — not consumed by
    /// anything yet.
    bool isTransparent = false;
};

/// Validates the CPU material descriptor before persistence or renderer handoff. Albedo channels and
/// metallic/roughness are finite in [0,1]; emissive channels are finite and nonnegative; GUID
/// references may be invalid to represent an intentionally unset texture or shader.
[[nodiscard]] bool IsMaterialAssetValidUVE(const MaterialAssetUVE& material) noexcept;

/// Loads `path` as a `.uve*` envelope with `AssetKindUVE::Material`, filling `outMaterial`.
/// Returns false (logging the reason) if the file is missing/malformed, isn't actually a
/// Material asset, or is missing an expected field.
[[nodiscard]] bool LoadMaterialAssetUVE(const std::filesystem::path& path, MaterialAssetUVE& outMaterial);

/// Writes `material` to `path` as a `.uve*` envelope with `AssetKindUVE::Material`. Returns false
/// (logging the reason) for invalid finite/range fields or a file publication failure; invalid
/// descriptors are rejected before opening the destination.
[[nodiscard]] bool SaveMaterialAssetUVE(const MaterialAssetUVE& material, const std::filesystem::path& path);

} // namespace UVE::Asset
