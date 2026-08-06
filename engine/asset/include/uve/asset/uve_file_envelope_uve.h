// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

namespace UVE::Asset {

/// The `assetType` field of the universal .uve* binary envelope below — one global numbering
/// shared by every `.uve*` file on disk, so no two asset kinds can ever collide on the same
/// value. `Scene`/`Prefab` are consumed by `Scene::SceneSerializerUVE`; `Blob` is the reference
/// "raw bytes" asset kind `AssetManagerUVE`'s tests/demonstration loader uses; `Bundle` is
/// `AssetBundleUVE`'s packed-multi-asset format (a variable-length name-indexed entry table);
/// `Mesh`/`Texture`/`Shader`/`Material` are `MeshAssetUVE`/`TextureAssetUVE`/`ShaderAssetUVE`/
/// `MaterialAssetUVE`'s native `.uve*` formats (Part 7.2's rendering-facing asset types); `Save`
/// is `Save::SaveGameSystemUVE`'s `.uvesave` format (Part 17) — a *fixed* two-section payload
/// (a length-prefixed metadata JSON section, then a length-prefixed embedded world-state JSON
/// section), deliberately distinct in shape from `Bundle`'s variable-length entry table, since a
/// save file always has exactly these two sections, never a variable named set.
enum class AssetKindUVE : std::uint32_t {
    Scene = 1,
    Prefab = 2,
    Blob = 3,
    Bundle = 4,
    Mesh = 5,
    Texture = 6,
    Shader = 7,
    Material = 8,
    Save = 9,
};

/// The fixed-size portion of a `.uve*` file's header, returned by ReadUveFileUVE() alongside the
/// payload bytes so callers can inspect `assetType`/`version` without re-parsing the file.
struct UveFileHeaderUVE {
    std::uint32_t version = 0;
    AssetKindUVE assetType = AssetKindUVE::Blob;
    std::uint32_t compressionMethod = 0;
};

/// Writes `payload` to `path` as a universal `.uve*` binary envelope: magic `"UVE\0"`,
/// `version uint32` (the current payload schema version), `assetType uint32`,
/// `compressionMethod uint32` (always `0 = None` — the only value implemented so far),
/// `payloadLength uint64`, then `payload` verbatim. Every field is written individually as a
/// fixed-width value (never a whole-struct write), sidestepping padding/alignment ambiguity.
/// The payload's own interpretation (UTF-8 JSON text for Scene/Prefab, a binary directory+blob
/// table for Bundle, raw file bytes for Blob) is entirely up to the caller. Returns false
/// (logging the path + reason) if the file can't be opened for writing.
[[nodiscard]] bool WriteUveFileUVE(const std::filesystem::path& path, AssetKindUVE assetType,
                                    const std::vector<std::byte>& payload);

/// Reads and validates a `.uve*` file written by WriteUveFileUVE(), returning its header and
/// payload bytes. Returns `std::nullopt` (logging a detailed `UVE_ERROR`: path + reason) on any
/// failure — missing file, bad magic, truncated header/payload, or an unsupported compression
/// method — never throws and never crashes. Deliberately does not validate `assetType` against
/// any specific expected value; callers that care (e.g. `SceneSerializerUVE` rejecting a Bundle
/// file, or vice versa) check `UveFileHeaderUVE::assetType` themselves.
[[nodiscard]] std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
ReadUveFileUVE(const std::filesystem::path& path);

} // namespace UVE::Asset
