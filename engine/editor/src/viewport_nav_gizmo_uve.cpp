// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/viewport_nav_gizmo_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Editor::Gizmo {
namespace {
constexpr float kDegenerateThresholdUVE = 0.18F;
constexpr float kHitRadiusUVE = 11.0F;
constexpr float kPiUVE = 3.14159265358979323846F;
constexpr float kHalfPiUVE = kPiUVE * 0.5F;

[[nodiscard]] Math::Vector3UVE DirectionForIndexUVE(const std::size_t index) noexcept {
    switch (index) {
        case 0U:
            return {1.0F, 0.0F, 0.0F};
        case 1U:
            return {-1.0F, 0.0F, 0.0F};
        case 2U:
            return {0.0F, 1.0F, 0.0F};
        case 3U:
            return {0.0F, -1.0F, 0.0F};
        case 4U:
            return {0.0F, 0.0F, 1.0F};
        case 5U:
            return {0.0F, 0.0F, -1.0F};
        default:
            return {};
    }
}

[[nodiscard]] ViewportNavAxisUVE AxisForIndexUVE(const std::size_t index) noexcept {
    return index < 2U ? ViewportNavAxisUVE::X : (index < 4U ? ViewportNavAxisUVE::Y : ViewportNavAxisUVE::Z);
}

[[nodiscard]] bool IsPositiveIndexUVE(const std::size_t index) noexcept {
    return (index % 2U) == 0U;
}

[[nodiscard]] ViewportNavPresetUVE PresetForIndexUVE(const std::size_t index) noexcept {
    switch (index) {
        case 0U:
            return ViewportNavPresetUVE::Right;
        case 1U:
            return ViewportNavPresetUVE::Left;
        case 2U:
            return ViewportNavPresetUVE::Top;
        case 3U:
            return ViewportNavPresetUVE::Bottom;
        case 4U:
            return ViewportNavPresetUVE::Front;
        case 5U:
            return ViewportNavPresetUVE::Back;
        default:
            return ViewportNavPresetUVE::Front;
    }
}
}

void ViewportNavGizmoUVE::SetAnchorUVE(const Math::Vector2UVE anchor) noexcept {
    m_anchor = anchor;
}

Math::Vector2UVE ViewportNavGizmoUVE::GetAnchorUVE() const noexcept {
    return m_anchor;
}

float ViewportNavGizmoUVE::GetRadiusUVE() const noexcept {
    return m_radius;
}

float ViewportNavGizmoUVE::GetPlateRadiusUVE() const noexcept {
    return m_plateRadius;
}

Math::Vector2UVE ViewportNavGizmoUVE::ProjectAxisDirectionUVE(const Math::Vector3UVE axisDirection,
                                                              const float yawRadians,
                                                              const float pitchRadians) const noexcept {
    const float cosineYaw = std::cos(-yawRadians);
    const float sineYaw = std::sin(-yawRadians);
    const float x = axisDirection.x * cosineYaw + axisDirection.z * sineYaw;
    const float z = -axisDirection.x * sineYaw + axisDirection.z * cosineYaw;
    const float cosinePitch = std::cos(-pitchRadians);
    const float sinePitch = std::sin(-pitchRadians);
    const float y = axisDirection.y * cosinePitch - z * sinePitch;
    return {x, -y};
}

void ViewportNavGizmoUVE::UpdateLayoutUVE(const float cameraYawRadians,
                                          const float cameraPitchRadians) noexcept {
    for (std::size_t index = 0U; index < m_buttons.size(); ++index) {
        ViewportNavButtonUVE& button = m_buttons[index];
        const Math::Vector2UVE projected = ProjectAxisDirectionUVE(DirectionForIndexUVE(index), cameraYawRadians,
                                                                   cameraPitchRadians);
        const float lengthSquared = (projected.x * projected.x) + (projected.y * projected.y);
        const float length = std::sqrt(std::max(0.0F, lengthSquared));
        button.axis = AxisForIndexUVE(index);
        button.positive = IsPositiveIndexUVE(index);
        button.preset = PresetForIndexUVE(index);
        button.degenerate = !std::isfinite(length) || length < kDegenerateThresholdUVE;
        if (button.degenerate || length <= 0.0001F) {
            button.screenPosition = m_anchor;
        } else {
            const float inverseLength = 1.0F / length;
            const Math::Vector2UVE normalized{projected.x * inverseLength, projected.y * inverseLength};
            button.screenPosition = m_anchor + Math::Vector2UVE{normalized.x * m_radius, normalized.y * m_radius};
        }
    }
}

