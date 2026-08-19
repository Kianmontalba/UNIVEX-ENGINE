// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>

namespace UVE::Physics {

struct HingeMotorLimitInputUVE final {
    float currentAngleRadians = 0.0F;
    float currentRelativeSpeedRadians = 0.0F;
    float deltaTimeSeconds = 0.0F;
    float effectiveInverseInertia = 0.0F;
    bool motorEnabled = false;
    float targetSpeedRadians = 0.0F;
    float maximumMotorTorque = 0.0F;
    bool limitsEnabled = false;
    float minimumAngleRadians = 0.0F;
    float maximumAngleRadians = 0.0F;
};

struct HingeMotorLimitResultUVE final {
    float angularSpeedDeltaRadians = 0.0F;
    float angleCorrectionRadians = 0.0F;
    bool motorApplied = false;
    bool limitApplied = false;
};

/// Evaluates one bounded hinge motor/limit policy step over copied scalar state.
/// The caller owns all state and applies the returned speed/angle corrections to a body or
/// constraint backend; this helper does not mutate ECS, transforms, constraints, or rotations.
[[nodiscard]] bool EvaluateHingeMotorLimitUVE(
    const HingeMotorLimitInputUVE& input, HingeMotorLimitResultUVE& outResult) noexcept;

} // namespace UVE::Physics
