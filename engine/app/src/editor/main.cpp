// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_stdio_uve.h"
#include "uve/editor/editor_uve.h"

namespace {

struct EditorLaunchOptionsUVE final {
    std::filesystem::path scenePath = "editor_scene.uvescene";
    std::optional<int> frameLimit;
    std::optional<std::uint32_t> glMajor;
    std::optional<std::uint32_t> glMinor;
    bool headless = false;
    bool bridgeStdio = false;
};

[[nodiscard]] bool ParseGlVersionUVE(const std::string_view value, std::uint32_t& major,
                                     std::uint32_t& minor) {
    const std::size_t separator = value.find('.');
    if (separator == std::string_view::npos || separator == 0U || separator + 1U >= value.size()) {
        return false;
    }

    const std::string_view majorText = value.substr(0U, separator);
    const std::string_view minorText = value.substr(separator + 1U);
    const auto [majorEnd, majorError] =
        std::from_chars(majorText.data(), majorText.data() + majorText.size(), major);
    const auto [minorEnd, minorError] =
        std::from_chars(minorText.data(), minorText.data() + minorText.size(), minor);
    return majorError == std::errc{} && minorError == std::errc{} &&
           majorEnd == majorText.data() + majorText.size() && minorEnd == minorText.data() + minorText.size();
}

[[nodiscard]] EditorLaunchOptionsUVE ParseOptionsUVE(const int argc, char** argv) {
    EditorLaunchOptionsUVE options{};
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--headless") {
            options.headless = true;
            continue;
        }
        if (argument == "--bridge-stdio") {
            options.bridgeStdio = true;
            options.headless = true;
            continue;
        }
        if (argument == "--scene" && index + 1 < argc) {
            options.scenePath = argv[++index];
            continue;
        }
        if (argument == "--frames" && index + 1 < argc) {
            int frameLimit = 0;
            const std::string_view value{argv[++index]};
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), frameLimit);
            if (error == std::errc{} && end == value.data() + value.size() && frameLimit >= 0) {
                options.frameLimit = frameLimit;
            }
            continue;
        }
        if (argument == "--gl-version" && index + 1 < argc) {
            std::uint32_t major = 0;
            std::uint32_t minor = 0;
            if (ParseGlVersionUVE(argv[++index], major, minor)) {
                options.glMajor = major;
                options.glMinor = minor;
            }
        }
    }
    if (options.headless && !options.frameLimit.has_value()) {
        options.frameLimit = 1;
    }
    return options;
}

} // namespace

/// Starts the standalone UniVex Editor Foundation v1. `--scene <path>` selects the `.uvescene`
/// document. `--frames <n>` bounds a run for automation, while the normal windowed invocation runs
/// until the user closes the editor. `--gl-version <major.minor>` overrides the requested desktop
/// OpenGL version for an explicitly chosen platform capability (for example virtual-display CI).
/// `--headless` keeps the editor's non-visual lifecycle usable in CI and defaults to a single frame.
/// `--bridge-stdio` always implies headless mode and runs a framed JSON-RPC bridge server instead
/// of constructing native ImGui/GLFW presentation for this process.
int main(const int argc, char** argv) {
    const EditorLaunchOptionsUVE options = ParseOptionsUVE(argc, argv);

    UVE::Core::EngineConfigUVE config{};
    config.logFilePath = "uve_editor.log";
    config.enableConsoleLogging = !options.bridgeStdio;
    config.headlessUVE = options.headless;
    config.commandLineArgs = std::vector<std::string>(argv + 1, argv + argc);
    if (options.glMajor.has_value() && options.glMinor.has_value()) {
        config.windowGlVersionMajor = *options.glMajor;
        config.windowGlVersionMinor = *options.glMinor;
    }

    UVE::Core::EngineCoreUVE engine(config);
    engine.Init();
    if (!engine.Load()) {
        engine.Shutdown();
        return 1;
    }

    UVE::Editor::EditorUVE editor(engine.GetServicesUVE(), options.scenePath, 100U, &engine);
    editor.InitUVE();

    if (std::filesystem::exists(options.scenePath)) {
        static_cast<void>(editor.LoadSceneUVE());
    }

    if (options.bridgeStdio) {
        UVE::Asset::DataTableRegistryUVE dataTableRegistry;
        UVE::Editor::EditorBridgeUVE bridge(editor, &dataTableRegistry);
        UVE::Editor::EditorBridgeStdioServerUVE server(bridge);
        const int result = server.ServeUVE(std::cin, std::cout, std::cerr);
        editor.ShutdownUVE();
        engine.Shutdown();
        return result;
    }

    engine.SetActiveCameraUVE(editor.GetViewportCameraUVE());
    engine.SetPostRenderCallbackUVE([&editor] { editor.RenderOverlayUVE(); });

    int framesRun = 0;
    while (!engine.GetServicesUVE().GetWindowManagerUVE().IsCloseRequestedUVE() &&
           (!options.frameLimit.has_value() || framesRun < *options.frameLimit)) {
        editor.TickUVE();
        engine.TickFrameUVE();
        ++framesRun;
    }

    engine.SetPostRenderCallbackUVE({});
    editor.ShutdownUVE();
    engine.Shutdown();
    return 0;
}
