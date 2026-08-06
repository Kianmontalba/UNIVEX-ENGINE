// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace UVE::Asset {

/// Pixel formats a loadable TextureAssetUVE can use. Deliberately a separate type from
/// `Render::TextureFormatUVE` (engine/render, Increment 10) even though the names overlap: this
/// engine's RHI already depends on engine/asset (to eventually turn a loaded MeshAssetUVE/
/// TextureAssetUVE into GPU resources), so if this enum referenced the RHI's, that would create a
/// dependency cycle. There is also no `Depth32Float` here — a depth format is only ever created
/// directly as a GPU render target, never loaded from a texture asset file. A future
/// render-side translation function (e.g. `Render::ToRenderTextureFormatUVE(Asset::TextureFormatUVE)`)
/// bridges the two when a real consumer needs it — not built here, since nothing in this
/// increment touches engine/render at all.
enum class TextureFormatUVE : std::uint8_t { RGBA8Unorm, RGBA16Float };

/// The CPU-side, engine-native representation of a `.uvetex` asset (Part 2's file-format table):
/// raw, uncompressed pixel data. Deliberately minimal — no mipmaps or block compression
/// (ASTC/ETC2/BC7) yet; Part 7.9's `TextureCompressorUVE` is blocked (needs a compressor library
/// not available in this environment) and is future-increment work.
struct TextureAssetUVE {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    TextureFormatUVE format = TextureFormatUVE::RGBA8Unorm;
    std::vector<std::byte> pixels;
};

/// The exact byte size of one pixel in `format` (4 for RGBA8Unorm, 8 for RGBA16Float) — the
/// expected `pixels.size()` for a `width x height` TextureAssetUVE is `width * height *
/// BytesPerPixelUVE(format)`.
[[nodiscard]] std::uint32_t BytesPerPixelUVE(TextureFormatUVE format) noexcept;

/// Loads `path` as a `.uve*` envelope with `AssetKindUVE::Texture`, filling `outTexture`. Returns
/// false (logging the reason) if the file is missing/malformed, isn't actually a Texture asset,
/// or `pixels.size()` doesn't equal `width * height * BytesPerPixelUVE(format)`.
[[nodiscard]] bool LoadTextureAssetUVE(const std::filesystem::path& path, TextureAssetUVE& outTexture);

/// Writes `texture` to `path` as a `.uve*` envelope with `AssetKindUVE::Texture`. Returns false
/// (logging the reason) if the file can't be written.
[[nodiscard]] bool SaveTextureAssetUVE(const TextureAssetUVE& texture, const std::filesystem::path& path);

} // namespace UVE::Asset
