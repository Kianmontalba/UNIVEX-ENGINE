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

/// Parses bounded OBJ declaration statistics from caller-owned text. It validates declaration
/// arity and face reference presence, reports polygon triangulation capacity, and returns copied
/// counts only; it does not resolve indices, load material libraries, allocate mesh data, or own
/// renderer/importer state.
[[nodiscard]] std::optional<ObjMetadataUVE> ParseObjMetadataUVE(std::string_view source);

} // namespace UVE::Asset
