// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

namespace UVE::Input {

/// A physical keyboard key. Deliberately not exhaustive of every scancode a real backend might
/// eventually report — covers the common gameplay-relevant set (letters, digits, arrows, the
/// usual modifiers, function keys); extend when a real WindowManagerUVE-sourced backend needs
/// more, following the same "grow when a real consumer needs it" discipline used throughout this
/// codebase.
enum class KeyCodeUVE {
    Unknown = 0,

    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    Up, Down, Left, Right,

    Space, Enter, Escape, Tab, Backspace,

    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Count, // Sentinel — array-sizes InputSystemUVE's key-state storage. Not a real key.
};

} // namespace UVE::Input
