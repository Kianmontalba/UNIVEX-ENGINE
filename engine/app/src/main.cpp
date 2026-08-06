// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include <string>
#include <vector>

#include "uve/core/engine_core_uve.h"

/// Proof-of-life entry point for the UniVex Engine. By default opens a real GLFW3 window with an
/// OpenGL 4.6 Core render device (Increment 20) and drives a full Init -> Load -> N x
/// (BeginFrame/Update/LateUpdate/Render/EndFrame) -> Shutdown cycle through EngineCoreUVE,
/// exiting 0 — the demo triangle Render() draws each frame is the "initializes the engine and
/// renders a frame" proof-of-life. Pass `--headless` to fall back to NullWindowManagerUVE/
/// NullRenderDeviceUVE instead (no display required — the mode CI and this project's own test
/// suite use). `argv[1..argc)` (the program path at argv[0] excluded) is forwarded to
/// EngineConfigUVE::commandLineArgs, so real invocations like
/// `uve_runtime --project <path>`, `uve_runtime --server`, or `uve_runtime --headless` (the
/// shapes the Hub launches this executable with) are parsed by CommandLineUVE.
int main(int argc, char** argv) {
    UVE::Core::EngineConfigUVE config{};
    config.logFilePath = "uve_engine.log";
    config.commandLineArgs = std::vector<std::string>(argv + 1, argv + argc);

    UVE::Core::EngineCoreUVE engine(config);

    constexpr int kFrameCount = 60; // ~1 second of frames at nominal 60 Hz
    return engine.RunUVE(kFrameCount);
}
