// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstdint>

namespace UVE::Window {

/// Validates and converts a signed backend framebuffer-size report before it crosses the public
/// uint32_t window-size boundary. Invalid or non-positive dimensions leave both outputs unchanged.
/// This does not query a backend, mutate a window, or publish a resize event.
[[nodiscard]] bool ValidateFramebufferSizeUVE(int width, int height, std::uint32_t& outWidth,
                                               std::uint32_t& outHeight) noexcept;

} // namespace UVE::Window
