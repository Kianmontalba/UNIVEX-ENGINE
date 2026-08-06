// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>
#include <vector>

#include "uve/input/input_binding_uve.h"

namespace UVE::Input {

/// Whether an InputActionUVE reports a discrete triggered/held/released state (Button) or a
/// continuous value in [-1, 1] (Axis1D).
enum class InputActionTypeUVE {
    Button,
    Axis1D,
};

/// A named, spec-style ("like Unity Input System") mapping from one or more physical inputs to a
/// single gameplay-facing action — e.g. `{"Jump", Button, {KeyBindingUVE(KeyCodeUVE::Space)}, {}}`
/// or `{"MoveHorizontal", Axis1D, {KeyBindingUVE(KeyCodeUVE::D)}, {KeyBindingUVE(KeyCodeUVE::A)}}`.
/// `positiveBindings` are OR'd together for a Button action (any bound input down triggers it) or
/// contribute +1 for an Axis1D action; `negativeBindings` are only meaningful for Axis1D
/// (contribute -1) and are ignored for Button actions.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct InputActionUVE {
    std::string name;
    InputActionTypeUVE type = InputActionTypeUVE::Button;
    std::vector<InputBindingUVE> positiveBindings;
    std::vector<InputBindingUVE> negativeBindings;
};

} // namespace UVE::Input
