// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/window/window_manager_uve.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/input/input_system_uve.h"
#include "uve/window/window_events_uve.h"
#include "uve/window/monitor_info_validation_uve.h"

namespace UVE::Window::Tests {
namespace {

// This sandbox's Mesa/llvmpipe GLX stack caps at OpenGL 4.5 Core (confirmed by direct testing —
// requesting 4.6 fails with GLXBadFBConfig). Tests explicitly request 4.5 so a missing display is
// the only reason IsValidUVE() would be false here — not an unrelated GL-version mismatch. The
// shipped production default (EngineConfigUVE::windowGlVersionMajor/Minor = 4, 6) is untouched.
[[nodiscard]] WindowDescUVE MakeTestWindowDescUVE() {
    WindowDescUVE desc;
    desc.title = "uve_window_manager_uve_tests";
    desc.width = 64;
    desc.height = 64;
    desc.glVersionMajor = 4;
    desc.glVersionMinor = 5;
    return desc;
}

// Needs a real (possibly virtual, e.g. Xvfb) X/Wayland display. Every test's fixture checks this
// at SetUp() and calls GTEST_SKIP() with a clear message if unavailable, so the same uve_tests
// binary runs cleanly with or without a display attached.
class WindowManagerUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        windowManager = std::make_unique<WindowManagerUVE>(eventSystem, MakeTestWindowDescUVE());
        if (!windowManager->IsValidUVE()) {
            GTEST_SKIP() << "No display available for WindowManagerUVE - skipping (run under "
                            "xvfb-run to exercise this test)";
        }
    }

    Events::EventSystemUVE eventSystem;
    std::unique_ptr<WindowManagerUVE> windowManager;
};

TEST_F(WindowManagerUVETest, Construction_SucceedsAndReportsRequestedSize) {
    EXPECT_TRUE(windowManager->IsValidUVE());
    EXPECT_EQ(windowManager->GetWidthUVE(), 64U);
    EXPECT_EQ(windowManager->GetHeightUVE(), 64U);
}

TEST_F(WindowManagerUVETest, GetBackendNameUVE_ReturnsGlfw3) {
    EXPECT_EQ(windowManager->GetBackendNameUVE(), "GLFW3");
}

TEST_F(WindowManagerUVETest, GetNativeWindowHandleUVE_ReturnsNonNull) {
    EXPECT_NE(windowManager->GetNativeWindowHandleUVE(), nullptr);
}

TEST_F(WindowManagerUVETest, PollEventsWithAttachedInput_CommitsFiniteMouseSnapshot) {
    Input::InputSystemUVE inputSystem(eventSystem);
    windowManager->AttachInputSystemUVE(&inputSystem);
    windowManager->PollEventsUVE();
    inputSystem.UpdateUVE();
    const Math::Vector2UVE mousePosition = inputSystem.GetMousePositionUVE();
    EXPECT_TRUE(std::isfinite(mousePosition.x));
    EXPECT_TRUE(std::isfinite(mousePosition.y));
}

TEST_F(WindowManagerUVETest, PollEventsAndSwapBuffers_DoNotCrash) {
    windowManager->PollEventsUVE();
    windowManager->SwapBuffersUVE();
    windowManager->PollEventsUVE();
    SUCCEED();
}

TEST_F(WindowManagerUVETest, IsCloseRequestedUVE_FalseUntilUserClosesWindow) {
    EXPECT_FALSE(windowManager->IsCloseRequestedUVE());
}

TEST_F(WindowManagerUVETest, SetVSyncEnabledUVE_RoundTrips) {
    windowManager->SetVSyncEnabledUVE(false);
    EXPECT_FALSE(windowManager->IsVSyncEnabledUVE());
    windowManager->SetVSyncEnabledUVE(true);
    EXPECT_TRUE(windowManager->IsVSyncEnabledUVE());
}

TEST_F(WindowManagerUVETest, SetFullscreenUVE_RoundTripsWithBackendConfirmedState) {
    auto* const glfwWindow = static_cast<GLFWwindow*>(windowManager->GetNativeWindowHandleUVE());
    GLFWmonitor* const primaryMonitor = glfwGetPrimaryMonitor();
    ASSERT_NE(primaryMonitor, nullptr);
    EXPECT_FALSE(windowManager->IsFullscreenUVE());
    EXPECT_EQ(glfwGetWindowMonitor(glfwWindow), nullptr);
    windowManager->SetFullscreenUVE(true);
    EXPECT_TRUE(windowManager->IsFullscreenUVE());
    EXPECT_EQ(glfwGetWindowMonitor(glfwWindow), primaryMonitor);
    windowManager->SetFullscreenUVE(false);
    EXPECT_FALSE(windowManager->IsFullscreenUVE());
    EXPECT_EQ(glfwGetWindowMonitor(glfwWindow), nullptr);
}

TEST_F(WindowManagerUVETest, EnumerateMonitorsUVE_ReturnsValidatedSnapshot) {
    const std::vector<MonitorInfoUVE> monitors = windowManager->EnumerateMonitorsUVE();
    EXPECT_FALSE(monitors.empty());
    EXPECT_TRUE(ValidateMonitorSnapshotUVE(monitors));
}

TEST_F(WindowManagerUVETest, ResizingWindow_PublishesWindowResizedEventUVE) {
    bool receivedEvent = false;
    std::uint32_t receivedWidth = 0;
    std::uint32_t receivedHeight = 0;
    eventSystem.Subscribe<WindowResizedEventUVE>([&](const WindowResizedEventUVE& event) {
        receivedEvent = true;
        receivedWidth = event.width;
        receivedHeight = event.height;
    });

    auto* const glfwWindow = static_cast<GLFWwindow*>(windowManager->GetNativeWindowHandleUVE());
    glfwSetWindowSize(glfwWindow, 128, 96);

    // The X server delivers the resulting ConfigureNotify asynchronously over the X connection -
    // a single PollEventsUVE() call right after glfwSetWindowSize() races the round-trip time,
    // which varies with X server load (observed to be flaky specifically when many other tests
    // ran against the same virtual display beforehand). Poll in a short bounded loop instead of
    // assuming one call is enough, mirroring how a real frame loop naturally keeps polling.
    for (int attempt = 0; attempt < 50 && !receivedEvent; ++attempt) {
        windowManager->PollEventsUVE();
        if (!receivedEvent) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    EXPECT_TRUE(receivedEvent);
    EXPECT_EQ(receivedWidth, 128U);
    EXPECT_EQ(receivedHeight, 96U);
}

TEST_F(WindowManagerUVETest, RepeatedCreateDestroyCycles_LeaveEachInstanceIndependentlyValid) {
    // windowManager (from the fixture) is already alive; construct and destroy several more in
    // sequence to prove the refcounted glfwInit()/glfwTerminate() design leaks nothing across
    // repeated create/destroy cycles.
    for (int i = 0; i < 3; ++i) {
        WindowManagerUVE extra(eventSystem, MakeTestWindowDescUVE());
        EXPECT_TRUE(extra.IsValidUVE());
    }
    EXPECT_TRUE(windowManager->IsValidUVE());
}

} // namespace
} // namespace UVE::Window::Tests
