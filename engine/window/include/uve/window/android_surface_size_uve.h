// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>

#include "uve/window/adaptive_render_resolution_uve.h"

namespace UVE::Window {

inline constexpr std::uint32_t kMaximumAndroidSurfaceAxisUVE = 2048U;
inline constexpr std::uint64_t kMaximumAndroidRenderTargetPixelsUVE = 1280ULL * 720ULL;

using AndroidSurfaceSizeUVE = AdaptiveRenderResolutionUVE;

/// Converts a native Android surface size into a positive, bounded render-target size. Invalid
/// sizes return {0,0}; oversized sizes are uniformly reduced to stay within both the per-axis and
/// total-pixel budgets used by the low-device Android profile.
[[nodiscard]] inline AndroidSurfaceSizeUVE ClampAndroidSurfaceSizeUVE(const std::int32_t rawWidth,
                                                                       const std::int32_t rawHeight) noexcept {
    if (rawWidth <= 0 || rawHeight <= 0) {
        return {};
    }
    return ComputeAdaptiveRenderResolutionUVE(
        static_cast<std::uint32_t>(rawWidth), static_cast<std::uint32_t>(rawHeight),
        AdaptiveRenderResolutionLimitsUVE{kMaximumAndroidSurfaceAxisUVE,
                                          kMaximumAndroidRenderTargetPixelsUVE});
}

} // namespace UVE::Window
