//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

#include <cstdint>
#include <string_view>

namespace UVE::Core {

/// The engine's formal lifecycle state. EngineCoreUVE always transitions
/// through these states strictly in order, one step at a time — see
/// IsValidTransitionUVE().
enum class EngineStateUVE : std::uint8_t {
    Uninitialized = 0,
    Initializing = 1,
    Running = 2,
    ShuttingDown = 3,
    Shutdown = 4,
};

/// Returns the display name of an EngineStateUVE value.
[[nodiscard]] constexpr std::string_view ToStringUVE(EngineStateUVE state) noexcept {
    switch (state) {
        case EngineStateUVE::Uninitialized:
            return "Uninitialized";
        case EngineStateUVE::Initializing:
            return "Initializing";
        case EngineStateUVE::Running:
            return "Running";
        case EngineStateUVE::ShuttingDown:
            return "ShuttingDown";
        case EngineStateUVE::Shutdown:
            return "Shutdown";
    }
    return "Unknown";
}

/// Returns true if moving from `from` directly to `to` is a valid single
/// step in the engine lifecycle: Uninitialized -> Initializing -> Running
/// -> ShuttingDown -> Shutdown, strictly in that order. No state is ever
/// revisited and no step may be skipped.
[[nodiscard]] constexpr bool IsValidTransitionUVE(EngineStateUVE from, EngineStateUVE to) noexcept {
    switch (from) {
        case EngineStateUVE::Uninitialized:
            return to == EngineStateUVE::Initializing;
        case EngineStateUVE::Initializing:
            return to == EngineStateUVE::Running;
        case EngineStateUVE::Running:
            return to == EngineStateUVE::ShuttingDown;
        case EngineStateUVE::ShuttingDown:
            return to == EngineStateUVE::Shutdown;
        case EngineStateUVE::Shutdown:
            return false;
    }
    return false;
}

} // namespace UVE::Core
