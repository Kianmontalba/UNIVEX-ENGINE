// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/window/window_desc_validation_uve.h"
#include "uve/window/display_mode_validation_uve.h"
#include <gtest/gtest.h>
namespace UVE::Window::Tests {
namespace {
TEST(WindowDescValidationUVETest, DefaultDescriptor_IsValid) {
    EXPECT_TRUE(ValidateWindowDescUVE(WindowDescUVE{}));
}
TEST(WindowDescValidationUVETest, ValidCustomDescriptor_IsAccepted) {
    WindowDescUVE desc;
    desc.title = "Editor Preview";
    desc.width = 1920U;
    desc.height = 1080U;
    desc.glVersionMajor = 3U;
    desc.glVersionMinor = 3U;
    EXPECT_TRUE(ValidateWindowDescUVE(desc));
}
TEST(WindowDescValidationUVETest, ZeroDimensionsOrEmptyTitle_AreRejected) {
    WindowDescUVE desc;
    desc.width = 0U;
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
    desc.width = 1280U;
    desc.height = 0U;
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
    desc.height = 720U;
    desc.title.clear();
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
}
TEST(WindowDescValidationUVETest, AxisDimensionsAboveSharedCap_AreRejected) {
    WindowDescUVE desc;
    desc.width = kMaximumDisplayModeAxisUVE + 1U;
    EXPECT_FALSE(ValidateWindowDescUVE(desc));

    desc.width = 1280U;
    desc.height = kMaximumDisplayModeAxisUVE + 1U;
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
}
TEST(WindowDescValidationUVETest, TitleAtSharedCap_IsAccepted) {
    WindowDescUVE desc;
    desc.title.assign(kMaximumWindowTitleBytesUVE, 'T');
    EXPECT_TRUE(ValidateWindowDescUVE(desc));
}

TEST(WindowDescValidationUVETest, OversizedOrEmbeddedNulTitle_IsRejected) {
    WindowDescUVE desc;
    desc.title.assign(kMaximumWindowTitleBytesUVE + 1U, 'T');
    EXPECT_FALSE(ValidateWindowDescUVE(desc));

    desc.title = std::string{"UVE\0Editor", 10U};
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
}

TEST(WindowDescValidationUVETest, OpenGlMajorVersionBelowOne_IsRejected) {
    WindowDescUVE desc;
    desc.glVersionMajor = 0U;
    EXPECT_FALSE(ValidateWindowDescUVE(desc));
}
} // namespace
} // namespace UVE::Window::Tests
