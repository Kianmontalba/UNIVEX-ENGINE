// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/mobile_input_system_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Input {

Math::Vector2UVE MobileInputSystemUVE::SanitizePositionUVE(const Math::Vector2UVE position) noexcept {
    return Math::Vector2UVE{std::isfinite(position.x) ? position.x : 0.0F,
                            std::isfinite(position.y) ? position.y : 0.0F};
}

Math::Vector3UVE MobileInputSystemUVE::SanitizeRotationRateUVE(
    const Math::Vector3UVE rotationRate) noexcept {
    return Math::Vector3UVE{std::isfinite(rotationRate.x) ? rotationRate.x : 0.0F,
                            std::isfinite(rotationRate.y) ? rotationRate.y : 0.0F,
                            std::isfinite(rotationRate.z) ? rotationRate.z : 0.0F};
}

float MobileInputSystemUVE::SanitizePressureUVE(const float pressure) noexcept {
    return std::isfinite(pressure) ? std::clamp(pressure, 0.0F, 1.0F) : 0.0F;
}

void MobileInputSystemUVE::SetTouchStateUVE(const std::size_t touchSlot, const bool active,
                                            const std::uint64_t identifier,
                                            const Math::Vector2UVE position, const float pressure) {
    if (touchSlot >= kMaximumTouchCountUVE || (active && identifier == 0U)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    if (active) {
        for (std::size_t otherSlot = 0U; otherSlot < kMaximumTouchCountUVE; ++otherSlot) {
            if (otherSlot != touchSlot && m_liveState.touches[otherSlot].active &&
                m_liveState.touches[otherSlot].identifier == identifier) {
                return;
            }
        }
    }
    TouchPointStateUVE& touch = m_liveState.touches[touchSlot];
    touch.active = active;
    touch.identifier = active ? identifier : 0U;
    touch.position = active ? SanitizePositionUVE(position) : Math::Vector2UVE{};
    touch.delta = {};
    touch.pressure = active ? SanitizePressureUVE(pressure) : 0.0F;
}

void MobileInputSystemUVE::SetGyroscopeRotationRateUVE(const Math::Vector3UVE rotationRate) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveState.gyroscopeRotationRate = SanitizeRotationRateUVE(rotationRate);
}

void MobileInputSystemUVE::UpdateUVE() {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_previousState = m_currentState;
    ++m_liveState.frameNumber;
    m_currentState = m_liveState;

    for (std::size_t touchSlot = 0U; touchSlot < kMaximumTouchCountUVE; ++touchSlot) {
        TouchPointStateUVE& currentTouch = m_currentState.touches[touchSlot];
        const TouchPointStateUVE& previousTouch = m_previousState.touches[touchSlot];
        if (currentTouch.active && previousTouch.active &&
            currentTouch.identifier == previousTouch.identifier) {
            const Math::Vector2UVE delta = currentTouch.position - previousTouch.position;
            currentTouch.delta = std::isfinite(delta.x) && std::isfinite(delta.y)
                ? delta
                : Math::Vector2UVE{};
        } else {
            currentTouch.delta = {};
        }
    }
}

MobileInputSnapshotUVE MobileInputSystemUVE::GetSnapshotUVE() const {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_currentState;
}

MobileInputSnapshotUVE MobileInputSystemUVE::GetPreviousSnapshotUVE() const {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_previousState;
}

} // namespace UVE::Input
