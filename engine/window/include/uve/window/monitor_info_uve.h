//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace UVE::Window {

/// One entry returned by IWindowManagerUVE::EnumerateMonitorsUVE() — a snapshot, not a live
/// handle; re-enumerate to observe monitor hot-plug changes. Exists for future use (multi-monitor
/// window placement, fullscreen target selection); nothing in this increment consumes it beyond
/// exposing it.
struct MonitorInfoUVE {
    std::string name;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool isPrimary = false;
};

} // namespace UVE::Window
