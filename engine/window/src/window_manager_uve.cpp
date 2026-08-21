// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/window/window_manager_uve.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "uve/debug/logging_macros_uve.h"
#include "uve/input/key_code_uve.h"
#include "uve/input/mouse_button_uve.h"
#include "uve/window/window_events_uve.h"
#include "uve/window/monitor_info_validation_uve.h"
#include "uve/window/window_desc_validation_uve.h"

namespace UVE::Window {

namespace {

/// Process-wide GLFW init refcount: only the first live WindowManagerUVE calls glfwInit(), only
/// the last one destroyed calls glfwTerminate(). Correct even though this codebase only ever
/// constructs one WindowManagerUVE at a time today — matches how a shared library-level resource
/// should be initialized exactly once regardless of how many owners reference it.
int g_glfwRefCount = 0;

void GlfwErrorCallbackUVE(int errorCode, const char* description) {
    UVE_ERROR("WindowManagerUVE: GLFW error {}: {}", errorCode, description != nullptr ? description : "(no description)");
}

} // namespace

struct WindowManagerUVE::ImplUVE {
    Events::IEventSystemUVE* eventSystem;
    GLFWwindow* window = nullptr;
    bool valid = false;
    // True iff this instance's constructor incremented g_glfwRefCount and has not yet undone
    // that increment — the destructor's only signal for whether it owes a decrement, since a
    // glfwInit() failure (never incremented) and a glfwCreateWindow() failure (incremented, then
    // already decremented inline) must not be double-counted.
    bool ownsGlfwRefCount = false;
    bool vsyncEnabled = true;
    bool fullscreen = false;
    Input::IInputSystemUVE* inputSystem = nullptr;
    float scrollDelta = 0.0F;
    int windowedX = 0;
    int windowedY = 0;
    int windowedWidth = 0;
    int windowedHeight = 0;

    explicit ImplUVE(Events::IEventSystemUVE& eventSystemIn) : eventSystem(&eventSystemIn) {}

    static ImplUVE& FromWindowUVE(GLFWwindow* glfwWindow) {
        return *static_cast<ImplUVE*>(glfwGetWindowUserPointer(glfwWindow));
    }

    static void CloseCallbackUVE(GLFWwindow* glfwWindow) {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
        FromWindowUVE(glfwWindow).eventSystem->Publish(WindowCloseRequestedEventUVE{});
    }

    static void FramebufferSizeCallbackUVE(GLFWwindow* glfwWindow, int width, int height) {
        // Publishes only — never touches GL state here. GlRenderDeviceUVE polls the window's
        // current size once per frame instead, per the approved design.
        FromWindowUVE(glfwWindow).eventSystem->Publish(WindowResizedEventUVE{
            static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)});
    }

    static void FocusCallbackUVE(GLFWwindow* glfwWindow, int focused) {
        FromWindowUVE(glfwWindow).eventSystem->Publish(WindowFocusChangedEventUVE{focused == GLFW_TRUE});
    }

    static void ScrollCallbackUVE(GLFWwindow* glfwWindow, double /*xOffset*/, double yOffset) {
        FromWindowUVE(glfwWindow).scrollDelta += static_cast<float>(yOffset);
    }