void ViewportNavGizmoUVE::UpdateHoverUVE(const Math::Vector2UVE mousePosition) noexcept {
    for (ViewportNavButtonUVE& button : m_buttons) {
        const Math::Vector2UVE delta = mousePosition - button.screenPosition;
        button.hovered = ((delta.x * delta.x) + (delta.y * delta.y)) < (kHitRadiusUVE * kHitRadiusUVE);
    }
}

bool ViewportNavGizmoUVE::HitTestPlateUVE(const Math::Vector2UVE mousePosition) const noexcept {
    const Math::Vector2UVE delta = mousePosition - m_anchor;
    return ((delta.x * delta.x) + (delta.y * delta.y)) < (m_plateRadius * m_plateRadius);
}

bool ViewportNavGizmoUVE::HandleClickUVE(const Math::Vector2UVE mousePosition,
                                         ViewportNavPresetUVE& outPreset) const noexcept {
    for (const ViewportNavButtonUVE& button : m_buttons) {
        const Math::Vector2UVE delta = mousePosition - button.screenPosition;
        if (((delta.x * delta.x) + (delta.y * delta.y)) < (kHitRadiusUVE * kHitRadiusUVE)) {
            outPreset = button.preset;
            return true;
        }
    }
    return false;
}

const std::array<ViewportNavButtonUVE, 6>& ViewportNavGizmoUVE::GetButtonsUVE() const noexcept {
    return m_buttons;
}

Math::Vector3UVE ViewportNavGizmoUVE::PresetDirectionUVE(const ViewportNavPresetUVE preset) noexcept {
    switch (preset) {
        case ViewportNavPresetUVE::Right:
            return {1.0F, 0.0F, 0.0F};
        case ViewportNavPresetUVE::Left:
            return {-1.0F, 0.0F, 0.0F};
        case ViewportNavPresetUVE::Top:
            return {0.0F, 1.0F, 0.0F};
        case ViewportNavPresetUVE::Bottom:
            return {0.0F, -1.0F, 0.0F};
        case ViewportNavPresetUVE::Front:
            return {0.0F, 0.0F, 1.0F};
        case ViewportNavPresetUVE::Back:
            return {0.0F, 0.0F, -1.0F};
    }
    return {0.0F, 0.0F, 1.0F};
}

void ViewportNavGizmoUVE::PresetAnglesUVE(const ViewportNavPresetUVE preset,
                                          float& outYawRadians,
                                          float& outPitchRadians) noexcept {
    outYawRadians = 0.0F;
    outPitchRadians = 0.0F;
    switch (preset) {
        case ViewportNavPresetUVE::Right:
            outYawRadians = kHalfPiUVE;
            break;
        case ViewportNavPresetUVE::Left:
            outYawRadians = -kHalfPiUVE;
            break;
        case ViewportNavPresetUVE::Top:
            outPitchRadians = -1.4835299F;
            break;
        case ViewportNavPresetUVE::Bottom:
            outPitchRadians = 1.4835299F;
            break;
        case ViewportNavPresetUVE::Front:
            break;
        case ViewportNavPresetUVE::Back:
            outYawRadians = kPiUVE;
            break;
    }
}

const char* ViewportNavGizmoUVE::AxisLabelUVE(const ViewportNavAxisUVE axis, const bool positive) noexcept {
    switch (axis) {
        case ViewportNavAxisUVE::X:
            return positive ? "X+" : "X-";
        case ViewportNavAxisUVE::Y:
            return positive ? "Y+" : "Y-";
        case ViewportNavAxisUVE::Z:
            return positive ? "Z+" : "Z-";
    }
    return "?";
}

std::uint32_t ViewportNavGizmoUVE::AxisColorUVE(const ViewportNavAxisUVE axis, const bool positive,
                                                 const bool hovered) noexcept {
    if (hovered) {
        return 0xFF46D9FF;
    }
    switch (axis) {
        case ViewportNavAxisUVE::X:
            return positive ? 0xFF5C5CDC : 0xA05C5CDC;
        case ViewportNavAxisUVE::Y:
            return positive ? 0xFF8BCD6E : 0xA08BCD6E;
        case ViewportNavAxisUVE::Z:
            return positive ? 0xFFE09B67 : 0xA0E09B67;
    }
    return 0xFFE5E5E5;
}

} // namespace UVE::Editor::Gizmo
