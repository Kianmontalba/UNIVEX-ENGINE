// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
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
/// save file always has exactly these two sections, never a variable named set. `DataTable` is the
/// envelope-backed typed data-table asset format. `Audio` is the bounded
/// interleaved PCM16-derived normalized sample envelope used by `.uveaudio` importer output. `Animation`
/// is the bounded JSON transform-sample/event envelope used by `.uveanim` importer output. The
/// `assetType` value is never reused.
inline constexpr std::size_t kMaximumUveFilePayloadBytesUVE = 512U * 1024U * 1024U;

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
    DataTable = 10,
    Audio = 11,
    Animation = 12,
};

/// The fixed-size portion of a `.uve*` file's header, returned by ReadUveFileUVE() alongside the
/// payload bytes so callers can inspect `assetType`/`version` without re-parsing the file.
struct UveFileHeaderUVE {
    std::uint32_t version = 0;
    AssetKindUVE assetType = AssetKindUVE::Blob;
    std::uint32_t compressionMethod = 0;
};

/// Returns whether a payload can be represented by the bounded universal `.uve*` envelope contract.
[[nodiscard]] bool IsUveFilePayloadSizeValidUVE(std::size_t payloadBytes) noexcept;

/// Encodes `payload` as a universal `.uve*` binary envelope in memory: magic "UVE\0",
/// `version uint32` (the current payload schema version), `assetType uint32`,
/// `compressionMethod uint32` (always `0 = None`), `payloadLength uint64`, then `payload`
/// verbatim. It is the authoritative byte layout used by both filesystem persistence and
/// in-memory scene-history snapshots.
[[nodiscard]] std::vector<std::byte> EncodeUveFileEnvelopeUVE(AssetKindUVE assetType,
                                                               const std::vector<std::byte>& payload);

/// Validates and decodes a universal `.uve*` binary envelope held in memory. `sourceDescription`
/// appears in diagnostic logging and should identify the caller's logical source. Returns
/// std::nullopt for bad magic, a truncated header/payload, unsupported version/compression, an invalid
/// asset-kind value, or an oversized payload. Trailing bytes remain permitted for compatibility with
/// ReadUveFileUVE().
[[nodiscard]] std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
DecodeUveFileEnvelopeUVE(const std::vector<std::byte>& envelope,
                         std::string_view sourceDescription = "<memory>");

/// Writes `payload` to `path` using EncodeUveFileEnvelopeUVE(). The payload's own interpretation
/// (UTF-8 JSON text for Scene/Prefab, a binary directory+blob table for Bundle, raw file bytes for
/// Blob) is entirely up to the caller. Returns false (logging the path + reason) if the file can't
/// be opened or written.
[[nodiscard]] bool WriteUveFileUVE(const std::filesystem::path& path, AssetKindUVE assetType,
                                    const std::vector<std::byte>& payload);

/// Reads and validates a `.uve*` file written by WriteUveFileUVE(), delegating binary parsing
/// to DecodeUveFileEnvelopeUVE(). Returns `std::nullopt` (logging a detailed `UVE_ERROR`: path +
/// reason) on any failure — missing file, bad magic, truncated header/payload, invalid asset-kind
/// value, or an unsupported compression method — never throws and never crashes. It does not validate
/// `assetType` against any specific expected value; callers that care (e.g. `SceneSerializerUVE` rejecting
/// a Bundle file, or vice versa) check `UveFileHeaderUVE::assetType` themselves.
[[nodiscard]] std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
ReadUveFileUVE(const std::filesystem::path& path);

} // namespace UVE::Asset