    static int ToGlfwKeyUVE(const Input::KeyCodeUVE key) noexcept {
        using Input::KeyCodeUVE;
        const int value = static_cast<int>(key);
        if (value >= static_cast<int>(KeyCodeUVE::A) && value <= static_cast<int>(KeyCodeUVE::Z)) {
            return GLFW_KEY_A + value - static_cast<int>(KeyCodeUVE::A);
        }
        if (value >= static_cast<int>(KeyCodeUVE::Num0) && value <= static_cast<int>(KeyCodeUVE::Num9)) {
            return GLFW_KEY_0 + value - static_cast<int>(KeyCodeUVE::Num0);
        }
        if (value >= static_cast<int>(KeyCodeUVE::F1) && value <= static_cast<int>(KeyCodeUVE::F12)) {
            return GLFW_KEY_F1 + value - static_cast<int>(KeyCodeUVE::F1);
        }
        switch (key) {
        case KeyCodeUVE::Up: return GLFW_KEY_UP;
        case KeyCodeUVE::Down: return GLFW_KEY_DOWN;
        case KeyCodeUVE::Left: return GLFW_KEY_LEFT;
        case KeyCodeUVE::Right: return GLFW_KEY_RIGHT;
        case KeyCodeUVE::Space: return GLFW_KEY_SPACE;
        case KeyCodeUVE::Enter: return GLFW_KEY_ENTER;
        case KeyCodeUVE::Escape: return GLFW_KEY_ESCAPE;
        case KeyCodeUVE::Tab: return GLFW_KEY_TAB;
        case KeyCodeUVE::Backspace: return GLFW_KEY_BACKSPACE;
        case KeyCodeUVE::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyCodeUVE::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyCodeUVE::LeftCtrl: return GLFW_KEY_LEFT_CONTROL;
        case KeyCodeUVE::RightCtrl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyCodeUVE::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyCodeUVE::RightAlt: return GLFW_KEY_RIGHT_ALT;
        default: return GLFW_KEY_UNKNOWN;
        }
    }

    static int ToGlfwMouseButtonUVE(const Input::MouseButtonUVE button) noexcept {
        switch (button) {
        case Input::MouseButtonUVE::Left: return GLFW_MOUSE_BUTTON_LEFT;
        case Input::MouseButtonUVE::Right: return GLFW_MOUSE_BUTTON_RIGHT;
        case Input::MouseButtonUVE::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
        default: return -1;
        }
    }

    void PollInputUVE() noexcept {
        if (inputSystem == nullptr || window == nullptr) {
            return;
        }
        for (int value = static_cast<int>(Input::KeyCodeUVE::A);
             value < static_cast<int>(Input::KeyCodeUVE::Count); ++value) {
            const auto key = static_cast<Input::KeyCodeUVE>(value);
            const int glfwKey = ToGlfwKeyUVE(key);
            if (glfwKey != GLFW_KEY_UNKNOWN) {
                inputSystem->SetKeyStateUVE(key, glfwGetKey(window, glfwKey) != GLFW_RELEASE);
            }
        }
        for (int value = 0; value < static_cast<int>(Input::MouseButtonUVE::Count); ++value) {
            const auto button = static_cast<Input::MouseButtonUVE>(value);
            const int glfwButton = ToGlfwMouseButtonUVE(button);
            inputSystem->SetMouseButtonStateUVE(
                button, glfwGetMouseButton(window, glfwButton) != GLFW_RELEASE);
        }
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(window, &cursorX, &cursorY);
        inputSystem->SetMousePositionUVE(
            Math::Vector2UVE{static_cast<float>(cursorX), static_cast<float>(cursorY)});
        inputSystem->SetMouseScrollDeltaUVE(scrollDelta);
        scrollDelta = 0.0F;
    }
};

