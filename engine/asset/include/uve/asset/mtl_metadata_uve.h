// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
namespace UVE::Asset {
inline constexpr std::uint64_t kMaximumMtlTextureReferenceBytesUVE = 256ULL;

struct MtlMetadataUVE final {
    std::uint32_t materialCount = 0U;
    std::uint32_t textureMapCount = 0U;
    std::uint32_t scalarPropertyCount = 0U;
    std::uint32_t vectorPropertyCount = 0U;
    std::uint32_t ignoredStatementCount = 0U;
};
/// Validates one bounded relative MTL texture-reference token without filesystem access.
/// Empty, absolute, NUL-containing, and traversal segments fail closed.
[[nodiscard]] bool ValidateMtlTextureReferenceUVE(std::string_view path) noexcept;

/// Parses bounded Wavefront MTL declaration statistics from caller-owned text. It validates
/// required tokens and returns counts only; it does not load texture paths, compile shaders,
/// resolve assets, or own material/renderer state.
[[nodiscard]] std::optional<MtlMetadataUVE> ParseMtlMetadataUVE(std::string_view source);
} // namespace UVE::Asset
