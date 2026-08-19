#include "uve/window/monitor_info_validation_uve.h"

namespace UVE::Window {

bool ValidateMonitorInfoUVE(const MonitorInfoUVE& monitor) noexcept {
    return !monitor.name.empty() && monitor.name.size() <= 256U && monitor.width > 0U &&
           monitor.height > 0U;
}

bool ValidateMonitorSnapshotUVE(const std::vector<MonitorInfoUVE>& monitors) noexcept {
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