WindowManagerUVE::WindowManagerUVE(Events::IEventSystemUVE& eventSystem, const WindowDescUVE& desc)
    : m_impl(std::make_unique<ImplUVE>(eventSystem)) {
    if (!ValidateWindowDescUVE(desc)) {
        UVE_ERROR("WindowManagerUVE: rejected invalid window descriptor before GLFW initialization");
        return;
    }
    if (g_glfwRefCount == 0) {
        glfwSetErrorCallback(&GlfwErrorCallbackUVE);
        if (glfwInit() != GLFW_TRUE) {
            UVE_FATAL("WindowManagerUVE: glfwInit() failed");
            return;
        }
    }
    ++g_glfwRefCount;
    m_impl->ownsGlfwRefCount = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, static_cast<int>(desc.glVersionMajor));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, static_cast<int>(desc.glVersionMinor));
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // GLFW_OPENGL_FORWARD_COMPAT is deliberately NOT set: it's only meaningful on macOS (required
    // there to get a non-deprecated 3.2+ core context), which this increment doesn't target
    // ("PC only for now (Windows + Linux)"). Confirmed by direct testing that setting it TRUE
    // breaks GLFW's X11 framebuffer-resize callback delivery on this sandbox's Mesa/GLX stack
    // (the resize itself still applies — glfwGetFramebufferSize reports the new size — but
    // glfwSetFramebufferSizeCallback's registered callback silently never fires), so omitting it
    // is a genuine fix, not a workaround, for the platforms this increment actually supports.
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_impl->window = glfwCreateWindow(static_cast<int>(desc.width), static_cast<int>(desc.height),
                                       desc.title.c_str(), nullptr, nullptr);
    if (m_impl->window == nullptr) {
        UVE_FATAL("WindowManagerUVE: glfwCreateWindow() failed requesting OpenGL {}.{} Core Profile",
                   desc.glVersionMajor, desc.glVersionMinor);
        --g_glfwRefCount;
        m_impl->ownsGlfwRefCount = false;
        if (g_glfwRefCount == 0) {
            glfwTerminate();
        }
        return;
    }

    glfwSetWindowUserPointer(m_impl->window, m_impl.get());
    glfwSetWindowCloseCallback(m_impl->window, &ImplUVE::CloseCallbackUVE);
    glfwSetFramebufferSizeCallback(m_impl->window, &ImplUVE::FramebufferSizeCallbackUVE);
    glfwSetWindowFocusCallback(m_impl->window, &ImplUVE::FocusCallbackUVE);
    glfwSetScrollCallback(m_impl->window, &ImplUVE::ScrollCallbackUVE);

    glfwMakeContextCurrent(m_impl->window);
    m_impl->vsyncEnabled = desc.vsyncEnabled;
    glfwSwapInterval(desc.vsyncEnabled ? 1 : 0);

    m_impl->valid = true;
    UVE_INFO("WindowManagerUVE: created {}x{} window \"{}\", requested OpenGL {}.{} Core", desc.width,
              desc.height, desc.title, desc.glVersionMajor, desc.glVersionMinor);
}

WindowManagerUVE::~WindowManagerUVE() {
    if (m_impl->window != nullptr) {
        glfwDestroyWindow(m_impl->window);
    }
    if (m_impl->ownsGlfwRefCount) {
        --g_glfwRefCount;
        if (g_glfwRefCount == 0) {
            glfwTerminate();
        }
    }
}

bool WindowManagerUVE::IsValidUVE() const noexcept {
    return m_impl->valid;
}

void WindowManagerUVE::AttachInputSystemUVE(Input::IInputSystemUVE* inputSystem) noexcept {
    m_impl->inputSystem = inputSystem;
}

void WindowManagerUVE::PollEventsUVE() {
    if (m_impl->valid) {
        glfwPollEvents();
        m_impl->PollInputUVE();
    }
}

void WindowManagerUVE::SwapBuffersUVE() {
    if (m_impl->valid) {
        glfwSwapBuffers(m_impl->window);
    }
}

bool WindowManagerUVE::IsCloseRequestedUVE() const noexcept {
    return m_impl->valid && glfwWindowShouldClose(m_impl->window) == GLFW_TRUE;
}

void WindowManagerUVE::SetVSyncEnabledUVE(bool enabled) {
    m_impl->vsyncEnabled = enabled;
    if (m_impl->valid) {
        glfwSwapInterval(enabled ? 1 : 0);
    }
}

bool WindowManagerUVE::IsVSyncEnabledUVE() const noexcept {
    return m_impl->vsyncEnabled;
}

