// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/framebuffer_size_validation_uve.h"

#include <limits>

#include <gtest/gtest.h>

namespace UVE::Window::Tests {
namespace {

TEST(FramebufferSizeValidationUVETest, AcceptsPositiveSignedDimensions) {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    EXPECT_TRUE(ValidateFramebufferSizeUVE(1920, 1080, width, height));
    EXPECT_EQ(width, 1920U);
    EXPECT_EQ(height, 1080U);
}

TEST(FramebufferSizeValidationUVETest, RejectsNonPositiveDimensionsAtomically) {
    constexpr std::uint32_t kOriginalWidth = 640U;
    constexpr std::uint32_t kOriginalHeight = 480U;
    std::uint32_t width = kOriginalWidth;
    std::uint32_t height = kOriginalHeight;

    EXPECT_FALSE(ValidateFramebufferSizeUVE(0, 480, width, height));
    EXPECT_EQ(width, kOriginalWidth);
    EXPECT_EQ(height, kOriginalHeight);

    EXPECT_FALSE(ValidateFramebufferSizeUVE(640, -1, width, height));
    EXPECT_EQ(width, kOriginalWidth);
    EXPECT_EQ(height, kOriginalHeight);

    EXPECT_FALSE(ValidateFramebufferSizeUVE(-1, 480, width, height));
    EXPECT_EQ(width, kOriginalWidth);
    EXPECT_EQ(height, kOriginalHeight);
}

TEST(FramebufferSizeValidationUVETest, ConvertsMaximumSignedDimensionsWithoutOverflow) {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    EXPECT_TRUE(ValidateFramebufferSizeUVE(std::numeric_limits<int>::max(),
                                           std::numeric_limits<int>::max(), width, height));
    EXPECT_EQ(width, static_cast<std::uint32_t>(std::numeric_limits<int>::max()));
    EXPECT_EQ(height, static_cast<std::uint32_t>(std::numeric_limits<int>::max()));
}

} // namespace
} // namespace UVE::Window::Tests
