// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/input/key_code_uve.h"
#include "uve/input/mouse_button_uve.h"

namespace UVE::Input {

/// Which physical input device family an InputBindingUVE reads from.
enum class InputBindingSourceUVE {
    Keyboard,
    Mouse,
};

/// One physical input bound into an InputActionUVE's positive or negative binding list. Only
/// `key` is meaningful when `source == Keyboard`; only `mouseButton` is meaningful when
/// `source == Mouse` — construct via KeyBindingUVE()/MouseBindingUVE() rather than setting both
/// fields directly, to avoid accidentally leaving the unused field in a misleading state.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct InputBindingUVE {
    InputBindingSourceUVE source = InputBindingSourceUVE::Keyboard;
    KeyCodeUVE key = KeyCodeUVE::Unknown;
    MouseButtonUVE mouseButton = MouseButtonUVE::Left;
};

/// Builds a keyboard InputBindingUVE.
[[nodiscard]] constexpr InputBindingUVE KeyBindingUVE(KeyCodeUVE key) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::Keyboard, key, MouseButtonUVE::Left};
}

/// Builds a mouse InputBindingUVE.
[[nodiscard]] constexpr InputBindingUVE MouseBindingUVE(MouseButtonUVE button) noexcept {
    return InputBindingUVE{InputBindingSourceUVE::Mouse, KeyCodeUVE::Unknown, button};
}

} // namespace UVE::Input
