// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>

namespace UVE::Audio {

/// Opaque handle to an audio voice created via IAudioDeviceUVE::CreateVoiceUVE() (and, one level
/// up, IAudioSystemUVE::CreateSourceUVE() — the two layers deliberately share this one handle
/// type rather than each minting their own, since AudioSystemUVE's voices and the underlying
/// device's voices are always 1:1). Mirrors Render::BufferHandleUVE's "small wrapper struct, not
/// a bare uint32_t" precedent.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct VoiceHandleUVE {
    std::uint32_t value = 0;
};

/// The sentinel "no voice" value. Never returned by a successful CreateVoiceUVE()/CreateSourceUVE()
/// call.
inline constexpr VoiceHandleUVE kInvalidVoiceHandleUVE{};

[[nodiscard]] constexpr bool operator==(const VoiceHandleUVE& lhs, const VoiceHandleUVE& rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(const VoiceHandleUVE& lhs, const VoiceHandleUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Audio

template <>
struct std::hash<UVE::Audio::VoiceHandleUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Audio::VoiceHandleUVE& handle) const noexcept {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};
