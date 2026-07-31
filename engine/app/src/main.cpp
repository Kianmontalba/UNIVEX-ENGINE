//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include <string>
#include <vector>

#include "uve/core/engine_core_uve.h"

/// Headless proof-of-life entry point for the UniVex Engine. There is no
/// windowing/rendering backend yet (this build environment has no display
/// or GPU), so "running the engine" here means driving a full Init -> Load
/// -> N x (BeginFrame/Update/LateUpdate/Render/EndFrame) -> Shutdown cycle
/// cleanly through EngineCoreUVE and exiting 0 — the adapted, headless form
/// of the "initializes the engine and runs a blank frame" proof-of-life.
/// `argv[1..argc)` (the program path at argv[0] excluded) is forwarded to
/// EngineConfigUVE::commandLineArgs, so real invocations like
/// `uve_runtime --project <path>` or `uve_runtime --server` (the shapes
/// the Hub launches this executable with) are parsed by CommandLineUVE.
int main(int argc, char** argv) {
    UVE::Core::EngineConfigUVE config{};
    config.logFilePath = "uve_engine.log";
    config.commandLineArgs = std::vector<std::string>(argv + 1, argv + argc);

    UVE::Core::EngineCoreUVE engine(config);

    constexpr int kFrameCount = 60; // ~1 second of frames at nominal 60 Hz
    return engine.RunUVE(kFrameCount);
}
