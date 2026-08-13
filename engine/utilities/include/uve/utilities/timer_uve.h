// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <chrono>

#include "uve/utilities/i_timer_uve.h"

namespace UVE::Utilities {

/// TimerUVE is the concrete, engine-standard implementation of ITimerUVE,
/// wrapping std::chrono::steady_clock. Delta time and total time are stored
/// internally as double: a float total would lose meaningful precision
/// over a long-running session, while a float delta is fine for per-frame
/// use (see GetDeltaTimeFloatUVE).
/// Thread-safety: not thread-safe. Owned and ticked exclusively by the
/// frame-pipeline thread (EngineCoreUVE's main loop); no internal
/// synchronization is provided.
class TimerUVE final : public ITimerUVE {
public:
    TimerUVE();

    void Tick() override;
    [[nodiscard]] double GetDeltaTimeUVE() const override;
    [[nodiscard]] float GetDeltaTimeFloatUVE() const override;
    [[nodiscard]] double GetTotalTimeUVE() const override;
    void Reset() override;
    void SetMaxDeltaTimeUVE(double maxDeltaSeconds) override;
    void SetFixedTimestepUVE(double fixedDeltaSeconds) override;
    FixedStepResultUVE AdvanceFixedStepUVE() override;
    void DiscardFixedStepAccumulatorUVE() noexcept override;

private:
    std::chrono::steady_clock::time_point m_lastTickTime;
    double m_deltaTime = 0.0;
    double m_totalTime = 0.0;
    double m_maxDeltaTime = 0.25;

    double m_fixedDeltaTime = 1.0 / 60.0;
    double m_accumulator = 0.0;

    /// Hard cap on stepsToRun returned by a single AdvanceFixedStepUVE()
    /// call. A second, independent spiral-of-death guard beyond the
    /// per-Tick() delta clamp: even with a clamped delta, a sustained run
    /// of low frame rates could otherwise let the accumulator grow forever
    /// if steps consistently take longer to run than they simulate. Not
    /// currently exposed via ITimerUVE — revisit if a future system needs
    /// it configurable.
    static constexpr int kMaxStepsPerTick = 8;
};

} // namespace UVE::Utilities
