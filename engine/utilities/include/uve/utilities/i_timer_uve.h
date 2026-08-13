// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

namespace UVE::Utilities {

/// Result of one AdvanceFixedStepUVE() call: how many fixed-size simulation
/// steps the caller should run this frame, plus the leftover fractional
/// progress toward the next step (useful for interpolating render state
/// between the last two simulated steps).
struct FixedStepResultUVE {
    /// Number of whole fixed timesteps to run this frame. Capped by the
    /// timer implementation to avoid an unbounded catch-up loop after a
    /// long stall.
    int stepsToRun = 0;

    /// Fractional progress toward the next fixed step, in [0, 1).
    double alpha = 0.0;
};

/// ITimerUVE is the interface for the engine's frame-timing source. Exists
/// so future builds (dedicated server running at a fixed simulated rate,
/// unit tests needing a deterministic/injectable clock) can substitute a
/// different timer implementation without changing any call site that
/// consumes an ITimerUVE&.
/// Thread-safety: implementations are not required to be thread-safe;
/// intended to be owned and driven exclusively by the frame-pipeline
/// thread.
class ITimerUVE {
public:
    virtual ~ITimerUVE() = default;

    /// Samples the clock, updating delta time, total time, and the
    /// fixed-step accumulator. Must be called exactly once per frame,
    /// before any other method is queried for this frame's values.
    virtual void Tick() = 0;

    /// Seconds elapsed since the previous Tick(), clamped to the configured
    /// maximum (see SetMaxDeltaTimeUVE) to guard against a spiral of death
    /// after a debugger pause or long stall.
    [[nodiscard]] virtual double GetDeltaTimeUVE() const = 0;

    /// GetDeltaTimeUVE() narrowed to float, for call sites that only need
    /// per-frame precision (e.g. shader/physics parameters), where float's
    /// precision loss on a single small delta is negligible.
    [[nodiscard]] virtual float GetDeltaTimeFloatUVE() const = 0;

    /// Total real (unclamped) seconds elapsed across every Tick() since
    /// construction or the last Reset(). This is wall-clock elapsed time,
    /// not simulated/game time — it is not affected by the delta-time
    /// clamp, so it stays accurate even across a long stall.
    [[nodiscard]] virtual double GetTotalTimeUVE() const = 0;

    /// Resets delta time, total time, and the fixed-step accumulator to
    /// zero, and resynchronizes the internal clock reference so the next
    /// Tick() computes a sane delta rather than one spanning the time since
    /// construction.
    virtual void Reset() = 0;

    /// Sets the maximum value GetDeltaTimeUVE() (and therefore each
    /// accumulator increment) can report in a single Tick(), regardless of
    /// how much real time actually elapsed. Default is 0.25 seconds.
    virtual void SetMaxDeltaTimeUVE(double maxDeltaSeconds) = 0;

    /// Sets the size, in seconds, of one fixed simulation step consumed by
    /// AdvanceFixedStepUVE(). Default is 1/60 second.
    virtual void SetFixedTimestepUVE(double fixedDeltaSeconds) = 0;

    /// Consumes as many fixed-size chunks of accumulated time as are
    /// available (capped at an internal maximum steps-per-call to avoid an
    /// unbounded catch-up loop), returning how many steps to run and the
    /// leftover fractional progress toward the next one.
    virtual FixedStepResultUVE AdvanceFixedStepUVE() = 0;

    /// Discards only fixed-step progress accumulated by Tick(). Wall-clock total time, the current
    /// frame delta, and the clock reference remain unchanged. Used by a paused transient session to
    /// prevent catch-up physics when normal simulation resumes.
    virtual void DiscardFixedStepAccumulatorUVE() noexcept = 0;
};

} // namespace UVE::Utilities
