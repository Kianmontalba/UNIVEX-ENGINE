// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/framebuffer_size_validation_uve.h"

namespace UVE::Window {

bool ValidateFramebufferSizeUVE(const int width, const int height, std::uint32_t& outWidth,
                               std::uint32_t& outHeight) noexcept {
    if (width <= 0 || height <= 0) {
        return false;
    }

    outWidth = static_cast<std::uint32_t>(width);
    outHeight = static_cast<std::uint32_t>(height);
    return true;
}

} // namespace UVE::Window
