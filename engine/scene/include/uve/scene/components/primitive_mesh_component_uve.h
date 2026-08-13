// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cmath>
#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// Renderer-owned primitive geometry choices. The component stores only this stable semantic kind;
/// the immutable vertex/index data remains a render implementation detail and is never duplicated
/// into a scene document.
enum class PrimitiveMeshKindUVE : std::uint8_t {
    Cube = 0U,
    UVSphere = 1U,
    Plane = 2U,
};

/// A serializable, editor-authored reference to one deterministic built-in primitive mesh.
/// `baseColor` is bounded linear RGB used by the primitive lighting path; it is not a material
/// asset and therefore does not imply texture, normal-map, or PBR authoring support.
struct PrimitiveMeshComponentUVE final {
    PrimitiveMeshKindUVE kind = PrimitiveMeshKindUVE::Cube;
    Math::Vector3UVE baseColor{0.72F, 0.72F, 0.72F};
};

[[nodiscard]] constexpr bool IsPrimitiveMeshKindValidUVE(const PrimitiveMeshKindUVE kind) noexcept {
    switch (kind) {
        case PrimitiveMeshKindUVE::Cube:
        case PrimitiveMeshKindUVE::UVSphere:
        case PrimitiveMeshKindUVE::Plane:
            return true;
    }
    return false;
}

[[nodiscard]] inline bool IsPrimitiveBaseColorValidUVE(const Math::Vector3UVE& color) noexcept {
    return std::isfinite(color.x) && std::isfinite(color.y) && std::isfinite(color.z) && color.x >= 0.0F &&
           color.x <= 1.0F && color.y >= 0.0F && color.y <= 1.0F && color.z >= 0.0F && color.z <= 1.0F;
}

[[nodiscard]] inline bool IsPrimitiveMeshComponentValidUVE(const PrimitiveMeshComponentUVE& component) noexcept {
    return IsPrimitiveMeshKindValidUVE(component.kind) && IsPrimitiveBaseColorValidUVE(component.baseColor);
}

} // namespace UVE::Scene
