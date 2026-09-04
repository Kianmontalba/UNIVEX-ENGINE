// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/gizmo_overlay_geometry_uve.h"

#include <cmath>
#include <numbers>

namespace UVE::Render {

namespace {

constexpr float kShaftRadiusUVE = 0.035F;
constexpr float kShaftLengthUVE = 0.72F;
constexpr float kConeRadiusUVE = 0.09F;
constexpr float kConeHeightUVE = 0.28F;
constexpr std::uint32_t kSidesUVE = 14U;

/// Appends one triangle whose normal is exactly cross(b - a, c - a) - so the caller's vertex
/// order alone decides facing, with no separate winding-correction heuristic to keep in sync with
/// it (unlike AddOutwardTriangleUVE's centroid-direction test in primitive_geometry_uve.cpp, which
/// only applies to shapes centered on the origin; this arrow's shaft/cone are not). Every call
/// site below was hand-derived to pass vertices in the order that makes this cross product point
/// away from the mesh's solid interior.
void AddFlatTriangleUVE(PrimitiveGeometryUVE& geometry, const Math::Vector3UVE& a, const Math::Vector3UVE& b,
                        const Math::Vector3UVE& c) {
    const Math::Vector3UVE normal = Math::NormalizeUVE(Math::CrossUVE(b - a, c - a));
    const auto base = static_cast<std::uint32_t>(geometry.vertices.size());
    geometry.vertices.push_back(Asset::MeshVertexUVE{a, normal, 0.0F, 0.0F});
    geometry.vertices.push_back(Asset::MeshVertexUVE{b, normal, 0.0F, 0.0F});
    geometry.vertices.push_back(Asset::MeshVertexUVE{c, normal, 0.0F, 0.0F});
    geometry.indices.push_back(base);
    geometry.indices.push_back(base + 1U);
    geometry.indices.push_back(base + 2U);
}

[[nodiscard]] PrimitiveGeometryUVE MakeGizmoArrowGeometryUVE() {
    PrimitiveGeometryUVE geometry;
    // Local +Z, not +Y: TryMakeLookAtUVE() (the engine's only two-vector rotation builder) points
    // local +Z along a target direction, so authoring this mesh's own "forward" as +Z lets the
    // editor build each arrow's world orientation with that existing, tested utility instead of a
    // second, hand-derived rotate-Y-to-direction formula.
    const float tipZ = kShaftLengthUVE + kConeHeightUVE;
    geometry.localBounds = Math::AabbUVE{{-kConeRadiusUVE, -kConeRadiusUVE, 0.0F},
                                         {kConeRadiusUVE, kConeRadiusUVE, tipZ}};
    geometry.vertices.reserve(static_cast<std::size_t>(kSidesUVE) * 3U * 4U);
    geometry.indices.reserve(geometry.vertices.capacity());

    for (std::uint32_t side = 0U; side < kSidesUVE; ++side) {
        const float theta0 = static_cast<float>(side) / static_cast<float>(kSidesUVE) * 2.0F * std::numbers::pi_v<float>;
        const float theta1 = static_cast<float>(side + 1U) / static_cast<float>(kSidesUVE) * 2.0F * std::numbers::pi_v<float>;
        const float cos0 = std::cos(theta0);
        const float sin0 = std::sin(theta0);
        const float cos1 = std::cos(theta1);
        const float sin1 = std::sin(theta1);

        // Shaft (cylinder): bottom ring at z=0, top ring at z=kShaftLengthUVE.
        const Math::Vector3UVE shaftBottom0{kShaftRadiusUVE * cos0, kShaftRadiusUVE * sin0, 0.0F};
        const Math::Vector3UVE shaftBottom1{kShaftRadiusUVE * cos1, kShaftRadiusUVE * sin1, 0.0F};
        const Math::Vector3UVE shaftTop0{kShaftRadiusUVE * cos0, kShaftRadiusUVE * sin0, kShaftLengthUVE};
        const Math::Vector3UVE shaftTop1{kShaftRadiusUVE * cos1, kShaftRadiusUVE * sin1, kShaftLengthUVE};
        AddFlatTriangleUVE(geometry, shaftBottom0, shaftBottom1, shaftTop1);
        AddFlatTriangleUVE(geometry, shaftBottom0, shaftTop1, shaftTop0);

        // Cone head: base ring at z=kShaftLengthUVE (radius kConeRadiusUVE), apex at z=tipZ.
        const Math::Vector3UVE coneBase0{kConeRadiusUVE * cos0, kConeRadiusUVE * sin0, kShaftLengthUVE};
        const Math::Vector3UVE coneBase1{kConeRadiusUVE * cos1, kConeRadiusUVE * sin1, kShaftLengthUVE};
        const Math::Vector3UVE tip{0.0F, 0.0F, tipZ};
        AddFlatTriangleUVE(geometry, coneBase0, coneBase1, tip);

        // Cone base cap (backward-facing): closes the flare where kConeRadiusUVE exceeds
        // kShaftRadiusUVE, so the underside of the arrowhead is not an open hole.
        const Math::Vector3UVE capCenter{0.0F, 0.0F, kShaftLengthUVE};
        AddFlatTriangleUVE(geometry, capCenter, coneBase1, coneBase0);
    }
    return geometry;
}

} // namespace

const PrimitiveGeometryUVE& GetGizmoArrowGeometryUVE() noexcept {
    static const PrimitiveGeometryUVE arrow = MakeGizmoArrowGeometryUVE();
    return arrow;
}

} // namespace UVE::Render
