// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/input/mobile_gesture_recognizer_uve.h"

namespace UVE::Input {

/// EngineServices boundary for bounded mobile gesture classification. The service borrows the
/// caller-owned mobile snapshot service, retains only the latest copied report and recognizer state,
/// and never owns platform callbacks, lifecycle, touch polling, or ECS state.
class IMobileGestureSystemUVE {
public:
    virtual ~IMobileGestureSystemUVE() = default;

    /// Consumes the mobile service's current copied snapshot once for the frame. Must run after
    /// IMobileInputSystemUVE::UpdateUVE() on the main thread.
    virtual void UpdateUVE(float frameDeltaSeconds) = 0;
    [[nodiscard]] virtual MobileGestureReportUVE GetLastReportUVE() const noexcept = 0;
    virtual void ResetUVE() noexcept = 0;
};

} // namespace UVE::Input
