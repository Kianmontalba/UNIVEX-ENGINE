#pragma once

#include <cstddef>
#include <vector>

#include "uve/window/monitor_info_uve.h"

namespace UVE::Window {

inline constexpr std::size_t kMaximumMonitorSnapshotEntriesUVE = 32U;

/// Validates copied monitor snapshots without owning display handles, hot-plug state, or backend choice.
[[nodiscard]] bool ValidateMonitorInfoUVE(const MonitorInfoUVE& monitor) noexcept;
[[nodiscard]] bool ValidateMonitorSnapshotUVE(const std::vector<MonitorInfoUVE>& monitors) noexcept;

} // namespace UVE::Window
