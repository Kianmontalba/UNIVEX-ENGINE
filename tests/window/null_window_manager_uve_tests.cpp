//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/window/null_window_manager_uve.h"

#include <gtest/gtest.h>

namespace UVE::Window::Tests {
namespace {

TEST(NullWindowManagerUVETest, IsValidUVE_AlwaysTrue) {
    NullWindowManagerUVE windowManager;
    EXPECT_TRUE(windowManager.IsValidUVE());
}

TEST(NullWindowManagerUVETest, Construction_BookkeepsWidthHeightVSyncFromDesc) {
    WindowDescUVE desc;
    desc.width = 1920;
    desc.height = 1080;
    desc.vsyncEnabled = false;
    NullWindowManagerUVE windowManager(desc);

    EXPECT_EQ(windowManager.GetWidthUVE(), 1920U);
    EXPECT_EQ(windowManager.GetHeightUVE(), 1080U);
    EXPECT_FALSE(windowManager.IsVSyncEnabledUVE());
}

TEST(NullWindowManagerUVETest, DefaultDesc_MatchesWindowDescUVEDefaults) {
    NullWindowManagerUVE windowManager;
    EXPECT_EQ(windowManager.GetWidthUVE(), WindowDescUVE{}.width);
    EXPECT_EQ(windowManager.GetHeightUVE(), WindowDescUVE{}.height);
    EXPECT_TRUE(windowManager.IsVSyncEnabledUVE());
}

TEST(NullWindowManagerUVETest, PollEventsAndSwapBuffers_NeverCrash) {
    NullWindowManagerUVE windowManager;
    windowManager.PollEventsUVE();
    windowManager.SwapBuffersUVE();
    windowManager.PollEventsUVE();
    SUCCEED();
}

TEST(NullWindowManagerUVETest, IsCloseRequestedUVE_AlwaysFalse) {
    NullWindowManagerUVE windowManager;
    EXPECT_FALSE(windowManager.IsCloseRequestedUVE());
    windowManager.PollEventsUVE();
    EXPECT_FALSE(windowManager.IsCloseRequestedUVE());
}

TEST(NullWindowManagerUVETest, SetVSyncEnabledUVE_RoundTrips) {
    NullWindowManagerUVE windowManager;
    windowManager.SetVSyncEnabledUVE(false);
    EXPECT_FALSE(windowManager.IsVSyncEnabledUVE());
    windowManager.SetVSyncEnabledUVE(true);
    EXPECT_TRUE(windowManager.IsVSyncEnabledUVE());
}

TEST(NullWindowManagerUVETest, SetFullscreenUVE_RoundTrips) {
    NullWindowManagerUVE windowManager;
    EXPECT_FALSE(windowManager.IsFullscreenUVE());
    windowManager.SetFullscreenUVE(true);
    EXPECT_TRUE(windowManager.IsFullscreenUVE());
    windowManager.SetFullscreenUVE(false);
    EXPECT_FALSE(windowManager.IsFullscreenUVE());
}

TEST(NullWindowManagerUVETest, EnumerateMonitorsUVE_ReturnsEmpty) {
    NullWindowManagerUVE windowManager;
    EXPECT_TRUE(windowManager.EnumerateMonitorsUVE().empty());
}

TEST(NullWindowManagerUVETest, GetNativeWindowHandleUVE_ReturnsNullptr) {
    NullWindowManagerUVE windowManager;
    EXPECT_EQ(windowManager.GetNativeWindowHandleUVE(), nullptr);
}

TEST(NullWindowManagerUVETest, GetBackendNameUVE_ReturnsNull) {
    NullWindowManagerUVE windowManager;
    EXPECT_EQ(windowManager.GetBackendNameUVE(), "Null");
}

} // namespace
} // namespace UVE::Window::Tests
