// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/display_mode_validation_uve.h"
#include <gtest/gtest.h>
namespace UVE::Window::Tests {
namespace {
TEST(DisplayModeValidationUVETest, AcceptsDefaultAndBoundedModes) {
    EXPECT_TRUE(ValidateDisplayModeDescUVE(DisplayModeDescUVE{}));
    EXPECT_TRUE(ValidateDisplayModeDescUVE({3840U, 2160U, 144U}));
    EXPECT_TRUE(ValidateDisplayModeDescUVE({kMaximumDisplayModeAxisUVE, kMaximumDisplayModeAxisUVE,
                                            kMaximumDisplayModeRefreshRateUVE}));
}
TEST(DisplayModeValidationUVETest, AcceptsZeroRefreshAsBackendAutoMode) {
    EXPECT_TRUE(ValidateDisplayModeDescUVE({1280U, 720U, 0U}));
}
TEST(DisplayModeValidationUVETest, RejectsZeroOrOversizedDimensions) {
    EXPECT_FALSE(ValidateDisplayModeDescUVE({0U, 1080U, 60U}));
    EXPECT_FALSE(ValidateDisplayModeDescUVE({1920U, 0U, 60U}));
    EXPECT_FALSE(ValidateDisplayModeDescUVE({kMaximumDisplayModeAxisUVE + 1U, 1080U, 60U}));
    EXPECT_FALSE(ValidateDisplayModeDescUVE({1920U, kMaximumDisplayModeAxisUVE + 1U, 60U}));
}
TEST(DisplayModeValidationUVETest, RejectsExcessiveRefreshRate) {
    EXPECT_FALSE(ValidateDisplayModeDescUVE({1920U, 1080U, kMaximumDisplayModeRefreshRateUVE + 1U}));
}
} // namespace
} // namespace UVE::Window::Tests
