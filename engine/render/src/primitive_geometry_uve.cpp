// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/primitive_geometry_uve.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace UVE::Render {

namespace {

constexpr float kPrimitiveHalfExtentUVE = 0.5F;
constexpr std::uint32_t kSphereSlicesUVE = 24U;
constexpr std::uint32_t kSphereStacksUVE = 16U;
constexpr float kGeometryEpsilonUVE = 0.000001F;

void AddOutwardTriangleUVE(PrimitiveGeometryUVE& geometry, std::uint32_t first, std::uint32_t second,
                           std::uint32_t third) {
    const Math::Vector3UVE& a = geometry.vertices[first].position;
    const Math::Vector3UVE& b = geometry.vertices[second].position;
    const Math::Vector3UVE& c = geometry.vertices[third].position;
    const Math::Vector3UVE faceNormal = Math::CrossUVE(b - a, c - a);
    const Math::Vector3UVE expectedOutward = a + b + c;
    if (Math::DotUVE(faceNormal, expectedOutward) < 0.0F) {
        std::swap(second, third);
    }
    geometry.indices.push_back(first);
    geometry.indices.push_back(second);
    geometry.indices.push_back(third);
}

[[nodiscard]] PrimitiveGeometryUVE MakeCubeGeometryUVE() {
    PrimitiveGeometryUVE geometry;
    geometry.localBounds = Math::AabbUVE{{-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                         {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE}};
    struct FaceUVE final {
        Math::Vector3UVE normal;
        std::array<Math::Vector3UVE, 4> positions;
    };
    const std::array<FaceUVE, 6> faces{
        FaceUVE{{1.0F, 0.0F, 0.0F}, {{{kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE}}}},
        FaceUVE{{-1.0F, 0.0F, 0.0F}, {{{-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE}}}},
        FaceUVE{{0.0F, 1.0F, 0.0F}, {{{-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE}}}},
        FaceUVE{{0.0F, -1.0F, 0.0F}, {{{-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                       {kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                       {kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE}}}},
        FaceUVE{{0.0F, 0.0F, 1.0F}, {{{-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE},
                                      {kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE}}}},
        FaceUVE{{0.0F, 0.0F, -1.0F}, {{{kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                       {-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE}}}},
    };
    constexpr std::array<std::array<float, 2>, 4> uvs{{{{0.0F, 0.0F}}, {{1.0F, 0.0F}}, {{1.0F, 1.0F}}, {{0.0F, 1.0F}}}};
    geometry.vertices.reserve(faces.size() * 4U);
    geometry.indices.reserve(faces.size() * 6U);
    for (const FaceUVE& face : faces) {
        const std::uint32_t base = static_cast<std::uint32_t>(geometry.vertices.size());
        for (std::size_t vertexIndex = 0U; vertexIndex < face.positions.size(); ++vertexIndex) {
            geometry.vertices.push_back(Asset::MeshVertexUVE{face.positions[vertexIndex], face.normal,
                                                              uvs[vertexIndex][0], uvs[vertexIndex][1]});
        }
        AddOutwardTriangleUVE(geometry, base, base + 1U, base + 2U);
        AddOutwardTriangleUVE(geometry, base, base + 2U, base + 3U);
    }
    return geometry;
}

[[nodiscard]] PrimitiveGeometryUVE MakePlaneGeometryUVE() {
    PrimitiveGeometryUVE geometry;
    geometry.localBounds = Math::AabbUVE{{-kPrimitiveHalfExtentUVE, -kGeometryEpsilonUVE, -kPrimitiveHalfExtentUVE},
                                         {kPrimitiveHalfExtentUVE, kGeometryEpsilonUVE, kPrimitiveHalfExtentUVE}};
    const Math::Vector3UVE normal{0.0F, 1.0F, 0.0F};
    geometry.vertices = {
        Asset::MeshVertexUVE{{-kPrimitiveHalfExtentUVE, 0.0F, -kPrimitiveHalfExtentUVE}, normal, 0.0F, 0.0F},
        Asset::MeshVertexUVE{{kPrimitiveHalfExtentUVE, 0.0F, -kPrimitiveHalfExtentUVE}, normal, 1.0F, 0.0F},
        Asset::MeshVertexUVE{{kPrimitiveHalfExtentUVE, 0.0F, kPrimitiveHalfExtentUVE}, normal, 1.0F, 1.0F},
        Asset::MeshVertexUVE{{-kPrimitiveHalfExtentUVE, 0.0F, kPrimitiveHalfExtentUVE}, normal, 0.0F, 1.0F},
    };
    // The plane is origin-centered, so its triangle centroid cannot determine outward direction.
    // Author the fixed +Y winding explicitly instead of using AddOutwardTriangleUVE's radial test.
    geometry.indices = {0U, 2U, 1U, 0U, 3U, 2U};
    return geometry;
}

[[nodiscard]] PrimitiveGeometryUVE MakeUVSphereGeometryUVE() {
    PrimitiveGeometryUVE geometry;
    geometry.localBounds = Math::AabbUVE{{-kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE, -kPrimitiveHalfExtentUVE},
                                         {kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE, kPrimitiveHalfExtentUVE}};
    const std::uint32_t columns = kSphereSlicesUVE + 1U;
    geometry.vertices.reserve(static_cast<std::size_t>(columns) * (kSphereStacksUVE + 1U));
    geometry.indices.reserve(static_cast<std::size_t>(kSphereSlicesUVE) * (6U * (kSphereStacksUVE - 1U)));

    for (std::uint32_t stack = 0U; stack <= kSphereStacksUVE; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(kSphereStacksUVE);
        const float latitude = v * std::numbers::pi_v<float>;
        const float sineLatitude = std::sin(latitude);
        const float cosineLatitude = std::cos(latitude);
        for (std::uint32_t slice = 0U; slice <= kSphereSlicesUVE; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(kSphereSlicesUVE);
            const float longitude = u * 2.0F * std::numbers::pi_v<float>;
            const float sineLongitude = slice == kSphereSlicesUVE ? 0.0F : std::sin(longitude);
            const float cosineLongitude = slice == kSphereSlicesUVE ? 1.0F : std::cos(longitude);
            Math::Vector3UVE normal{sineLatitude * cosineLongitude, cosineLatitude,
                                    sineLatitude * sineLongitude};
            Math::Vector3UVE position = normal * kPrimitiveHalfExtentUVE;
            if (stack == 0U) {
                normal = Math::Vector3UVE{0.0F, 1.0F, 0.0F};
                position = normal * kPrimitiveHalfExtentUVE;
            } else if (stack == kSphereStacksUVE) {
                normal = Math::Vector3UVE{0.0F, -1.0F, 0.0F};
                position = normal * kPrimitiveHalfExtentUVE;
            }
            geometry.vertices.push_back(Asset::MeshVertexUVE{position, normal, u, v});
        }
    }

    const auto indexAt = [](const std::uint32_t stack, const std::uint32_t slice) noexcept {
        return stack * (kSphereSlicesUVE + 1U) + slice;
    };
    for (std::uint32_t slice = 0U; slice < kSphereSlicesUVE; ++slice) {
        AddOutwardTriangleUVE(geometry, indexAt(0U, slice), indexAt(1U, slice + 1U), indexAt(1U, slice));
    }
    for (std::uint32_t stack = 1U; stack + 1U < kSphereStacksUVE; ++stack) {
        for (std::uint32_t slice = 0U; slice < kSphereSlicesUVE; ++slice) {
            const std::uint32_t topLeft = indexAt(stack, slice);
            const std::uint32_t topRight = indexAt(stack, slice + 1U);
            const std::uint32_t bottomLeft = indexAt(stack + 1U, slice);
            const std::uint32_t bottomRight = indexAt(stack + 1U, slice + 1U);
            AddOutwardTriangleUVE(geometry, topLeft, topRight, bottomRight);
            AddOutwardTriangleUVE(geometry, topLeft, bottomRight, bottomLeft);
        }
    }
    for (std::uint32_t slice = 0U; slice < kSphereSlicesUVE; ++slice) {
        AddOutwardTriangleUVE(geometry, indexAt(kSphereStacksUVE - 1U, slice),
                               indexAt(kSphereStacksUVE - 1U, slice + 1U),
                               indexAt(kSphereStacksUVE, slice));
    }
    return geometry;
}

} // namespace

const PrimitiveGeometryUVE& GetPrimitiveGeometryUVE(const Scene::PrimitiveMeshKindUVE kind) noexcept {
    static const PrimitiveGeometryUVE cube = MakeCubeGeometryUVE();
    static const PrimitiveGeometryUVE sphere = MakeUVSphereGeometryUVE();
    static const PrimitiveGeometryUVE plane = MakePlaneGeometryUVE();
    switch (kind) {
        case Scene::PrimitiveMeshKindUVE::Cube:
            return cube;
        case Scene::PrimitiveMeshKindUVE::UVSphere:
            return sphere;
        case Scene::PrimitiveMeshKindUVE::Plane:
            return plane;
    }
    return cube;
}

} // namespace UVE::Render
