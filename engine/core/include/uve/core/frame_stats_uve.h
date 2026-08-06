//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

#include <cstdint>

namespace UVE::Core {

/// Read-only snapshot of the current frame's timing/throughput statistics,
/// updated by EngineCoreUVE once per frame. Deliberately a plain data
/// struct so a future profiler system can consume/store it as-is.
/// Thread-safety: value type. EngineCoreUVE::GetFrameStatsUVE() returns a
/// const reference reflecting whichever frame most recently completed; no
/// synchronization is provided against a concurrently-running frame, since
/// the frame pipeline itself is single-threaded this increment.
struct FrameStatsUVE {
    /// Count of frames completed since the current RunUVE() call started
    /// (0 before the first BeginFrame()).
    std::uint64_t frameNumber = 0;

    /// Wall-clock seconds the most recently completed frame's full
    /// BeginFrame..EndFrame pipeline took to execute.
    double frameTimeSeconds = 0.0;

    /// UVE::Utilities::ITimerUVE::GetDeltaTimeUVE() as sampled during this
    /// frame's BeginFrame().
    double deltaTimeSeconds = 0.0;

    /// UVE::Utilities::ITimerUVE::GetTotalTimeUVE() as sampled during this
    /// frame's BeginFrame().
    double totalTimeSeconds = 0.0;

    /// Exponential moving average of 1/deltaTimeSeconds, recomputed each
    /// LateUpdate(). Remains 0 until the first frame with a non-zero delta
    /// has completed LateUpdate().
    double fps = 0.0;
};

} // namespace UVE::Core
