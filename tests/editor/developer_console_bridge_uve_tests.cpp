// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"

#include <gtest/gtest.h>

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeConsoleBridgeConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_developer_console_bridge_tests.log";
    config.settingsFilePath = "uve_developer_console_bridge_tests_settings.json";
    config.assetDatabaseFilePath = "uve_developer_console_bridge_tests_assets.json";
    config.saveDirectoryPath = "uve_developer_console_bridge_tests_saves";
    config.shaderCachePath = "uve_developer_console_bridge_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

TEST(EditorDeveloperConsoleBridgeUVE, RoutesBoundedCommandsAndCopiesGeneration) {
    Core::EngineCoreUVE engine(MakeConsoleBridgeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_developer_console_bridge.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        EXPECT_FALSE(initial.developerConsole.console.outputTruncated);
        EXPECT_TRUE(initial.developerConsole.console.generation > 0U);

        EditorBridgeRequestUVE help{};
        help.protocolVersion = kEditorBridgeProtocolVersionUVE;
        help.requestId = 1U;
        help.expectedRevision = initial.revision;
        help.kind = EditorBridgeRequestKindUVE::SubmitDeveloperConsoleCommand;
        help.developerConsoleCommand = "help";
        const EditorBridgeResponseUVE helpResponse = bridge.DispatchUVE(help);
        ASSERT_TRUE(helpResponse.applied);
        ASSERT_FALSE(helpResponse.snapshot.developerConsole.console.output.empty());
        EXPECT_GT(helpResponse.snapshot.developerConsole.console.generation,
                  initial.developerConsole.console.generation);

        EditorBridgeRequestUVE unknown = help;
        unknown.requestId = 2U;
        unknown.expectedRevision = helpResponse.snapshot.revision;
        unknown.developerConsoleCommand = "notRegistered";
        const EditorBridgeResponseUVE unknownResponse = bridge.DispatchUVE(unknown);
        EXPECT_FALSE(unknownResponse.applied);
        EXPECT_EQ(unknownResponse.code, "bridge.developer_console.command.rejected");
        ASSERT_FALSE(unknownResponse.snapshot.developerConsole.console.output.empty());
        EXPECT_EQ(unknownResponse.snapshot.developerConsole.console.output.back().severity,
                  DeveloperConsoleSeverityUVE::Error);

        EditorBridgeRequestUVE stale = help;
        stale.requestId = 3U;
        stale.expectedRevision = initial.revision;
        const EditorBridgeResponseUVE staleResponse = bridge.DispatchUVE(stale);
        EXPECT_FALSE(staleResponse.applied);
        EXPECT_EQ(staleResponse.code, "bridge.snapshot.stale");

        EditorBridgeRequestUVE clear{};
        clear.protocolVersion = kEditorBridgeProtocolVersionUVE;
        clear.requestId = 4U;
        clear.expectedRevision = unknownResponse.snapshot.revision;
        clear.kind = EditorBridgeRequestKindUVE::ClearDeveloperConsole;
        const EditorBridgeResponseUVE clearResponse = bridge.DispatchUVE(clear);
        ASSERT_TRUE(clearResponse.applied);
        EXPECT_TRUE(clearResponse.snapshot.developerConsole.console.output.empty());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
