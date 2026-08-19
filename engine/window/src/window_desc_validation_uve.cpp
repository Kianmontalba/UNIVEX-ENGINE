// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/window_desc_validation_uve.h"
namespace UVE::Window {
bool ValidateWindowDescUVE(const WindowDescUVE& desc) noexcept {
    // glVersionMinor is unsigned, so its type already enforces the requested nonnegative rule.
    return desc.width > 0U && desc.height > 0U && !desc.title.empty() && desc.glVersionMajor >= 1U;
}
} // namespace UVE::Window
