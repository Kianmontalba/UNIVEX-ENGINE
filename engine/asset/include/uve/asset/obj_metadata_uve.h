// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace UVE::Asset {

struct ObjMetadataUVE final {
    std::uint32_t positionCount = 0U;
    std::uint32_t texcoordCount = 0U;
    std::uint32_t normalCount = 0U;
    std::uint32_t faceCount = 0U;
    std::uint32_t triangleCount = 0U;
    std::uint32_t groupCount = 0U;
    std::uint32_t materialUseCount = 0U;
    std::uint32_t materialLibraryCount = 0U;
    std::uint32_t ignoredStatementCount = 0U;
};

/// Resolves one OBJ 1-based or negative-relative attribute index to a zero-based copied index.
/// Zero, empty domains, and out-of-range values fail closed without allocating mesh data.
[[nodiscard]] bool ResolveObjIndexUVE(std::int64_t rawIndex, std::uint32_t attributeCount,
                                      std::uint32_t& outZeroBasedIndex) noexcept;

struct ObjFaceVertexUVE final {
    std::uint32_t positionIndex = 0U;
    std::optional<std::uint32_t> texcoordIndex;
    std::optional<std::uint32_t> normalIndex;
    [[nodiscard]] bool operator==(const ObjFaceVertexUVE&) const noexcept = default;
};

/// Resolves one bounded OBJ face vertex token (`v`, `v/vt`, `v//vn`, or `v/vt/vn`) into copied
/// zero-based indices. It performs no face triangulation, mesh allocation, material loading, or I/O.
[[nodiscard]] bool ResolveObjFaceVertexUVE(
    std::string_view token, std::uint32_t positionCount, std::uint32_t texcoordCount,
    std::uint32_t normalCount, ObjFaceVertexUVE& outVertex) noexcept;

/// Parses bounded OBJ declaration statistics from caller-owned text. It validates declaration
/// arity and face reference presence, reports polygon triangulation capacity, and returns copied
/// counts only; it does not resolve indices, load material libraries, allocate mesh data, or own
/// renderer/importer state.
[[nodiscard]] std::optional<ObjMetadataUVE> ParseObjMetadataUVE(std::string_view source);

} // namespace UVE::Asset
