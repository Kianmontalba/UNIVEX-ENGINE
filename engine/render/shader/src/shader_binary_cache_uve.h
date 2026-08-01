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
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace UVE::Render::Shader::Detail {

/// One entry read back from the on-disk GL program-binary cache (Increment 21).
struct CacheEntryUVE {
    std::uint32_t glBinaryFormat = 0;
    std::vector<std::byte> payload;
};

/// Returns `<cachePath>/<platform>/<contentHash as 16 lowercase hex digits>.uveshadercache` — the
/// platform subdirectory (`"linux"`/`"windows"`/`"macos"`) is chosen at compile time, mirroring
/// engine/platform's own `#if defined(...)` idiom. Never touches the filesystem itself (no
/// existence check, no directory creation) — purely a deterministic path computation, so it's
/// trivially unit-testable.
[[nodiscard]] std::filesystem::path GetCacheFilePathUVE(const std::filesystem::path& cachePath,
                                                         std::uint64_t contentHash);

/// Reads and validates the small binary envelope this cache uses (an 8-byte magic + a
/// format-version field this module owns entirely, distinct from and not to be confused with the
/// engine's `.uve*` asset envelope — this is pure derived data, never a shippable asset). Returns
/// `std::nullopt` (never throws) for a missing file, a bad magic/version, or a truncated payload —
/// every one of these is an ordinary cache miss to the caller, not a hard error.
[[nodiscard]] std::optional<CacheEntryUVE> ReadCacheEntryUVE(const std::filesystem::path& filePath);

/// Writes `payload` (tagged with `glBinaryFormat`, the backend-reported binary format
/// IRenderDeviceUVE::GetPipelineBinaryUVE() produced it with) to `filePath`, creating parent
/// directories as needed. Returns false (logging at WARNING, not ERROR — a cache write failure is
/// never fatal, it just means the next startup recompiles from source) on any I/O failure.
[[nodiscard]] bool WriteCacheEntryUVE(const std::filesystem::path& filePath, std::uint32_t glBinaryFormat,
                                      std::span<const std::byte> payload);

} // namespace UVE::Render::Shader::Detail
