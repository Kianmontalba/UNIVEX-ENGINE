// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>

#include "uve/input/i_gamepad_input_system_uve.h"
#include "uve/input/key_code_uve.h"
#include "uve/input/mouse_button_uve.h"

namespace UVE::Input {

/// Which physical input device family an InputBindingUVE reads from.
enum class InputBindingSourceUVE {
    Keyboard,
    Mouse,
    GamepadButton,
    GamepadAxis,
};

/// One physical input bound into an InputActionUVE's positive or negative binding list. Only
/// `key` is meaningful when `source == Keyboard`; only `mouseButton` is meaningful when
/// `source == Mouse`; `gamepadIndex`/`gamepadButton` are meaningful for GamepadButton; and
/// `gamepadIndex`/`gamepadAxis`/`scale` are meaningful for GamepadAxis — construct via the helper
/// functions rather than setting unused fields directly.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct InputBindingUVE {
    InputBindingSourceUVE source = InputBindingSourceUVE::Keyboard;
    KeyCodeUVE key = KeyCodeUVE::Unknown;
    MouseButtonUVE mouseButton = MouseButtonUVE::Left;
    std::size_t gamepadIndex = 0U;
    GamepadButtonUVE gamepadButton = GamepadButtonUVE::South;
    GamepadAxisUVE gamepadAxis = GamepadAxisUVE::LeftX;
    float scale = 1.0F;
};

/// Builds a keyboard InputBindingUVE.
[[nodiscard]] constexpr InputBindingUVE KeyBindingUVE(KeyCodeUVE key) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::Keyboard, key, MouseButtonUVE::Left, 0U,
                          GamepadButtonUVE::South, GamepadAxisUVE::LeftX, 1.0F};
}

/// Builds a mouse InputBindingUVE.
[[nodiscard]] constexpr InputBindingUVE MouseBindingUVE(MouseButtonUVE button) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::Mouse, KeyCodeUVE::Unknown, button, 0U,
                          GamepadButtonUVE::South, GamepadAxisUVE::LeftX, 1.0F};
}

[[nodiscard]] constexpr InputBindingUVE GamepadButtonBindingUVE(
    std::size_t gamepadIndex, GamepadButtonUVE button) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::GamepadButton, KeyCodeUVE::Unknown, MouseButtonUVE::Left,
                           gamepadIndex, button, GamepadAxisUVE::LeftX, 1.0F};
}

[[nodiscard]] constexpr InputBindingUVE GamepadAxisBindingUVE(
    std::size_t gamepadIndex, GamepadAxisUVE axis, float scale = 1.0F) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::GamepadAxis, KeyCodeUVE::Unknown, MouseButtonUVE::Left,
                           gamepadIndex, GamepadButtonUVE::South, axis, scale};
}

} // namespace UVE::Input
