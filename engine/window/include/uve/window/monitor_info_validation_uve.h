#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "uve/window/monitor_info_uve.h"

namespace UVE::Window {

inline constexpr std::size_t kMaximumMonitorSnapshotEntriesUVE = 32U;

/// Validates signed backend monitor dimensions before conversion into the copied unsigned snapshot DTO.
[[nodiscard]] bool ValidateMonitorDimensionsUVE(int width, int height, std::uint32_t& outWidth,
                                               std::uint32_t& outHeight) noexcept;

/// Validates copied monitor snapshots without owning display handles, hot-plug state, or backend choice.
[[nodiscard]] bool ValidateMonitorInfoUVE(const MonitorInfoUVE& monitor) noexcept;
[[nodiscard]] bool ValidateMonitorSnapshotUVE(const std::vector<MonitorInfoUVE>& monitors) noexcept;

} // namespace UVE::Window
