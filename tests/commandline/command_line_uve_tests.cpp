//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/commandline/command_line_uve.h"

#include <atomic>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::CommandLine::Tests {
namespace {

TEST(CommandLineUVETest, EmptyArgs_NoFlagsPresent) {
    const CommandLineUVE commandLine(std::vector<std::string>{});
    EXPECT_FALSE(commandLine.HasFlagUVE("project"));
    EXPECT_FALSE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("project", "default"), "default");
}

TEST(CommandLineUVETest, FlagWithValue_IsCapturedUnderNormalizedName) {
    const CommandLineUVE commandLine(std::vector<std::string>{"--project", "MyGame"});
    EXPECT_TRUE(commandLine.HasFlagUVE("project"));
    EXPECT_EQ(commandLine.GetValueUVE("project", "default"), "MyGame");
}

TEST(CommandLineUVETest, RawDashDashPrefixedQuery_DoesNotMatch) {
    const CommandLineUVE commandLine(std::vector<std::string>{"--project", "MyGame", "--server"});

    EXPECT_TRUE(commandLine.HasFlagUVE("project"));
    EXPECT_EQ(commandLine.GetValueUVE("project", ""), "MyGame");
    EXPECT_TRUE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("server", "default"), "default");

    // Proves the normalization layer actually strips "--" rather than being a no-op.
    EXPECT_FALSE(commandLine.HasFlagUVE("--project"));
    EXPECT_FALSE(commandLine.HasFlagUVE("--server"));
    EXPECT_EQ(commandLine.GetValueUVE("--project", "default"), "default");
}

TEST(CommandLineUVETest, AbsentFlag_HasFlagFalseAndGetValueReturnsDefault) {
    const CommandLineUVE commandLine(std::vector<std::string>{"--project", "MyGame"});
    EXPECT_FALSE(commandLine.HasFlagUVE("build"));
    EXPECT_EQ(commandLine.GetValueUVE("build", "fallback"), "fallback");
}

TEST(CommandLineUVETest, PresenceOnlyFlag_AsLastToken_HasFlagTrueValueIsDefault) {
    const CommandLineUVE commandLine(std::vector<std::string>{"--server"});
    EXPECT_TRUE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("server", "unset"), "unset");
}

TEST(CommandLineUVETest, PresenceOnlyFlag_FollowedByAnotherFlag_BothRecognizedIndependently) {
    const CommandLineUVE commandLine(std::vector<std::string>{"--server", "--verbose"});
    EXPECT_TRUE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("server", "unset"), "unset");
    EXPECT_TRUE(commandLine.HasFlagUVE("verbose"));
    EXPECT_EQ(commandLine.GetValueUVE("verbose", "unset"), "unset");
}

TEST(CommandLineUVETest, MixedFlags_HubStyleInvocation_ParsesAllCorrectly) {
    const CommandLineUVE commandLine(
        std::vector<std::string>{"--project", "/path/to/project", "--build", "linux", "--server"});
    EXPECT_EQ(commandLine.GetValueUVE("project", ""), "/path/to/project");
    EXPECT_EQ(commandLine.GetValueUVE("build", ""), "linux");
    EXPECT_TRUE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("server", "unset"), "unset");
}

TEST(CommandLineUVETest, ArgcArgvConstructor_SkipsProgramPath_MatchesVectorConstructor) {
    const char* rawArgs[] = {"/usr/bin/uve_runtime", "--project", "MyGame", "--server"};
    const int argc = static_cast<int>(std::size(rawArgs));
    const CommandLineUVE commandLine(argc, const_cast<char**>(rawArgs));

    EXPECT_TRUE(commandLine.HasFlagUVE("project"));
    EXPECT_EQ(commandLine.GetValueUVE("project", ""), "MyGame");
    EXPECT_TRUE(commandLine.HasFlagUVE("server"));
    EXPECT_EQ(commandLine.GetValueUVE("server", "unset"), "unset");
}

TEST(CommandLineUVETest, ArgcArgvConstructor_ArgcOne_NoArgumentsBeyondProgramPath) {
    const char* rawArgs[] = {"/usr/bin/uve_runtime"};
    const int argc = static_cast<int>(std::size(rawArgs));
    const CommandLineUVE commandLine(argc, const_cast<char**>(rawArgs));

    EXPECT_FALSE(commandLine.HasFlagUVE("project"));
}

TEST(CommandLineUVETest, ConcurrentReads_FromManyThreads_NoDataRacesOrCrashes) {
    const CommandLineUVE commandLine(
        std::vector<std::string>{"--project", "/path/to/project", "--build", "linux", "--server"});

    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 500;
    std::atomic<int> mismatchCount{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
        threads.emplace_back([&commandLine, &mismatchCount] {
            for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
                if (!commandLine.HasFlagUVE("project") ||
                    commandLine.GetValueUVE("project", "") != "/path/to/project" ||
                    !commandLine.HasFlagUVE("server") ||
                    commandLine.GetValueUVE("server", "unset") != "unset") {
                    mismatchCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(mismatchCount.load(), 0);
}

} // namespace
} // namespace UVE::CommandLine::Tests
