// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/utilities/timer_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Utilities {

TimerUVE::TimerUVE() : m_lastTickTime(std::chrono::steady_clock::now()) {}

void TimerUVE::Tick() {
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    const double rawDelta = std::chrono::duration<double>(now - m_lastTickTime).count();
    m_lastTickTime = now;

    m_totalTime += rawDelta;
    m_deltaTime = std::min(rawDelta, m_maxDeltaTime);
    m_accumulator += m_deltaTime;
}

double TimerUVE::GetDeltaTimeUVE() const {
    return m_deltaTime;
}

float TimerUVE::GetDeltaTimeFloatUVE() const {
    return static_cast<float>(m_deltaTime);
}

double TimerUVE::GetTotalTimeUVE() const {
    return m_totalTime;
}

void TimerUVE::Reset() {
    m_lastTickTime = std::chrono::steady_clock::now();
    m_deltaTime = 0.0;
    m_totalTime = 0.0;
    m_accumulator = 0.0;
}

void TimerUVE::SetMaxDeltaTimeUVE(double maxDeltaSeconds) {
    m_maxDeltaTime = maxDeltaSeconds;
}

void TimerUVE::SetFixedTimestepUVE(double fixedDeltaSeconds) {
    m_fixedDeltaTime = fixedDeltaSeconds;
}

FixedStepResultUVE TimerUVE::AdvanceFixedStepUVE() {
    FixedStepResultUVE result;

    while (m_accumulator >= m_fixedDeltaTime && result.stepsToRun < kMaxStepsPerTick) {
        m_accumulator -= m_fixedDeltaTime;
        ++result.stepsToRun;
    }

    // Hit the step cap with more than a full step still buffered: drop the
    // remainder down to a single sub-step's worth rather than let the
    // accumulator grow unbounded across frames (see kMaxStepsPerTick doc).
    if (result.stepsToRun >= kMaxStepsPerTick && m_accumulator >= m_fixedDeltaTime) {
        m_accumulator = std::fmod(m_accumulator, m_fixedDeltaTime);
    }

    result.alpha = (m_fixedDeltaTime > 0.0) ? (m_accumulator / m_fixedDeltaTime) : 0.0;
    return result;
}

void TimerUVE::DiscardFixedStepAccumulatorUVE() noexcept {
    m_accumulator = 0.0;
}

} // namespace UVE::Utilities
