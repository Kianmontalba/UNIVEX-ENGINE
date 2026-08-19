// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace UVE::Save {
inline constexpr std::size_t kMaximumCompressedSavePayloadBytesUVE = 64U * 1024U * 1024U;
/// Compresses a fixed save payload when bounded RLE is smaller; otherwise returns an unchanged copy.
[[nodiscard]] std::vector<std::byte> CompressSavePayloadUVE(const std::vector<std::byte>& payload);
/// Expands a compressed payload or copies a legacy raw payload; output is unchanged on failure.
[[nodiscard]] bool DecompressSavePayloadUVE(const std::vector<std::byte>& payload,
                                             std::vector<std::byte>& outPayload);
} // namespace UVE::Save
