//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>

namespace UVE::Window {

/// Published through Events::IEventSystemUVE when the user requests the window be closed (e.g.
/// clicking the OS close button). IWindowManagerUVE::IsCloseRequestedUVE() is the direct-poll
/// equivalent EngineCoreUVE drives its own quit sequence from; this event exists for any other
/// system that wants to observe the same moment (e.g. a future "save before quit?" prompt).
/// Publishing happens only from IWindowManagerUVE's own GLFW callback, never from inside GL code.
struct WindowCloseRequestedEventUVE {};

/// Published through Events::IEventSystemUVE whenever the window's framebuffer size changes.
/// Deliberately carries no side effects of its own — the GLFW callback that publishes this only
/// publishes; it never touches GL state or recreates GPU resources. Consumers (GlRenderDeviceUVE)
/// poll IWindowManagerUVE::GetWidthUVE()/GetHeightUVE() once per frame instead of reacting to this
/// event synchronously, per the approved design.
struct WindowResizedEventUVE {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

/// Published through Events::IEventSystemUVE whenever the window gains or loses OS input focus.
struct WindowFocusChangedEventUVE {
    bool focused = false;
};

} // namespace UVE::Window
