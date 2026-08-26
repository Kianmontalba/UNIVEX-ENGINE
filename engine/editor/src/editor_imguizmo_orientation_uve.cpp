// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "editor_imguizmo_orientation_uve.h"

#include <algorithm>
#include <cmath>

#include <imGuIZMOquat.h>

namespace UVE::Editor {

bool DrawViewportOrientationWidgetImGuIZMOUVE(const float screenX, const float screenY, const float size,
                                              float& yawRadians, float& pitchRadians) noexcept {
    if (!std::isfinite(screenX) || !std::isfinite(screenY) || !std::isfinite(size) || size <= 0.0F ||
        !std::isfinite(yawRadians) || !std::isfinite(pitchRadians)) {
        return false;
    }

    const float halfYaw = yawRadians * 0.5F;
    const float halfPitch = pitchRadians * 0.5F;
    const vgm::Quat yawRotation{std::cos(halfYaw), 0.0F, std::sin(halfYaw), 0.0F};
    const vgm::Quat pitchRotation{std::cos(halfPitch), std::sin(halfPitch), 0.0F, 0.0F};
    vgm::Quat orientation = yawRotation * pitchRotation;

    const ImVec2 previousCursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2{screenX, screenY});
    const bool changed = ImGui::gizmo3D("##uve_camera_orientation", orientation, size,
                                         imguiGizmo::mode3Axes | imguiGizmo::cubeAtOrigin);
    ImGui::SetCursorScreenPos(previousCursor);

    if (!changed) {
        return false;
    }

    const vgm::Vec3 forward = orientation * vgm::Vec3{0.0F, 0.0F, -1.0F};
    const float clampedForwardY = std::clamp(forward.y, -1.0F, 1.0F);
    yawRadians = std::atan2(-forward.x, -forward.z);
    pitchRadians = std::asin(clampedForwardY);
    return std::isfinite(yawRadians) && std::isfinite(pitchRadians);
}

} // namespace UVE::Editor
