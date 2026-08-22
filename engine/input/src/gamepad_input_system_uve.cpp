// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/gamepad_input_system_uve.h"
#include "uve/input/input_frame_counter_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace UVE::Input {

GamepadInputSystemUVE::GamepadInputSystemUVE(const float deadZone) noexcept
    : m_deadZone(std::isfinite(deadZone) && deadZone >= 0.0F && deadZone < 1.0F ? deadZone : 0.15F) {}

bool GamepadInputSystemUVE::IsValidGamepadIndexUVE(const std::size_t gamepadIndex) noexcept {
    return gamepadIndex < kMaximumGamepadCountUVE;
}

bool GamepadInputSystemUVE::IsValidAxisUVE(const GamepadAxisUVE axis) noexcept {
    return static_cast<std::size_t>(axis) < kGamepadAxisCountUVE;
}

bool GamepadInputSystemUVE::IsValidButtonUVE(const GamepadButtonUVE button) noexcept {
    return static_cast<std::size_t>(button) < kGamepadButtonCountUVE;
}

float GamepadInputSystemUVE::NormalizeAxisValueUVE(const float value) const noexcept {
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    const float clamped = std::clamp(value, -1.0F, 1.0F);
    const float magnitude = std::fabs(clamped);
    if (magnitude <= m_deadZone) {
        return 0.0F;
    }
    const float normalizedMagnitude = (magnitude - m_deadZone) / (1.0F - m_deadZone);
    return std::copysign(normalizedMagnitude, clamped);
}

void GamepadInputSystemUVE::SetConnectedUVE(const std::size_t gamepadIndex, const bool connected) {
    if (!IsValidGamepadIndexUVE(gamepadIndex)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveState[gamepadIndex].connected = connected;
    if (!connected) {
        m_liveState[gamepadIndex].axes.fill(0.0F);
        m_liveState[gamepadIndex].buttons.fill(false);
    }
}

void GamepadInputSystemUVE::SetAxisStateUVE(const std::size_t gamepadIndex, const GamepadAxisUVE axis,
                                            const float value) {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidAxisUVE(axis)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    if (!m_liveState[gamepadIndex].connected) {
        return;
    }
    m_liveState[gamepadIndex].axes[static_cast<std::size_t>(axis)] = value;
}

void GamepadInputSystemUVE::SetButtonStateUVE(const std::size_t gamepadIndex, const GamepadButtonUVE button,
                                              const bool isDown) {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidButtonUVE(button)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    if (!m_liveState[gamepadIndex].connected) {
        return;
    }
    m_liveState[gamepadIndex].buttons[static_cast<std::size_t>(button)] = isDown;
}

void GamepadInputSystemUVE::UpdateUVE() {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_previousState = m_currentState;
    AdvanceInputFrameNumberUVE(m_frameNumber);

    for (std::size_t gamepadIndex = 0U; gamepadIndex < kMaximumGamepadCountUVE; ++gamepadIndex) {
        GamepadStateSnapshotUVE committed{};
        committed.frameNumber = m_frameNumber;
        committed.connected = m_liveState[gamepadIndex].connected;
        if (committed.connected) {
            for (std::size_t axisIndex = 0U; axisIndex < kGamepadAxisCountUVE; ++axisIndex) {
                committed.axes[axisIndex] = NormalizeAxisValueUVE(m_liveState[gamepadIndex].axes[axisIndex]);
            }
            committed.buttons = m_liveState[gamepadIndex].buttons;
        }
        m_currentState[gamepadIndex] = committed;
    }
}

GamepadStateSnapshotUVE GamepadInputSystemUVE::GetSnapshotUVE(const std::size_t gamepadIndex) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex)) {
        return {};
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_currentState[gamepadIndex];
}

GamepadStateSnapshotUVE GamepadInputSystemUVE::GetPreviousSnapshotUVE(const std::size_t gamepadIndex) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex)) {
        return {};
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_previousState[gamepadIndex];
}

float GamepadInputSystemUVE::GetAxisValueUVE(const std::size_t gamepadIndex, const GamepadAxisUVE axis) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidAxisUVE(axis)) {
        return 0.0F;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_currentState[gamepadIndex].axes[static_cast<std::size_t>(axis)];
}

bool GamepadInputSystemUVE::IsButtonDownUVE(const std::size_t gamepadIndex, const GamepadButtonUVE button) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidButtonUVE(button)) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    return m_currentState[gamepadIndex].buttons[static_cast<std::size_t>(button)];
}

bool GamepadInputSystemUVE::WasButtonPressedThisFrameUVE(const std::size_t gamepadIndex,
                                                          const GamepadButtonUVE button) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidButtonUVE(button)) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    const std::size_t buttonIndex = static_cast<std::size_t>(button);
    return m_currentState[gamepadIndex].buttons[buttonIndex] && !m_previousState[gamepadIndex].buttons[buttonIndex];
}

bool GamepadInputSystemUVE::WasButtonReleasedThisFrameUVE(const std::size_t gamepadIndex,
                                                           const GamepadButtonUVE button) const {
    if (!IsValidGamepadIndexUVE(gamepadIndex) || !IsValidButtonUVE(button)) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    const std::size_t buttonIndex = static_cast<std::size_t>(button);
    return !m_currentState[gamepadIndex].buttons[buttonIndex] && m_previousState[gamepadIndex].buttons[buttonIndex];
}

} // namespace UVE::Input
