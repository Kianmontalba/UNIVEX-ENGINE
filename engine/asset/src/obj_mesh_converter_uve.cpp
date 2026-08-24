// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/obj_mesh_converter_uve.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/obj_metadata_uve.h"
#include "uve/math/vector2_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Asset {
namespace {

constexpr float kDegenerateTriangleEpsilonSquaredUVE = 0.00000001F;

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFiniteBoundsUVE(const Math::AabbUVE& bounds) noexcept {
    return IsFiniteVectorUVE(bounds.min) && IsFiniteVectorUVE(bounds.max) &&
           bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

[[nodiscard]] std::string_view NextTokenUVE(std::string_view& rest) noexcept {
    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
        rest.remove_prefix(1U);
    }
    const std::size_t end = rest.find_first_of(" \t");
    const std::string_view token = rest.substr(0U, end);
    rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end);
    return token;
}

[[nodiscard]] std::optional<float> ParseFiniteFloatUVE(const std::string_view text) noexcept {
    if (text.empty()) {
        return std::nullopt;
    }
    float value = 0.0F;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value,
                                        std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] bool ParsePositionUVE(std::string_view rest, Math::Vector3UVE& outPosition) noexcept {
    const std::optional<float> x = ParseFiniteFloatUVE(NextTokenUVE(rest));
    const std::optional<float> y = ParseFiniteFloatUVE(NextTokenUVE(rest));
    const std::optional<float> z = ParseFiniteFloatUVE(NextTokenUVE(rest));
    if (!x || !y || !z) {
        return false;
    }

    const std::string_view weightText = NextTokenUVE(rest);
    if (!weightText.empty()) {
        const std::optional<float> weight = ParseFiniteFloatUVE(weightText);
        if (!weight || std::abs(*weight) <= kDegenerateTriangleEpsilonSquaredUVE) {
            return false;
        }
        outPosition = Math::Vector3UVE{*x / *weight, *y / *weight, *z / *weight};
    } else {
        outPosition = Math::Vector3UVE{*x, *y, *z};
    }
    if (!NextTokenUVE(rest).empty()) {
        return false;
    }
    return std::isfinite(outPosition.x) && std::isfinite(outPosition.y) && std::isfinite(outPosition.z);
}

[[nodiscard]] bool ParseTexcoordUVE(std::string_view rest, Math::Vector2UVE& outTexcoord) noexcept {
    const std::optional<float> u = ParseFiniteFloatUVE(NextTokenUVE(rest));
    const std::optional<float> v = ParseFiniteFloatUVE(NextTokenUVE(rest));
    if (!u || !v) {
        return false;
    }
    if (!NextTokenUVE(rest).empty()) {
        return false;
    }
    outTexcoord = Math::Vector2UVE{*u, *v};
    return true;
}

[[nodiscard]] bool ParseNormalUVE(std::string_view rest, Math::Vector3UVE& outNormal) noexcept {
    const std::optional<float> x = ParseFiniteFloatUVE(NextTokenUVE(rest));
    const std::optional<float> y = ParseFiniteFloatUVE(NextTokenUVE(rest));
    const std::optional<float> z = ParseFiniteFloatUVE(NextTokenUVE(rest));
    if (!x || !y || !z) {
        return false;
    }
    if (!NextTokenUVE(rest).empty()) {
        return false;
    }
    outNormal = Math::Vector3UVE{*x, *y, *z};
    const float lengthSquared = Math::LengthSquaredUVE(outNormal);
    return std::isfinite(lengthSquared) && lengthSquared > kDegenerateTriangleEpsilonSquaredUVE;
}

void UpdateBoundsUVE(const Math::Vector3UVE& position, bool& hasBounds, Math::AabbUVE& bounds) noexcept {
    if (!hasBounds) {
        bounds = Math::AabbUVE{position, position};
        hasBounds = true;
        return;
    }
    bounds.min.x = std::min(bounds.min.x, position.x);
    bounds.min.y = std::min(bounds.min.y, position.y);
    bounds.min.z = std::min(bounds.min.z, position.z);
    bounds.max.x = std::max(bounds.max.x, position.x);
    bounds.max.y = std::max(bounds.max.y, position.y);
    bounds.max.z = std::max(bounds.max.z, position.z);
}

[[nodiscard]] Math::Vector3UVE ComputeFaceNormalUVE(const Math::Vector3UVE& first,
                                                     const Math::Vector3UVE& second,
                                                     const Math::Vector3UVE& third) noexcept {
    const Math::Vector3UVE firstEdge = second - first;
    const Math::Vector3UVE secondEdge = third - first;
    if (!IsFiniteVectorUVE(firstEdge) || !IsFiniteVectorUVE(secondEdge)) {
        return {};
    }
    const Math::Vector3UVE cross = Math::CrossUVE(firstEdge, secondEdge);
    const float lengthSquared = Math::LengthSquaredUVE(cross);
    if (!std::isfinite(lengthSquared) || lengthSquared <= kDegenerateTriangleEpsilonSquaredUVE) {
        return {};
    }
    const Math::Vector3UVE normalized = Math::NormalizeUVE(cross);
    return IsFiniteVectorUVE(normalized) ? normalized : Math::Vector3UVE{};
}

} // namespace

