//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "uve/debug/log_level_uve.h"

namespace UVE::Core {

/// Configuration passed into EngineCoreUVE's constructor. Every field has a
/// sensible default, so most callers (including tests) can construct one
/// with zero overrides. New fields can be added here in the future without
/// changing EngineCoreUVE's constructor signature.
/// Thread-safety: value type; read only during EngineCoreUVE::Init().
struct EngineConfigUVE {
    /// Target frames per second for the frame pipeline. Informational this
    /// increment — nothing throttles to it yet, since there is no frame
    /// limiter — but consumed by future systems comparing FrameStatsUVE's
    /// actual FPS against this target.
    double targetFps = 60.0;

    /// Frequency, in Hz, of the fixed-timestep simulation accumulator (see
    /// UVE::Utilities::ITimerUVE::SetFixedTimestepUVE()). Converted to
    /// seconds-per-step by EngineCoreUVE during Init().
    double fixedUpdateFps = 60.0;

    /// Maximum delta time, in seconds, a single frame may report (see
    /// UVE::Utilities::ITimerUVE::SetMaxDeltaTimeUVE()) — guards against a
    /// spiral of death after a debugger pause or long stall.
    double maxDeltaTimeSeconds = 0.25;

    /// Minimum severity a log message must have to reach any sink.
    Debug::LogLevelUVE minLogLevel = Debug::LogLevelUVE::Trace;

    /// Path the FileSinkUVE attached during Init() will append to.
    std::filesystem::path logFilePath = "uve_engine.log";

    /// Whether a ConsoleSinkUVE is attached during Init(), in addition to
    /// the FileSinkUVE (which is always attached regardless of this flag).
    bool enableConsoleLogging = true;

    /// Number of ThreadPoolUVE worker threads to spawn during Init(). `0`
    /// (the default) means "auto" — see
    /// UVE::Threading::ThreadPoolUVE::ThreadPoolUVE() for the exact
    /// resolution policy.
    std::size_t threadPoolWorkerCount = 0;

    /// Path ConfigManagerUVE::LoadUVE() is called with during Init(). A
    /// missing file at this path is not an error (see
    /// IConfigManagerUVE::LoadUVE()) — a first-run engine has no settings
    /// file yet.
    std::filesystem::path settingsFilePath = ".uvesettings";

    /// Raw startup argument tokens (excluding the program path) that
    /// CommandLineUVE parses during Init(). Populated by main() from
    /// argv[1..argc); left empty by default so tests can construct an
    /// EngineConfigUVE without a real process argv.
    std::vector<std::string> commandLineArgs = {};
};

} // namespace UVE::Core