void WindowManagerUVE::SetFullscreenUVE(bool fullscreen) {
    if (!m_impl->valid || fullscreen == m_impl->fullscreen) {
        return;
    }

    if (fullscreen) {
        glfwGetWindowPos(m_impl->window, &m_impl->windowedX, &m_impl->windowedY);
        glfwGetWindowSize(m_impl->window, &m_impl->windowedWidth, &m_impl->windowedHeight);

        GLFWmonitor* const primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* const mode = primaryMonitor != nullptr ? glfwGetVideoMode(primaryMonitor) : nullptr;
        if (primaryMonitor == nullptr || mode == nullptr) {
            UVE_ERROR("WindowManagerUVE: SetFullscreenUVE(true) failed - no primary monitor/video mode available");
            return;
        }
        glfwSetWindowMonitor(m_impl->window, primaryMonitor, 0, 0, mode->width, mode->height,
                              mode->refreshRate);
        if (glfwGetWindowMonitor(m_impl->window) != primaryMonitor) {
            UVE_ERROR("WindowManagerUVE: GLFW did not confirm fullscreen transition");
            return;
        }
    } else {
        glfwSetWindowMonitor(m_impl->window, nullptr, m_impl->windowedX, m_impl->windowedY,
                              m_impl->windowedWidth, m_impl->windowedHeight, 0);
        if (glfwGetWindowMonitor(m_impl->window) != nullptr) {
            UVE_ERROR("WindowManagerUVE: GLFW did not confirm windowed transition");
            return;
        }
    }
    m_impl->fullscreen = fullscreen;
}

bool WindowManagerUVE::IsFullscreenUVE() const noexcept {
    return m_impl->fullscreen;
}

std::uint32_t WindowManagerUVE::GetWidthUVE() const noexcept {
    if (!m_impl->valid) {
        return 0;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_impl->window, &width, &height);
    return static_cast<std::uint32_t>(width);
}

std::uint32_t WindowManagerUVE::GetHeightUVE() const noexcept {
    if (!m_impl->valid) {
        return 0;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_impl->window, &width, &height);
    return static_cast<std::uint32_t>(height);
}

std::vector<MonitorInfoUVE> WindowManagerUVE::EnumerateMonitorsUVE() const {
    std::vector<MonitorInfoUVE> monitors;
    int monitorCount = 0;
    GLFWmonitor** const glfwMonitors = glfwGetMonitors(&monitorCount);
    if (monitorCount < 0 || (monitorCount > 0 && glfwMonitors == nullptr)) {
        return {};
    }
    if (monitorCount > static_cast<int>(kMaximumMonitorSnapshotEntriesUVE)) {
        UVE_ERROR("WindowManagerUVE: GLFW returned {} monitors, exceeding the {}-entry snapshot cap",
                  monitorCount, kMaximumMonitorSnapshotEntriesUVE);
        return {};
    }
    GLFWmonitor* const primaryMonitor = glfwGetPrimaryMonitor();

    monitors.reserve(static_cast<std::size_t>(monitorCount));
    for (int index = 0; index < monitorCount; ++index) {
        GLFWmonitor* const monitor = glfwMonitors[index];
        const GLFWvidmode* const mode = glfwGetVideoMode(monitor);
        if (mode == nullptr) {
            continue; // Skip a monitor GLFW couldn't query a mode for rather than crashing.
        }
        const char* const name = glfwGetMonitorName(monitor);
        monitors.push_back(MonitorInfoUVE{name != nullptr ? name : "", static_cast<std::uint32_t>(mode->width),
                                           static_cast<std::uint32_t>(mode->height), monitor == primaryMonitor});
    }
    if (!ValidateMonitorSnapshotUVE(monitors)) {
        UVE_ERROR("WindowManagerUVE: GLFW returned an invalid monitor snapshot");
        return {};
    }
    return monitors;
}

void* WindowManagerUVE::GetNativeWindowHandleUVE() const noexcept {
    return m_impl->valid ? static_cast<void*>(m_impl->window) : nullptr;
}

std::string_view WindowManagerUVE::GetBackendNameUVE() const noexcept {
    return "GLFW3";
}

} // namespace UVE::Window