bool ConvertObjMeshUVE(const std::string_view source, MeshAssetUVE& outMesh) {
    if (source.empty() || source.size() > kMaximumObjMeshSourceBytesUVE) {
        return false;
    }

    std::vector<Math::Vector3UVE> positions;
    std::vector<Math::Vector2UVE> texcoords;
    std::vector<Math::Vector3UVE> normals;
    positions.reserve(64U);
    texcoords.reserve(64U);
    normals.reserve(64U);

    MeshAssetUVE candidate;
    bool hasBounds = false;
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
            std::string_view rest = line;
            const std::string_view directive = NextTokenUVE(rest);
            if (directive == "v") {
                if (positions.size() >= kMaximumObjMeshVerticesUVE) {
                    return false;
                }
                Math::Vector3UVE position;
                if (!ParsePositionUVE(rest, position)) {
                    return false;
                }
                positions.push_back(position);
            } else if (directive == "vt") {
                if (texcoords.size() >= kMaximumObjMeshVerticesUVE) {
                    return false;
                }
                Math::Vector2UVE texcoord;
                if (!ParseTexcoordUVE(rest, texcoord)) {
                    return false;
                }
                texcoords.push_back(texcoord);
            } else if (directive == "vn") {
                if (normals.size() >= kMaximumObjMeshVerticesUVE) {
                    return false;
                }
                Math::Vector3UVE normal;
                if (!ParseNormalUVE(rest, normal)) {
                    return false;
                }
                normals.push_back(Math::NormalizeUVE(normal));
            } else if (directive == "f") {
                std::vector<ObjFaceVertexUVE> faceVertices;
                faceVertices.reserve(8U);
                while (true) {
                    const std::string_view token = NextTokenUVE(rest);
                    if (token.empty()) {
                        break;
                    }
                    if (faceVertices.size() >= kMaximumObjMeshVerticesUVE / 3U + 2U) {
                        return false;
                    }
                    ObjFaceVertexUVE faceVertex;
                    if (!ResolveObjFaceVertexUVE(token, static_cast<std::uint32_t>(positions.size()),
                                                 static_cast<std::uint32_t>(texcoords.size()),
                                                 static_cast<std::uint32_t>(normals.size()), faceVertex)) {
                        return false;
                    }
                    faceVertices.push_back(faceVertex);
                }
                if (faceVertices.size() < 3U) {
                    return false;
                }

                for (std::size_t triangleIndex = 1U; triangleIndex + 1U < faceVertices.size(); ++triangleIndex) {
                    const std::array<ObjFaceVertexUVE, 3> triangle{
                        faceVertices[0], faceVertices[triangleIndex], faceVertices[triangleIndex + 1U]};
                    const Math::Vector3UVE firstPosition = positions[triangle[0].positionIndex];
                    const Math::Vector3UVE secondPosition = positions[triangle[1].positionIndex];
                    const Math::Vector3UVE thirdPosition = positions[triangle[2].positionIndex];
                    const Math::Vector3UVE faceNormal =
                        ComputeFaceNormalUVE(firstPosition, secondPosition, thirdPosition);
                    if (Math::LengthSquaredUVE(faceNormal) <= kDegenerateTriangleEpsilonSquaredUVE ||
                        candidate.vertices.size() > kMaximumObjMeshVerticesUVE - 3U) {
                        return false;
                    }

                    const std::array<Math::Vector3UVE, 3> trianglePositions{
                        firstPosition, secondPosition, thirdPosition};
                    for (std::size_t corner = 0U; corner < triangle.size(); ++corner) {
                        const ObjFaceVertexUVE& faceVertex = triangle[corner];
                        MeshVertexUVE vertex;
                        vertex.position = trianglePositions[corner];
                        vertex.normal = faceVertex.normalIndex.has_value()
                                            ? normals[*faceVertex.normalIndex]
                                            : faceNormal;
                        if (faceVertex.texcoordIndex.has_value()) {
                            const Math::Vector2UVE& texcoord = texcoords[*faceVertex.texcoordIndex];
                            vertex.u = texcoord.x;
                            vertex.v = texcoord.y;
                        }
                        candidate.vertices.push_back(vertex);
                        candidate.indices.push_back(static_cast<std::uint32_t>(candidate.vertices.size() - 1U));
                        UpdateBoundsUVE(vertex.position, hasBounds, candidate.localBounds);
                    }
                }
            }
        }

        if (lineEnd == std::string_view::npos) {
            break;
        }
        lineStart = lineEnd + 1U;
    }

    if (candidate.vertices.empty() || candidate.indices.size() != candidate.vertices.size() || !hasBounds) {
        return false;
    }
    if (!TryGenerateMeshTangentsUVE(candidate.vertices, candidate.indices)) {
        return false;
    }
    if (!IsFiniteBoundsUVE(candidate.localBounds)) {
        return false;
    }
    for (const MeshVertexUVE& vertex : candidate.vertices) {
        if (!IsFiniteVectorUVE(vertex.position) || !IsFiniteVectorUVE(vertex.normal) ||
            !std::isfinite(vertex.u) || !std::isfinite(vertex.v) || !IsFiniteVectorUVE(vertex.tangent) ||
            !std::isfinite(vertex.tangentHandedness)) {
            return false;
        }
    }
    outMesh = std::move(candidate);
    return true;
}

} // namespace UVE::Asset
