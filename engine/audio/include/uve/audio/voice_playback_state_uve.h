// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

namespace UVE::Audio {

/// A voice's playback state as NullAudioDeviceUVE (or a future real backend) reports it.
/// Deliberately two-valued this increment: no real decoded audio exists to know a one-shot's
/// natural duration (see engine/audio's "explicitly out of scope" notes), so a voice never
/// transitions to Stopped on its own — only an explicit StopUVE()/DestroyVoiceUVE() call changes
/// it. A looping voice is therefore correctly "Playing" forever until stopped; a non-looping voice
/// is also "Playing" until stopped, which is honest given this increment has no way to know when
/// it would otherwise finish.
enum class VoicePlaybackStateUVE : std::uint8_t { Stopped, Playing };

} // namespace UVE::Audio
