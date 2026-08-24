#include "uve/window/monitor_info_validation_uve.h"

namespace UVE::Window {

bool ValidateMonitorDimensionsUVE(const int width, const int height, std::uint32_t& outWidth,
                                  std::uint32_t& outHeight) noexcept {
    if (width <= 0 || height <= 0) {
        return false;
    }
    outWidth = static_cast<std::uint32_t>(width);
    outHeight = static_cast<std::uint32_t>(height);
    return true;
}

bool ValidateMonitorInfoUVE(const MonitorInfoUVE& monitor) noexcept {
    return !monitor.name.empty() && monitor.name.size() <= 256U &&
           monitor.name.find('\0') == std::string::npos && monitor.width > 0U && monitor.height > 0U;
}

bool ValidateMonitorSnapshotUVE(const std::vector<MonitorInfoUVE>& monitors) noexcept {
    if (monitors.size() > kMaximumMonitorSnapshotEntriesUVE) {
        return false;
    }
    bool primarySeen = false;
    for (const MonitorInfoUVE& monitor : monitors) {
        if (!ValidateMonitorInfoUVE(monitor)) {
            return false;
        }
        if (monitor.isPrimary && primarySeen) {
            return false;
        }
        primarySeen = primarySeen || monitor.isPrimary;
    }
    return true;
}

} // namespace UVE::Window
