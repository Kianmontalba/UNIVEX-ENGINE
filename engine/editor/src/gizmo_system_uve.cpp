// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/gizmo_system_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Editor::Gizmo {
namespace {
constexpr float kMinimumScreenScaleUVE = 0.06F;
constexpr float kMaximumScreenScaleUVE = 60.0F;

[[nodiscard]] Math::Vector3UVE BaseAxisUVE(const GizmoAxisUVE axis) noexcept {
    switch (axis) {
        case GizmoAxisUVE::X:
            return {1.0F, 0.0F, 0.0F};
        case GizmoAxisUVE::Y:
            return {0.0F, 1.0F, 0.0F};
        case GizmoAxisUVE::Z:
            return {0.0F, 0.0F, 1.0F};
        default:
            return {};
    }
}
} // namespace

GizmoLayerVisibilityUVE GizmoSystemUVE::LayersForUVE(const GizmoModeUVE mode) noexcept {
    switch (mode) {
        case GizmoModeUVE::Move:
            return {true, false, false};
        case GizmoModeUVE::Rotate:
            return {false, true, false};
        case GizmoModeUVE::Scale:
            return {false, false, true};
        case GizmoModeUVE::Universal:
            return {true, true, true};
    }
    return {};
}

Math::Vector3UVE GizmoSystemUVE::AxisDirectionUVE(const GizmoAxisUVE axis,
                                                   const Math::QuaternionUVE& targetRotation,
                                                   const GizmoSpaceUVE space) noexcept {
    const Math::Vector3UVE base = BaseAxisUVE(axis);
    if (space == GizmoSpaceUVE::World || axis == GizmoAxisUVE::None) {
        return base;
    }
    const Math::Vector3UVE direction = Math::RotateVectorUVE(targetRotation, base);
    const float lengthSquared = Math::LengthSquaredUVE(direction);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 0.00000001F) {
        return {};
    }
    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    return {direction.x * inverseLength, direction.y * inverseLength, direction.z * inverseLength};
}

float GizmoSystemUVE::ComputeScreenScaleUVE(const float cameraDistance) noexcept {
    if (!std::isfinite(cameraDistance)) {
        return kMinimumScreenScaleUVE;
    }
    return std::clamp(cameraDistance * 0.12F, kMinimumScreenScaleUVE, kMaximumScreenScaleUVE);
}

float GizmoSystemUVE::MoveRadiusMultiplierUVE() noexcept {
    return 1.0F;
}

float GizmoSystemUVE::RotateRadiusMultiplierUVE() noexcept {
    return 1.35F;
}

float GizmoSystemUVE::ScaleRadiusMultiplierUVE() noexcept {
    return 0.65F;
}

Math::Vector3UVE GizmoSystemUVE::AxisColorUVE(const GizmoAxisUVE axis, const bool hovered) noexcept {
    if (hovered) {
        return {1.0F, 0.85F, 0.20F};
    }
    switch (axis) {
        case GizmoAxisUVE::X:
            return {0.90F, 0.25F, 0.30F};
        case GizmoAxisUVE::Y:
            return {0.30F, 0.80F, 0.45F};
        case GizmoAxisUVE::Z:
            return {0.25F, 0.55F, 1.00F};
        case GizmoAxisUVE::XY:
            return {0.60F, 0.525F, 0.375F};
        case GizmoAxisUVE::YZ:
            return {0.275F, 0.675F, 0.725F};
        case GizmoAxisUVE::XZ:
            return {0.575F, 0.40F, 0.65F};
        default:
            return {0.85F, 0.85F, 0.85F};
    }
}

} // namespace UVE::Editor::Gizmo
