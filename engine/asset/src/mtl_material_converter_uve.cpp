// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/mtl_material_converter_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <new>
#include <optional>
#include <string_view>
#include <utility>

#include "uve/asset/mtl_metadata_uve.h"

namespace UVE::Asset {
namespace {

[[nodiscard]] std::string_view NextTokenUVE(std::string_view& rest) noexcept {
    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
        rest.remove_prefix(1U);
    }
    const std::size_t end = rest.find_first_of(" \t");
    const std::string_view token = rest.substr(0U, end);
    rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end);
    return token;
}

[[nodiscard]] bool ParseDirectiveUVE(std::string_view line, std::string_view& outDirective) noexcept {
    const std::size_t commentStart = line.find('#');
    if (commentStart != std::string_view::npos) {
        line = line.substr(0U, commentStart);
    }
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1U);
    }
    std::string_view rest = line;
    outDirective = NextTokenUVE(rest);
    return !outDirective.empty();
}

[[nodiscard]] float AverageVectorUVE(const std::array<float, 3U>& value) noexcept {
    return (value[0] + value[1] + value[2]) / 3.0F;
}

[[nodiscard]] bool IsUnitIntervalUVE(const float value) noexcept {
    return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

} // namespace

bool ConvertMtlMaterialUVE(const std::string_view source, MaterialAssetUVE& outMaterial) {
    if (source.empty() || source.size() > kMaximumMtlMaterialSourceBytesUVE ||
        source.find('\0') != std::string_view::npos) {
        return false;
    }

    try {
        MaterialAssetUVE candidate;
        bool hasMaterialName = false;
        std::size_t lineStart = 0U;
        while (lineStart <= source.size()) {
            const std::size_t lineEnd = source.find('\n', lineStart);
            std::string_view line = source.substr(lineStart, lineEnd == std::string_view::npos
                                                        ? std::string_view::npos
                                                        : lineEnd - lineStart);
            if (!line.empty() && line.back() == '\r') {
                line.remove_suffix(1U);
            }
            const std::size_t commentStart = line.find('#');
            if (commentStart != std::string_view::npos) {
                line = line.substr(0U, commentStart);
            }
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
                line.remove_prefix(1U);
            }

            if (!line.empty()) {
                std::string_view directive;
                if (!ParseDirectiveUVE(line, directive)) {
                    return false;
                }
                if (directive == "newmtl") {
                    std::string_view rest = line;
                    static_cast<void>(NextTokenUVE(rest));
                    if (hasMaterialName) {
                        return false;
                    }
                    if (NextTokenUVE(rest).empty() || !NextTokenUVE(rest).empty()) {
                        return false;
                    }
                    hasMaterialName = true;
                } else if (!hasMaterialName) {
                    return false;
                } else if (directive.rfind("map_", 0U) == 0U) {
                    MtlTextureMapUVE map;
                    if (!ParseMtlTextureMapUVE(line, map)) {
                        return false;
                    }
                    // MaterialAssetUVE stores only AssetGuidUVE slots. The validated source path
                    // remains syntax-only until an explicit texture resolver/import policy exists.
                } else {
                    MtlMaterialPropertyUVE property;
                    if (!ParseMtlMaterialPropertyUVE(line, property)) {
                        return false;
                    }
                    if (directive == "Kd") {
                        candidate.albedoColor = Math::Vector3UVE{property.vectorValue[0], property.vectorValue[1],
                                                                 property.vectorValue[2]};
                    } else if (directive == "Ke") {
                        candidate.emissiveColor = Math::Vector3UVE{property.vectorValue[0], property.vectorValue[1],
                                                                   property.vectorValue[2]};
                    } else if (directive == "Ks") {
                        candidate.metallic = std::clamp(AverageVectorUVE(property.vectorValue), 0.0F, 1.0F);
                    } else if (directive == "Ns") {
                        const float normalized = std::clamp(property.scalarValue, 0.0F, 1000.0F) / 1000.0F;
                        candidate.roughness = std::clamp(1.0F - normalized, 0.04F, 1.0F);
                    } else if (directive == "d") {
                        if (!IsUnitIntervalUVE(property.scalarValue)) {
                            return false;
                        }
                        candidate.isTransparent = property.scalarValue < 1.0F;
                    } else if (directive == "Tr") {
                        if (!IsUnitIntervalUVE(property.scalarValue)) {
                            return false;
                        }
                        candidate.isTransparent = property.scalarValue > 0.0F;
                    }
                }
            }

            if (lineEnd == std::string_view::npos) {
                break;
            }
            lineStart = lineEnd + 1U;
        }

        if (!hasMaterialName) {
            return false;
        }
        outMaterial = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace UVE::Asset
