// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/audio_listener_orientation_validation_uve.h"

#include <cmath>

namespace UVE::Audio {
namespace {

[[nodiscard]] bool IsFiniteNonZeroVectorUVE(const Math::Vector3UVE value) noexcept {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
        return false;
    }
    const float lengthSquared = Math::LengthSquaredUVE(value);
    return std::isfinite(lengthSquared) && lengthSquared > 0.0F;
}

} // namespace

bool IsAudioListenerOrientationValidUVE(const Math::Vector3UVE forward,
                                       const Math::Vector3UVE up) noexcept {
    return IsFiniteNonZeroVectorUVE(forward) && IsFiniteNonZeroVectorUVE(up);
}

} // namespace UVE::Audio
