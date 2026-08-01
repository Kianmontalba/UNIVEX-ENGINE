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
#include <string>

namespace UVE::Window {

/// Describes the window WindowManagerUVE creates. Passed once at construction; there is no
/// "resize the desc" API — width/height change through the OS (user drag, SetFullscreenUVE()) and
/// are read back via IWindowManagerUVE::GetWidthUVE()/GetHeightUVE(), not by mutating this struct.
struct WindowDescUVE {
    std::string title = "UniVex Engine";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool resizable = true;
    bool vsyncEnabled = true;

    /// Requested OpenGL context version (WindowManagerUVE's real GLFW3 backend requests Core
    /// Profile at exactly this version via glfwWindowHint before creating the window). The
    /// production default is OpenGL 4.6 Core per the approved architecture decision. This is
    /// configurable — not hardcoded — because GL context availability is a real driver/platform
    /// fact: this project's own development sandbox (Mesa llvmpipe under Xvfb) caps at 4.5 Core
    /// and fails to create a 4.6 context (confirmed by direct testing), so verification here uses
    /// an explicit {4, 5} override without touching the shipped default a real GPU driver targets.
    std::uint32_t glVersionMajor = 4;
    std::uint32_t glVersionMinor = 6;
};

} // namespace UVE::Window
