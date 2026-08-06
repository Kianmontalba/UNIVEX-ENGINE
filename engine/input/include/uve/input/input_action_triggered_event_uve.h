// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>

#include "uve/input/input_action_uve.h"

namespace UVE::Input {

/// Queued through IEventSystemUVE (Events::IEventSystemUVE::QueueEvent<T>()) whenever a Button
/// InputActionUVE newly transitions to triggered — mirrors AssetLoadCompletedEventUVE's precedent
/// of a plain event struct, not a bespoke observer/callback mechanism. `type`/`axisValue` let
/// this one struct also serve Axis1D actions once a future increment adds rate-limited/
/// threshold-crossing axis events; this increment only ever queues it for Button-type triggers,
/// so `type` is always `Button` and `axisValue` is always `0.0F` today.
struct InputActionTriggeredEventUVE {
    std::string actionName;
    InputActionTypeUVE type = InputActionTypeUVE::Button;
    float axisValue = 0.0F;
};

} // namespace UVE::Input
