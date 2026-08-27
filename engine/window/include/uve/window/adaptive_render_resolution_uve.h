// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace UVE::Window {

struct AdaptiveRenderResolutionLimitsUVE final {
    std::uint32_t maximumAxis = 1U;
    std::uint64_t maximumPixels = 1ULL;
};

struct AdaptiveRenderResolutionUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
};

/// Converts a positive drawable size into an aspect-preserving render-target size bounded by both
/// an axis limit and a total-pixel budget. Invalid sizes or invalid limits return {0,0}. The helper
/// is deterministic and platform-neutral so Android startup clamping and live desktop/Android
/// target resizing cannot drift into different policies.
[[nodiscard]] inline AdaptiveRenderResolutionUVE ComputeAdaptiveRenderResolutionUVE(
    const std::uint32_t drawableWidth, const std::uint32_t drawableHeight,
    const AdaptiveRenderResolutionLimitsUVE limits) noexcept {
    if (drawableWidth == 0U || drawableHeight == 0U || limits.maximumAxis == 0U || limits.maximumPixels == 0ULL) {
        return {};
    }

    const double width = static_cast<double>(drawableWidth);
    const double height = static_cast<double>(drawableHeight);
    const double pixelCount = width * height;
    const double pixelScale = std::sqrt(
        std::min(1.0, static_cast<double>(limits.maximumPixels) / pixelCount));
    const double axisScale = std::min(
        1.0, std::min(static_cast<double>(limits.maximumAxis) / width,
                      static_cast<double>(limits.maximumAxis) / height));
    const double scale = std::min(pixelScale, axisScale);
    if (!std::isfinite(scale) || scale <= 0.0) {
        return {};
    }

    return AdaptiveRenderResolutionUVE{
        std::max(1U, static_cast<std::uint32_t>(std::floor(width * scale))),
        std::max(1U, static_cast<std::uint32_t>(std::floor(height * scale))),
    };
}

} // namespace UVE::Window
