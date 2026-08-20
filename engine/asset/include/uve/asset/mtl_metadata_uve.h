// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace UVE::Asset {
inline constexpr std::uint64_t kMaximumMtlTextureReferenceBytesUVE = 256ULL;

enum class MtlMaterialPropertyKindUVE : std::uint8_t { Invalid, Scalar, Vector3, TextureReference };

struct MtlMaterialPropertyUVE final {
    MtlMaterialPropertyKindUVE kind = MtlMaterialPropertyKindUVE::Invalid;
    float scalarValue = 0.0F;
    std::array<float, 3U> vectorValue{};
    std::string textureReference;
    [[nodiscard]] bool operator==(const MtlMaterialPropertyUVE&) const noexcept = default;
};

/// Parses one bounded MTL scalar, three-component vector, or validated texture-map property into
/// copied facts. It performs no shader/material conversion, filesystem I/O, or renderer ownership.
[[nodiscard]] bool ParseMtlMaterialPropertyUVE(
    std::string_view sourceLine, MtlMaterialPropertyUVE& outProperty);

struct MtlTextureMapUVE final {
    std::string textureReference;
    std::array<float, 3U> scale{1.0F, 1.0F, 1.0F};
    std::array<float, 3U> offset{};
    bool clamp = false;
    [[nodiscard]] bool operator==(const MtlTextureMapUVE&) const noexcept = default;
};

/// Parses one bounded MTL map directive with strict three-component `-s`/`-o` options and
/// `-clamp on|off`. It performs no texture loading, filesystem I/O, shader compilation, or material conversion.
[[nodiscard]] bool ParseMtlTextureMapUVE(std::string_view sourceLine, MtlTextureMapUVE& outMap);

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
