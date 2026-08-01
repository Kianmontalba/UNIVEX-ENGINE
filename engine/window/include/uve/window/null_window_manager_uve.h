//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/window/i_window_manager_uve.h"
#include "uve/window/window_desc_uve.h"

namespace UVE::Window {

/// NullWindowManagerUVE is the headless IWindowManagerUVE backend: it never touches GLFW, X11, or
/// any OS window/GL API — it bookkeeps a width/height/vsync/fullscreen state matching what a real
/// window with the given WindowDescUVE would report, so code that queries an IWindowManagerUVE&
/// behaves consistently whether or not a real window exists. Used whenever
/// EngineConfigUVE::headlessUVE is true, so EngineServicesUVE's "every service is a live
/// reference" contract needs no headless-mode exception — mirrors NullRenderDeviceUVE/
/// NullAudioDeviceUVE's exact role for their own interfaces.
/// Thread-safety: not thread-safe, matching IWindowManagerUVE's own documented contract.
class NullWindowManagerUVE final : public IWindowManagerUVE {
public:
    explicit NullWindowManagerUVE(const WindowDescUVE& desc = WindowDescUVE{});

    [[nodiscard]] bool IsValidUVE() const noexcept override;
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
    std::uint32_t m_width;
    std::uint32_t m_height;
    bool m_vsyncEnabled;
    bool m_fullscreen = false;
};

} // namespace UVE::Window
