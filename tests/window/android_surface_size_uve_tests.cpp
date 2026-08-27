// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/window/android_surface_size_uve.h"

#include <cstdint>

#include <gtest/gtest.h>

namespace UVE::Window::Tests {

TEST(AndroidSurfaceSizeUVETest, InvalidNativeSizesReturnZeroSize) {
    EXPECT_EQ(ClampAndroidSurfaceSizeUVE(0, 720).width, 0U);
    EXPECT_EQ(ClampAndroidSurfaceSizeUVE(1280, 0).height, 0U);
    EXPECT_EQ(ClampAndroidSurfaceSizeUVE(-1, 720).width, 0U);
    EXPECT_EQ(ClampAndroidSurfaceSizeUVE(1280, -1).height, 0U);
}

TEST(AndroidSurfaceSizeUVETest, NormalSurfaceSizeIsPreserved) {
    const AndroidSurfaceSizeUVE size = ClampAndroidSurfaceSizeUVE(1280, 720);
    EXPECT_EQ(size.width, 1280U);
    EXPECT_EQ(size.height, 720U);
}

TEST(AndroidSurfaceSizeUVETest, OversizedSurfaceStaysWithinPixelAndAxisBudgets) {
    const AndroidSurfaceSizeUVE size = ClampAndroidSurfaceSizeUVE(3840, 2160);
    ASSERT_GT(size.width, 0U);
    ASSERT_GT(size.height, 0U);
    EXPECT_LE(size.width, kMaximumAndroidSurfaceAxisUVE);
    EXPECT_LE(size.height, kMaximumAndroidSurfaceAxisUVE);
    EXPECT_LE(static_cast<std::uint64_t>(size.width) * size.height,
              kMaximumAndroidRenderTargetPixelsUVE);
}

TEST(AndroidSurfaceSizeUVETest, PortraitSurfacePreservesOrientation) {
    const AndroidSurfaceSizeUVE size = ClampAndroidSurfaceSizeUVE(1080, 1920);
    EXPECT_GT(size.height, size.width);
    EXPECT_LE(size.width, kMaximumAndroidSurfaceAxisUVE);
    EXPECT_LE(size.height, kMaximumAndroidSurfaceAxisUVE);
    EXPECT_LE(static_cast<std::uint64_t>(size.width) * size.height,
              kMaximumAndroidRenderTargetPixelsUVE);
}

TEST(AndroidSurfaceSizeUVETest, ExtremeAspectSurfaceStillProducesPositiveDimensions) {
    const AndroidSurfaceSizeUVE size = ClampAndroidSurfaceSizeUVE(100000, 1);
    EXPECT_EQ(size.width, kMaximumAndroidSurfaceAxisUVE);
    EXPECT_EQ(size.height, 1U);
}

} // namespace UVE::Window::Tests
