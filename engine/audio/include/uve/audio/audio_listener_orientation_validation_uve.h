// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Audio {

/// Validates the copied listener basis inputs used by AudioSystemUVE. Both vectors must be finite
/// and have a finite, strictly positive squared length. This does not normalize the basis, enforce
/// orthogonality, or select an audio backend.
[[nodiscard]] bool IsAudioListenerOrientationValidUVE(Math::Vector3UVE forward,
                                                       Math::Vector3UVE up) noexcept;

} // namespace UVE::Audio
