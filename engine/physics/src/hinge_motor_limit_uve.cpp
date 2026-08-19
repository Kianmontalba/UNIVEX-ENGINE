// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/hinge_motor_limit_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Physics {
namespace {

[[nodiscard]] bool IsFiniteUVE(const float value) noexcept {
    return std::isfinite(value);
}

} // namespace

bool EvaluateHingeMotorLimitUVE(
    const HingeMotorLimitInputUVE& input, HingeMotorLimitResultUVE& outResult) noexcept {
    if (!IsFiniteUVE(input.currentAngleRadians) || !IsFiniteUVE(input.currentRelativeSpeedRadians) ||
        !IsFiniteUVE(input.deltaTimeSeconds) || input.deltaTimeSeconds < 0.0F ||
        !IsFiniteUVE(input.effectiveInverseInertia) || input.effectiveInverseInertia < 0.0F ||
        !IsFiniteUVE(input.targetSpeedRadians) || !IsFiniteUVE(input.maximumMotorTorque) ||
        input.maximumMotorTorque < 0.0F || !IsFiniteUVE(input.minimumAngleRadians) ||
        !IsFiniteUVE(input.maximumAngleRadians) ||
        (input.limitsEnabled && input.minimumAngleRadians > input.maximumAngleRadians)) {
        return false;
    }

    HingeMotorLimitResultUVE candidate{};
    if (input.motorEnabled && input.deltaTimeSeconds > 0.0F &&
        input.effectiveInverseInertia > 0.0F && input.maximumMotorTorque > 0.0F) {
        const float maximumSpeedDelta = input.maximumMotorTorque * input.effectiveInverseInertia *
                                        input.deltaTimeSeconds;
        if (!IsFiniteUVE(maximumSpeedDelta)) {
            return false;
        }
        const float requestedSpeedDelta = input.targetSpeedRadians - input.currentRelativeSpeedRadians;
        candidate.angularSpeedDeltaRadians =
            std::clamp(requestedSpeedDelta, -maximumSpeedDelta, maximumSpeedDelta);
        candidate.motorApplied = std::fabs(candidate.angularSpeedDeltaRadians) > 0.0F;
    }

    if (input.limitsEnabled) {
        if (input.currentAngleRadians < input.minimumAngleRadians) {
            candidate.angleCorrectionRadians = input.minimumAngleRadians - input.currentAngleRadians;
        } else if (input.currentAngleRadians > input.maximumAngleRadians) {
            candidate.angleCorrectionRadians = input.maximumAngleRadians - input.currentAngleRadians;
        }
        candidate.limitApplied = std::fabs(candidate.angleCorrectionRadians) > 0.0F;
    }

    if (!IsFiniteUVE(candidate.angularSpeedDeltaRadians) || !IsFiniteUVE(candidate.angleCorrectionRadians)) {
        return false;
    }
    outResult = candidate;
    return true;
}

} // namespace UVE::Physics
