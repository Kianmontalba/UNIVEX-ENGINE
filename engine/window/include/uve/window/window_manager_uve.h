// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/events/i_event_system_uve.h"
#include "uve/window/i_window_manager_uve.h"
#include "uve/window/window_desc_uve.h"

namespace UVE::Window {

/// WindowManagerUVE is the concrete, GLFW3-backed implementation of IWindowManagerUVE — the only
/// backend that ever actually calls glfwInit/glfwCreateWindow/glfwMakeContextCurrent/
/// glfwTerminate. Every GLFW type and header is confined to window_manager_uve.cpp (PIMPL); this
/// header never includes GLFW/glfw3.h, matching the codebase's established third-party-library
/// confinement discipline (ConfigManagerUVE's nlohmann::json PIMPL).
/// Owns the entire GL context lifecycle: glfwInit() (refcounted static — only the first live
/// WindowManagerUVE calls it, only the last one destroyed calls glfwTerminate()),
/// glfwCreateWindow(), and glfwMakeContextCurrent() all happen inside the constructor, in that
/// order. A render device built against this window (e.g. Render::GlRenderDeviceUVE) is
/// constructed strictly afterward and never creates, destroys, or activates the context itself.
class WindowManagerUVE final : public IWindowManagerUVE {
public:
    /// `eventSystem` must outlive this WindowManagerUVE. On any GLFW failure (glfwInit or
    /// glfwCreateWindow), logs UVE_ERROR/UVE_FATAL internally and leaves IsValidUVE() == false —
    /// never throws.
    WindowManagerUVE(Events::IEventSystemUVE& eventSystem, const WindowDescUVE& desc);
    ~WindowManagerUVE() override;

    WindowManagerUVE(const WindowManagerUVE&) = delete;
    WindowManagerUVE& operator=(const WindowManagerUVE&) = delete;

    [[nodiscard]] bool IsValidUVE() const noexcept override;
    void AttachInputSystemUVE(Input::IInputSystemUVE* inputSystem) noexcept override;
    void PollEventsUVE() override;
    void SwapBuffersUVE() override;
    [[nodiscard]] bool IsCloseRequestedUVE() const noexcept override;
    void SetVSyncEnabledUVE(bool enabled) override;
    [[nodiscard]] bool IsVSyncEnabledUVE() const noexcept override;
    void SetFullscreenUVE(bool fullscreen) override;
    [[nodiscard]] bool IsFullscreenUVE() const noexcept override;
    [[nodiscard]] std::uint32_t GetWidthUVE() const noexcept override;
    [[nodiscard]] std::uint32_t GetHeightUVE() const noexcept override;
    [[nodiscard]] std::vector<MonitorInfoUVE> EnumerateMonitorsUVE() const override;
    [[nodiscard]] void* GetNativeWindowHandleUVE() const noexcept override;
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Window
