// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <array>
#include <cstdint>

#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor::Gizmo {

enum class ViewportNavAxisUVE : std::uint8_t {
    X,
    Y,
    Z,
};

enum class ViewportNavPresetUVE : std::uint8_t {
    Right,
    Left,
    Top,
    Bottom,
    Front,
    Back,
};

struct ViewportNavButtonUVE final {
    ViewportNavAxisUVE axis = ViewportNavAxisUVE::X;
    bool positive = true;
    ViewportNavPresetUVE preset = ViewportNavPresetUVE::Right;
    bool degenerate = false;
    bool hovered = false;
    Math::Vector2UVE screenPosition{};
};

class ViewportNavGizmoUVE final {
public:
    void SetAnchorUVE(Math::Vector2UVE anchor) noexcept;
    [[nodiscard]] Math::Vector2UVE GetAnchorUVE() const noexcept;
    [[nodiscard]] float GetRadiusUVE() const noexcept;
    [[nodiscard]] float GetPlateRadiusUVE() const noexcept;

    void UpdateLayoutUVE(float cameraYawRadians, float cameraPitchRadians) noexcept;
    void UpdateHoverUVE(Math::Vector2UVE mousePosition) noexcept;

    [[nodiscard]] bool HitTestPlateUVE(Math::Vector2UVE mousePosition) const noexcept;
    [[nodiscard]] bool HandleClickUVE(Math::Vector2UVE mousePosition,
                                      ViewportNavPresetUVE& outPreset) const noexcept;

    [[nodiscard]] const std::array<ViewportNavButtonUVE, 6>& GetButtonsUVE() const noexcept;
    [[nodiscard]] static Math::Vector3UVE PresetDirectionUVE(ViewportNavPresetUVE preset) noexcept;
    static void PresetAnglesUVE(ViewportNavPresetUVE preset, float& outYawRadians,
                                float& outPitchRadians) noexcept;
    [[nodiscard]] static const char* AxisLabelUVE(ViewportNavAxisUVE axis, bool positive) noexcept;
    [[nodiscard]] static std::uint32_t AxisColorUVE(ViewportNavAxisUVE axis, bool positive,
                                                     bool hovered) noexcept;

private:
    [[nodiscard]] Math::Vector2UVE ProjectAxisDirectionUVE(Math::Vector3UVE axisDirection,
                                                           float yawRadians, float pitchRadians) const noexcept;

    Math::Vector2UVE m_anchor{};
    float m_radius = 32.0F;
    float m_plateRadius = 47.0F;
    std::array<ViewportNavButtonUVE, 6> m_buttons{};
};

} // namespace UVE::Editor::Gizmo
