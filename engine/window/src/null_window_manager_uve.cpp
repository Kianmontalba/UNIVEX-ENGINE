//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/window/null_window_manager_uve.h"

namespace UVE::Window {

NullWindowManagerUVE::NullWindowManagerUVE(const WindowDescUVE& desc)
    : m_width(desc.width), m_height(desc.height), m_vsyncEnabled(desc.vsyncEnabled) {}

bool NullWindowManagerUVE::IsValidUVE() const noexcept {
    return true;
}

void NullWindowManagerUVE::PollEventsUVE() {
    // No real OS event queue to pump.
}

void NullWindowManagerUVE::SwapBuffersUVE() {
    // No real back buffer to present.
}

bool NullWindowManagerUVE::IsCloseRequestedUVE() const noexcept {
    return false;
}

void NullWindowManagerUVE::SetVSyncEnabledUVE(bool enabled) {
    m_vsyncEnabled = enabled;
}

bool NullWindowManagerUVE::IsVSyncEnabledUVE() const noexcept {
    return m_vsyncEnabled;
}

void NullWindowManagerUVE::SetFullscreenUVE(bool fullscreen) {
    m_fullscreen = fullscreen;
}

bool NullWindowManagerUVE::IsFullscreenUVE() const noexcept {
    return m_fullscreen;
}

std::uint32_t NullWindowManagerUVE::GetWidthUVE() const noexcept {
    return m_width;
}

std::uint32_t NullWindowManagerUVE::GetHeightUVE() const noexcept {
    return m_height;
}

std::vector<MonitorInfoUVE> NullWindowManagerUVE::EnumerateMonitorsUVE() const {
    return {};
}

void* NullWindowManagerUVE::GetNativeWindowHandleUVE() const noexcept {
    return nullptr;
}

std::string_view NullWindowManagerUVE::GetBackendNameUVE() const noexcept {
    return "Null";
}

} // namespace UVE::Window
