// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstdint>
#include <limits>

namespace UVE::Input {

/// Advances an input snapshot frame number while preserving the documented monotonic contract at
/// uint64_t saturation. The counter is value-only; it owns no device state or frame scheduling.
inline void AdvanceInputFrameNumberUVE(std::uint64_t& frameNumber) noexcept {
    if (frameNumber < std::numeric_limits<std::uint64_t>::max()) {
        ++frameNumber;
    }
}

} // namespace UVE::Input

// EOF
