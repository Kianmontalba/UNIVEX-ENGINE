// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace UVE::Window {

inline constexpr std::uint32_t kMaximumAndroidSurfaceAxisUVE = 2048U;
inline constexpr std::uint64_t kMaximumAndroidRenderTargetPixelsUVE = 1280ULL * 720ULL;

struct AndroidSurfaceSizeUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
};

/// Converts a native Android surface size into a positive, bounded render-target size. Invalid
/// sizes return {0,0}; oversized sizes are uniformly reduced to stay within both the per-axis and
/// total-pixel budgets used by the low-device Android profile.
[[nodiscard]] inline AndroidSurfaceSizeUVE ClampAndroidSurfaceSizeUVE(const std::int32_t rawWidth,
                                                                       const std::int32_t rawHeight) noexcept {
    if (rawWidth <= 0 || rawHeight <= 0) {
        return {};
    }

    const float width = static_cast<float>(rawWidth);
    const float height = static_cast<float>(rawHeight);
    const float pixelScale = std::sqrt(
        std::min(1.0F, static_cast<float>(kMaximumAndroidRenderTargetPixelsUVE) / (width * height)));
    const float axisScale = std::min(
        1.0F, std::min(static_cast<float>(kMaximumAndroidSurfaceAxisUVE) / width,
                       static_cast<float>(kMaximumAndroidSurfaceAxisUVE) / height));
    const float scale = std::min(pixelScale, axisScale);

    return AndroidSurfaceSizeUVE{
        std::max(1U, static_cast<std::uint32_t>(std::floor(width * scale))),
        std::max(1U, static_cast<std::uint32_t>(std::floor(height * scale))),
    };
}

} // namespace UVE::Window
