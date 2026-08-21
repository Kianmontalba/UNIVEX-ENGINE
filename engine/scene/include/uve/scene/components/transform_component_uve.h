// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// The authored, parent-relative (local) position/rotation/scale of a scene-graph entity —
/// exactly what a Node3D "requires" per the master spec (Position, Rotation, Scale). Attach via
/// SceneGraphUVE::AttachTransformUVE() rather than directly, so the paired
/// WorldTransformComponentUVE/HierarchyComponentUVE are never forgotten. Setting this directly
/// via IEntityManagerUVE::GetComponentUVE() does NOT mark the entity dirty — use
/// SceneGraphUVE::SetLocalTransformUVE() to change it so world-transform propagation stays
/// correct.
struct TransformComponentUVE final {
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    Math::Vector3UVE localScale{1.0F, 1.0F, 1.0F};
};

/// Validates authored local transforms before scene persistence or graph propagation. Position and
/// scale must be finite; rotation must be finite and already normalized so composition cannot
/// silently introduce non-rotational scale or non-finite world state.
[[nodiscard]] inline bool IsTransformComponentValidUVE(const TransformComponentUVE& transform) noexcept {
    const auto isFiniteVector = [](const Math::Vector3UVE& value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!isFiniteVector(transform.localPosition) || !isFiniteVector(transform.localScale) ||
        !Math::IsFiniteUVE(transform.localRotation)) {
        return false;
    }
    const float rotationLengthSquared = Math::LengthSquaredUVE(transform.localRotation);
    return std::isfinite(rotationLengthSquared) &&
           std::abs(rotationLengthSquared - 1.0F) <= 1.0e-3F;
}

} // namespace UVE::Scene
