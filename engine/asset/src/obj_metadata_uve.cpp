// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/obj_metadata_uve.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string_view>

namespace UVE::Asset {
namespace {

constexpr std::uint32_t kMaximumCountUVE = 1'000'000U;

[[nodiscard]] bool IncrementUVE(std::uint32_t& value, const std::uint32_t amount = 1U) noexcept {
    if (amount > kMaximumCountUVE || value > kMaximumCountUVE - amount) {
        return false;
    }
    value += amount;
    return true;
}

[[nodiscard]] std::string_view NextTokenUVE(std::string_view& rest) noexcept {
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front())) != 0) {
        rest.remove_prefix(1U);
    }
    const std::size_t end = rest.find_first_of(" \t\r\n");
    const std::string_view token = rest.substr(0U, end);
    rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end);
    return token;
}

[[nodiscard]] bool IsFaceTokenUVE(const std::string_view token) noexcept {
    if (token.empty() || token.find_first_of("/\\") == 0U || token.find_last_of("/") == token.size() - 1U) {
        return false;
    }
    for (const char character : token) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0 || character == '\\') {
            return false;
        }
    }
    return true;
}

} // namespace

bool ResolveObjIndexUVE(const std::int64_t rawIndex, const std::uint32_t attributeCount,
                        std::uint32_t& outZeroBasedIndex) noexcept {
    if (attributeCount == 0U || rawIndex == 0) {
        return false;
    }
    std::uint64_t zeroBasedIndex = 0U;
    if (rawIndex > 0) {
        const std::uint64_t oneBasedIndex = static_cast<std::uint64_t>(rawIndex);
        if (oneBasedIndex > static_cast<std::uint64_t>(attributeCount)) {
            return false;
        }
        zeroBasedIndex = oneBasedIndex - 1U;
    } else {
        const std::uint64_t relativeDistance =
            static_cast<std::uint64_t>(-(rawIndex + 1)) + 1U;
        if (relativeDistance > static_cast<std::uint64_t>(attributeCount)) {
            return false;
        }
        zeroBasedIndex = static_cast<std::uint64_t>(attributeCount) - relativeDistance;
    }
    outZeroBasedIndex = static_cast<std::uint32_t>(zeroBasedIndex);
    return true;
}

std::optional<ObjMetadataUVE> ParseObjMetadataUVE(const std::string_view source) {
    ObjMetadataUVE metadata;
    std::size_t lineStart = 0U;
    while (lineStart <= source.size()) {
        const std::size_t lineEnd = source.find('\n', lineStart);
        std::string_view line = source.substr(lineStart, lineEnd == std::string_view::npos
                                                        ? std::string_view::npos
                                                        : lineEnd - lineStart);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1U);
        }
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
            line.remove_prefix(1U);
        }
        if (!line.empty() && line.front() != '#') {
            std::string_view rest = line;
            const std::string_view directive = NextTokenUVE(rest);
            if (directive == "v" || directive == "vt" || directive == "vn") {
                const std::string_view first = NextTokenUVE(rest);
                const std::string_view second = NextTokenUVE(rest);
                const std::string_view third = NextTokenUVE(rest);
                if (first.empty() || second.empty() ||
                    ((directive == "v" || directive == "vn") && third.empty())) {
                    return std::nullopt;
                }
                std::uint32_t& count = directive == "v" ? metadata.positionCount
                                      : directive == "vt" ? metadata.texcoordCount
                                                            : metadata.normalCount;
                if (!IncrementUVE(count)) {
                    return std::nullopt;
                }
            } else if (directive == "f") {
                std::uint32_t faceVertices = 0U;
                while (true) {
                    const std::string_view token = NextTokenUVE(rest);
                    if (token.empty()) {
                        break;
                    }
                    if (!IsFaceTokenUVE(token) || !IncrementUVE(faceVertices)) {
                        return std::nullopt;
                    }
                }
                if (faceVertices < 3U || !IncrementUVE(metadata.faceCount) ||
                    !IncrementUVE(metadata.triangleCount, faceVertices - 2U)) {
                    return std::nullopt;
                }
            } else if (directive == "g") {
                if (NextTokenUVE(rest).empty() || !IncrementUVE(metadata.groupCount)) {
                    return std::nullopt;
                }
            } else if (directive == "usemtl") {
                if (NextTokenUVE(rest).empty() || !IncrementUVE(metadata.materialUseCount)) {
                    return std::nullopt;
                }
            } else if (directive == "mtllib") {
                if (NextTokenUVE(rest).empty() || !IncrementUVE(metadata.materialLibraryCount)) {
                    return std::nullopt;
                }
            } else if (!IncrementUVE(metadata.ignoredStatementCount)) {
                return std::nullopt;
            }
        }
        if (lineEnd == std::string_view::npos) {
            break;
        }
        lineStart = lineEnd + 1U;
    }
    return metadata;
}

} // namespace UVE::Asset
