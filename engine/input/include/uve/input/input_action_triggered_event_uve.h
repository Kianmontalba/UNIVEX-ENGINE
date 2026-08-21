// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>

#include "uve/input/input_action_uve.h"

namespace UVE::Input {

inline constexpr float kInputActionAxisTriggerThresholdUVE = 0.5F;

/// Queued through IEventSystemUVE (Events::IEventSystemUVE::QueueEvent<T>()) when a Button action
/// newly transitions to triggered or an Axis1D action crosses from an absolute value at/below
/// `kInputActionAxisTriggerThresholdUVE` to a value above it. This remains a plain copied event
/// struct rather than a bespoke observer/callback mechanism; Axis1D events carry the current
/// clamped value in `axisValue`, while Button events retain the zero default.
struct InputActionTriggeredEventUVE {
    std::string actionName;
    InputActionTypeUVE type = InputActionTypeUVE::Button;
    float axisValue = 0.0F;
};

} // namespace UVE::Input
