// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace UVE::Input {

inline constexpr std::size_t kMaximumGamepadCountUVE = 4U;
inline constexpr std::size_t kGamepadAxisCountUVE = 6U;
inline constexpr std::size_t kGamepadButtonCountUVE = 16U;

/// Fixed cross-platform axis vocabulary. Backends map their native layout into this value type.
enum class GamepadAxisUVE : std::uint8_t {
    LeftX = 0,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger,
    Count,
};

/// Fixed cross-platform button vocabulary. Backends map their native layout into this value type.
enum class GamepadButtonUVE : std::uint8_t {
    South = 0,
    East,
    West,
    North,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftStick,
    RightStick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Miscellaneous,
    Count,
};

struct GamepadStateSnapshotUVE final {
    bool connected = false;
    std::array<float, kGamepadAxisCountUVE> axes{};
    std::array<bool, kGamepadButtonCountUVE> buttons{};
    std::uint64_t frameNumber = 0U;

    [[nodiscard]] bool operator==(const GamepadStateSnapshotUVE&) const noexcept = default;
};

/// Injectable, bounded gamepad state service. Set*StateUVE() methods update a live copy and are
/// safe from any thread; UpdateUVE() commits one deterministic current/previous snapshot on the
/// main thread. Axis values are clamped to [-1,1] and normalized through the configured symmetric
/// dead zone. A disconnected pad commits a zeroed snapshot while preserving the frame counter.
class IGamepadInputSystemUVE {
public:
    virtual ~IGamepadInputSystemUVE() = default;

    virtual void SetConnectedUVE(std::size_t gamepadIndex, bool connected) = 0;
    virtual void SetAxisStateUVE(std::size_t gamepadIndex, GamepadAxisUVE axis, float value) = 0;
    virtual void SetButtonStateUVE(std::size_t gamepadIndex, GamepadButtonUVE button, bool isDown) = 0;

    /// Commits all live device state exactly once for the frame and increments its copied frame
    /// number. Must be called on the main thread.
    virtual void UpdateUVE() = 0;

    [[nodiscard]] virtual GamepadStateSnapshotUVE GetSnapshotUVE(std::size_t gamepadIndex) const = 0;
    [[nodiscard]] virtual GamepadStateSnapshotUVE GetPreviousSnapshotUVE(std::size_t gamepadIndex) const = 0;
    [[nodiscard]] virtual float GetAxisValueUVE(std::size_t gamepadIndex, GamepadAxisUVE axis) const = 0;
    [[nodiscard]] virtual bool IsButtonDownUVE(std::size_t gamepadIndex, GamepadButtonUVE button) const = 0;
    [[nodiscard]] virtual bool WasButtonPressedThisFrameUVE(std::size_t gamepadIndex,
                                                             GamepadButtonUVE button) const = 0;
    [[nodiscard]] virtual bool WasButtonReleasedThisFrameUVE(std::size_t gamepadIndex,
                                                              GamepadButtonUVE button) const = 0;
    [[nodiscard]] virtual float GetDeadZoneUVE() const noexcept = 0;
};

} // namespace UVE::Input
