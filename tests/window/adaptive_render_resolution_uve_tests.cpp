// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cmath>

#include <gtest/gtest.h>

#include "uve/window/adaptive_render_resolution_uve.h"

namespace UVE::Window {
namespace {

TEST(AdaptiveRenderResolutionUVETest, InvalidInputsReturnZeroSize) {
    const AdaptiveRenderResolutionLimitsUVE limits{2048U, 1280ULL * 720ULL};
    EXPECT_EQ(ComputeAdaptiveRenderResolutionUVE(0U, 720U, limits).width, 0U);
    EXPECT_EQ(ComputeAdaptiveRenderResolutionUVE(1280U, 0U, limits).height, 0U);
    EXPECT_EQ(ComputeAdaptiveRenderResolutionUVE(1280U, 720U, AdaptiveRenderResolutionLimitsUVE{0U, 1ULL}).width, 0U);
    EXPECT_EQ(ComputeAdaptiveRenderResolutionUVE(1280U, 720U, AdaptiveRenderResolutionLimitsUVE{2048U, 0ULL}).height, 0U);
}

TEST(AdaptiveRenderResolutionUVETest, SmallPortraitSurfaceScalesToAndroidBudgetAndPreservesAspect) {
    const AdaptiveRenderResolutionUVE result = ComputeAdaptiveRenderResolutionUVE(
        720U, 1600U, AdaptiveRenderResolutionLimitsUVE{2048U, 1280ULL * 720ULL});
    // The portrait surface exceeds the Android pixel budget, so it scales down uniformly.
    EXPECT_EQ(result.width, 643U);
    EXPECT_EQ(result.height, 1431U);
    EXPECT_NEAR(static_cast<double>(result.width) / static_cast<double>(result.height), 0.45, 0.001);
}

TEST(AdaptiveRenderResolutionUVETest, AndroidPolicyCapsOversizedLandscapeSurface) {
    const AdaptiveRenderResolutionUVE result = ComputeAdaptiveRenderResolutionUVE(
        2400U, 1080U, AdaptiveRenderResolutionLimitsUVE{2048U, 1280ULL * 720ULL});
    EXPECT_EQ(result.width, 1431U);
    EXPECT_EQ(result.height, 643U);
    EXPECT_LE(static_cast<std::uint64_t>(result.width) * result.height, 1280ULL * 720ULL);
    EXPECT_LE(result.width, 2048U);
    EXPECT_LE(result.height, 2048U);
}

TEST(AdaptiveRenderResolutionUVETest, DesktopFourKSurfaceRemainsNativeResolution) {
    const AdaptiveRenderResolutionUVE result = ComputeAdaptiveRenderResolutionUVE(
        3840U, 2160U, AdaptiveRenderResolutionLimitsUVE{8192U, 3840ULL * 2160ULL});
    EXPECT_EQ(result.width, 3840U);
    EXPECT_EQ(result.height, 2160U);
}

TEST(AdaptiveRenderResolutionUVETest, DesktopOversizedSurfaceScalesToPixelBudget) {
    const AdaptiveRenderResolutionUVE result = ComputeAdaptiveRenderResolutionUVE(
        5120U, 2880U, AdaptiveRenderResolutionLimitsUVE{8192U, 3840ULL * 2160ULL});
    EXPECT_EQ(result.width, 3840U);
    EXPECT_EQ(result.height, 2160U);
}

} // namespace
} // namespace UVE::Window
