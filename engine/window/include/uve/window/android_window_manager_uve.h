#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "uve/window/i_window_manager_uve.h"
#include "uve/window/window_desc_uve.h"

namespace UVE::Events {
class IEventSystemUVE;
}

namespace UVE::Window {

/// Android NativeActivity window backend. The supplied native handle is a borrowed ANativeWindow*
/// owned by Android; this class owns only the EGL display, context, and window surface created from it.
/// All methods are main-thread-only, matching IWindowManagerUVE and Android's window contract.
class AndroidWindowManagerUVE final : public IWindowManagerUVE {
public:
    AndroidWindowManagerUVE(Events::IEventSystemUVE& eventSystem, void* nativeWindow,
                            const WindowDescUVE& description);
    ~AndroidWindowManagerUVE() override;

    AndroidWindowManagerUVE(const AndroidWindowManagerUVE&) = delete;
    AndroidWindowManagerUVE& operator=(const AndroidWindowManagerUVE&) = delete;

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
