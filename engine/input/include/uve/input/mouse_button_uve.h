// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

namespace UVE::Input {

/// A mouse button. Deliberately minimal — Left/Right/Middle covers every near-term gameplay
/// need; extend (e.g. thumb buttons) when a real consumer needs it.
enum class MouseButtonUVE {
    Left,
    Right,
    Middle,

    Count, // Sentinel — array-sizes InputSystemUVE's mouse-button-state storage. Not a real button.
};

} // namespace UVE::Input
