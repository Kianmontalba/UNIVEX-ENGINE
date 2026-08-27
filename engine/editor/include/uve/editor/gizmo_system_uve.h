// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstdint>

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor::Gizmo {

enum class GizmoModeUVE : std::uint8_t {
    Move,
    Rotate,
    Scale,
    Universal,
};

enum class GizmoAxisUVE : std::uint8_t {
    None,
    X,
    Y,
    Z,
    XY,
    YZ,
    XZ,
    XYZ,
    ScreenRotate,
};

enum class GizmoSpaceUVE : std::uint8_t {
    Local,
    World,
};

struct GizmoLayerVisibilityUVE final {
    bool move = false;
    bool rotate = false;
    bool scale = false;
};

class GizmoSystemUVE final {
public:
    [[nodiscard]] static GizmoLayerVisibilityUVE LayersForUVE(GizmoModeUVE mode) noexcept;

    [[nodiscard]] static Math::Vector3UVE AxisDirectionUVE(
        GizmoAxisUVE axis, const Math::QuaternionUVE& targetRotation, GizmoSpaceUVE space) noexcept;

    [[nodiscard]] static float ComputeScreenScaleUVE(float cameraDistance) noexcept;
    [[nodiscard]] static float MoveRadiusMultiplierUVE() noexcept;
    [[nodiscard]] static float RotateRadiusMultiplierUVE() noexcept;
    [[nodiscard]] static float ScaleRadiusMultiplierUVE() noexcept;

    [[nodiscard]] static Math::Vector3UVE AxisColorUVE(GizmoAxisUVE axis, bool hovered) noexcept;
};

} // namespace UVE::Editor::Gizmo
