// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "uve/input/i_mobile_input_system_uve.h"

namespace UVE::Input {

class MobileInputSystemUVE final : public IMobileInputSystemUVE {
public:
    void SetTouchStateUVE(std::size_t touchSlot, bool active, std::uint64_t identifier,
                          Math::Vector2UVE position, float pressure) override;
    void SetGyroscopeRotationRateUVE(Math::Vector3UVE rotationRate) override;
    void UpdateUVE() override;

    [[nodiscard]] MobileInputSnapshotUVE GetSnapshotUVE() const override;
    [[nodiscard]] MobileInputSnapshotUVE GetPreviousSnapshotUVE() const override;

private:
    [[nodiscard]] static Math::Vector2UVE SanitizePositionUVE(Math::Vector2UVE position) noexcept;
    [[nodiscard]] static Math::Vector3UVE SanitizeRotationRateUVE(Math::Vector3UVE rotationRate) noexcept;
    [[nodiscard]] static float SanitizePressureUVE(float pressure) noexcept;

    mutable std::mutex m_liveStateMutex;
    MobileInputSnapshotUVE m_liveState{};
    MobileInputSnapshotUVE m_currentState{};
    MobileInputSnapshotUVE m_previousState{};
};

} // namespace UVE::Input
