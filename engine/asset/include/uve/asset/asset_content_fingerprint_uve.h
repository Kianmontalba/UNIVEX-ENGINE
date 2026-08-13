// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace UVE::Asset {

/// Deterministic non-cryptographic identity for one complete file byte stream. It is suitable for
/// derived-artifact cache validity only; it is not a security digest or an integrity guarantee
/// against a deliberate collision. Value type; safe to copy between queue/cache snapshots.
struct AssetContentFingerprintUVE final {
    std::uint64_t hash = 0U;
    std::uint64_t byteCount = 0U;

    [[nodiscard]] bool operator==(const AssetContentFingerprintUVE&) const noexcept = default;
};

/// Computes a deterministic FNV-1a fingerprint over every byte in `path`. Returns std::nullopt
/// when the path is empty, unreadable, non-regular, or changes in a way that prevents a complete
/// byte-stream read. This helper does not log because callers own the relevant import/cache
/// diagnostic context. It never follows a caller-supplied path through any additional directory
/// traversal beyond opening that exact filesystem path.
[[nodiscard]] std::optional<AssetContentFingerprintUVE>
ComputeAssetContentFingerprintUVE(const std::filesystem::path& path);

} // namespace UVE::Asset
