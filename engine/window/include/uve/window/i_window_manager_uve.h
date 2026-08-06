// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "uve/window/monitor_info_uve.h"

namespace UVE::Window {

/// IWindowManagerUVE owns exactly one OS window and — for the real GLFW3 backend — the entire
/// lifecycle of the OpenGL context tied to it: initialization of the windowing library,
/// termination, context creation, context destruction, and context activation
/// (`glfwMakeContextCurrent`) all happen inside the concrete implementation, never in a render
/// device. A render device (e.g. Render::GlRenderDeviceUVE) is constructed *after* an
/// IWindowManagerUVE& and assumes a current context already exists — it never creates, destroys,
/// or activates a context itself, and never creates or destroys the window.
/// Never exposes a backend-specific type: GetNativeWindowHandleUVE() is intentionally type-erased
/// (matching this codebase's established void* type-erasure precedent — ChunkUVE,
/// IAssetManagerUVE::RegisterLoaderUVE<T>) so a future backend (SDL3, a mobile-native window) can
/// satisfy this same interface without it ever naming GLFW.
/// Thread-safety: not thread-safe. Every method here must be called only from EngineCoreUVE's
/// single main-loop thread — this mirrors GLFW's own single-thread-per-window contract.
class IWindowManagerUVE {
public:
    virtual ~IWindowManagerUVE() = default;

    /// True iff the window (and, for the real backend, its GL context) was created successfully.
    /// False means every other method is still safe to call (no crashes) but reports a degraded,
    /// inert state — callers must not attempt to build a render device against an invalid window.
    [[nodiscard]] virtual bool IsValidUVE() const noexcept = 0;

    /// Pumps OS window/input events (GLFW: glfwPollEvents()). Publishes
    /// WindowCloseRequestedEventUVE/WindowResizedEventUVE/WindowFocusChangedEventUVE for anything
    /// that changed since the last call. Intended to be called once per frame.
    virtual void PollEventsUVE() = 0;

    /// Presents the window's back buffer (GLFW: glfwSwapBuffers()). Only meaningful for the real
    /// backend; a headless implementation no-ops.
    virtual void SwapBuffersUVE() = 0;

    /// True iff the user has requested the window be closed (e.g. clicked the OS close button)
    /// since the last ResetCloseRequestedUVE() call, if any. EngineCoreUVE polls this once per
    /// frame to drive its own RequestQuitUVE().
    [[nodiscard]] virtual bool IsCloseRequestedUVE() const noexcept = 0;

    /// Enables or disables vertical sync (GLFW: glfwSwapInterval(1 or 0)).
    virtual void SetVSyncEnabledUVE(bool enabled) = 0;
    [[nodiscard]] virtual bool IsVSyncEnabledUVE() const noexcept = 0;

    /// Toggles the window between windowed and borderless-fullscreen-on-primary-monitor.
    virtual void SetFullscreenUVE(bool fullscreen) = 0;
    [[nodiscard]] virtual bool IsFullscreenUVE() const noexcept = 0;

    /// The window's current framebuffer size in pixels. Always current — never cached stale
    /// across a resize, since GlRenderDeviceUVE polls this every frame to set the GL viewport
    /// rather than reacting inside a resize callback.
    [[nodiscard]] virtual std::uint32_t GetWidthUVE() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetHeightUVE() const noexcept = 0;

    /// Every currently connected monitor, in backend-defined order. A snapshot, not a live
    /// handle — re-enumerate to observe hot-plug changes. Empty for a headless implementation.
    [[nodiscard]] virtual std::vector<MonitorInfoUVE> EnumerateMonitorsUVE() const = 0;

    /// A type-erased native window handle (GLFW: the GLFWwindow* reinterpret_cast to void*) —
    /// the one and only bridge a render device needs to attach its context calls to this window.
    /// nullptr for a headless implementation, or when IsValidUVE() is false.
    [[nodiscard]] virtual void* GetNativeWindowHandleUVE() const noexcept = 0;

    /// A short human-readable backend name (e.g. "GLFW3", "Null"), for logging/diagnostics.
    [[nodiscard]] virtual std::string_view GetBackendNameUVE() const noexcept = 0;
};

} // namespace UVE::Window
