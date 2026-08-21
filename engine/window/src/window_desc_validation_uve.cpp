// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/window_desc_validation_uve.h"
#include "uve/window/display_mode_validation_uve.h"
namespace UVE::Window {
bool ValidateWindowDescUVE(const WindowDescUVE& desc) noexcept {
    // Keep dimensions within the shared display-mode axis cap before the GLFW int narrowing cast.
    // glVersionMinor is unsigned, so its type already enforces the requested nonnegative rule.
    return desc.width > 0U && desc.width <= kMaximumDisplayModeAxisUVE && desc.height > 0U &&
           desc.height <= kMaximumDisplayModeAxisUVE && !desc.title.empty() &&
           desc.title.size() <= kMaximumWindowTitleBytesUVE && desc.title.find('\0') == std::string::npos &&
           desc.glVersionMajor >= 1U;
}
} // namespace UVE::Window
