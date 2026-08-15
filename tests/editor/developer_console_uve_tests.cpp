// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/developer_console_uve.h"

#include <gtest/gtest.h>

#include <string>

namespace UVE::Editor::Tests {

TEST(DeveloperConsoleUVE, BuiltInsAndRegisteredCVarAreDeterministic) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCVar("r.vsync", "1"));

    ASSERT_TRUE(console.ExecuteUVE("help"));
    ASSERT_TRUE(console.ExecuteUVE("cvar r.vsync"));
    ASSERT_TRUE(console.ExecuteUVE("cvar r.vsync 0"));

    const DeveloperConsoleSnapshotUVE snapshot = console.GetSnapshotUVE();
    ASSERT_GE(snapshot.output.size(), 4U);
    EXPECT_EQ(snapshot.cvars.size(), 1U);
    EXPECT_EQ(snapshot.cvars.front().name, "r.vsync");
    EXPECT_EQ(snapshot.cvars.front().value, "0");
    EXPECT_EQ(snapshot.history.size(), 3U);
    EXPECT_FALSE(snapshot.outputTruncated);
}

TEST(DeveloperConsoleUVE, ExplicitCommandsAndUnknownInputStayNativeAndBounded) {
    DeveloperConsoleUVE console;
    std::string receivedArguments;
    ASSERT_TRUE(console.RegisterCommand("echo", "Append a diagnostic echo.",
                                        [&receivedArguments](DeveloperConsoleUVE& target, const std::string_view arguments) {
                                            receivedArguments = std::string(arguments);
                                            target.AppendUVE(DeveloperConsoleSeverityUVE::Info, receivedArguments);
                                        }));

    ASSERT_TRUE(console.ExecuteUVE("echo hello"));
    EXPECT_EQ(receivedArguments, "hello");
    EXPECT_FALSE(console.ExecuteUVE("notRegistered"));
    EXPECT_FALSE(console.ExecuteUVE(std::string(DeveloperConsoleUVE::kMaximumValueBytesUVE + 1U, 'x')));

    const DeveloperConsoleSnapshotUVE snapshot = console.GetSnapshotUVE();
    ASSERT_FALSE(snapshot.output.empty());
    EXPECT_EQ(snapshot.output.back().severity, DeveloperConsoleSeverityUVE::Error);
    EXPECT_EQ(snapshot.history.size(), 2U);
}

TEST(DeveloperConsoleUVE, OutputAndHistoryExposeTruncationAndClearIsExplicit) {
    DeveloperConsoleUVE console;
    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumOutputUVE + 4U; ++index) {
        console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "entry");
    }
    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumHistoryUVE + 4U; ++index) {
        ASSERT_TRUE(console.ExecuteUVE("help"));
    }

    const DeveloperConsoleSnapshotUVE beforeClear = console.GetSnapshotUVE();
    EXPECT_TRUE(beforeClear.outputTruncated);
    EXPECT_TRUE(beforeClear.historyTruncated);
    EXPECT_EQ(beforeClear.output.size(), DeveloperConsoleUVE::kMaximumOutputUVE);
    EXPECT_EQ(beforeClear.history.size(), DeveloperConsoleUVE::kMaximumHistoryUVE);
    EXPECT_TRUE(console.ClearUVE());
    EXPECT_FALSE(console.ClearUVE());
    EXPECT_TRUE(console.GetSnapshotUVE().output.empty());
}

} // namespace UVE::Editor::Tests
