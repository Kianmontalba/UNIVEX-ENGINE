// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <mutex>

#include "uve/input/i_gamepad_input_system_uve.h"

namespace UVE::Input {

class GamepadInputSystemUVE final : public IGamepadInputSystemUVE {
public:
    explicit GamepadInputSystemUVE(float deadZone = 0.15F) noexcept;

    void SetConnectedUVE(std::size_t gamepadIndex, bool connected) override;
    void SetAxisStateUVE(std::size_t gamepadIndex, GamepadAxisUVE axis, float value) override;
    void SetButtonStateUVE(std::size_t gamepadIndex, GamepadButtonUVE button, bool isDown) override;
    void UpdateUVE() override;

    [[nodiscard]] GamepadStateSnapshotUVE GetSnapshotUVE(std::size_t gamepadIndex) const override;
    [[nodiscard]] GamepadStateSnapshotUVE GetPreviousSnapshotUVE(std::size_t gamepadIndex) const override;
    [[nodiscard]] float GetAxisValueUVE(std::size_t gamepadIndex, GamepadAxisUVE axis) const override;
    [[nodiscard]] bool IsButtonDownUVE(std::size_t gamepadIndex, GamepadButtonUVE button) const override;
    [[nodiscard]] bool WasButtonPressedThisFrameUVE(std::size_t gamepadIndex,
                                                     GamepadButtonUVE button) const override;
    [[nodiscard]] bool WasButtonReleasedThisFrameUVE(std::size_t gamepadIndex,
                                                      GamepadButtonUVE button) const override;
    [[nodiscard]] float GetDeadZoneUVE() const noexcept override { return m_deadZone; }

private:
    using SnapshotArrayUVE = std::array<GamepadStateSnapshotUVE, kMaximumGamepadCountUVE>;

    [[nodiscard]] static bool IsValidGamepadIndexUVE(std::size_t gamepadIndex) noexcept;
    [[nodiscard]] static bool IsValidAxisUVE(GamepadAxisUVE axis) noexcept;
    [[nodiscard]] static bool IsValidButtonUVE(GamepadButtonUVE button) noexcept;
    [[nodiscard]] float NormalizeAxisValueUVE(float value) const noexcept;

    const float m_deadZone;

    mutable std::mutex m_liveStateMutex;
    SnapshotArrayUVE m_liveState{};
    SnapshotArrayUVE m_currentState{};
    SnapshotArrayUVE m_previousState{};
    std::uint64_t m_frameNumber = 0U;
};

} // namespace UVE::Input
