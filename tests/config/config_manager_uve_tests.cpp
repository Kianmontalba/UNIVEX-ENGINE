//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/config/config_manager_uve.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Config::Tests {
namespace {

/// Writes `contents` to `path`, overwriting any existing file. Used to set
/// up JSON fixture files for LoadUVE() tests.
void WriteFixtureFileUVE(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream file(path);
    ASSERT_TRUE(file.is_open());
    file << contents;
}

TEST(ConfigManagerUVETest, LoadUVE_NonexistentPath_ReturnsFalseAndDefaultsAreReturned) {
    ConfigManagerUVE config;
    EXPECT_FALSE(config.LoadUVE("uve_config_tests_nonexistent.uvesettings"));
    EXPECT_EQ(config.GetStringUVE("editor.theme", "light"), "light");
    EXPECT_EQ(config.GetIntUVE("window.width", 1280), 1280);
}

TEST(ConfigManagerUVETest, LoadUVE_ValidFixture_ReadsAllFourScalarTypesAcrossNestedPaths) {
    const std::filesystem::path fixturePath = "uve_config_tests_valid.uvesettings";
    WriteFixtureFileUVE(fixturePath, R"({
        "version": 1,
        "editor": {
            "theme": "dark"
        },
        "window": {
            "width": 1920,
            "scale": 1.5
        },
        "server": {
            "enabled": true
        }
    })");

    ConfigManagerUVE config;
    ASSERT_TRUE(config.LoadUVE(fixturePath));

    EXPECT_EQ(config.GetStringUVE("editor.theme", "light"), "dark");
    EXPECT_EQ(config.GetIntUVE("window.width", 0), 1920);
    EXPECT_DOUBLE_EQ(config.GetDoubleUVE("window.scale", 0.0), 1.5);
    EXPECT_TRUE(config.GetBoolUVE("server.enabled", false));
    EXPECT_EQ(config.GetIntUVE("version", 0), 1);

    std::filesystem::remove(fixturePath);
}

TEST(ConfigManagerUVETest, GetXUVE_MissingKey_ReturnsDefault) {
    ConfigManagerUVE config;
    EXPECT_EQ(config.GetStringUVE("does.not.exist", "fallback"), "fallback");
    EXPECT_EQ(config.GetIntUVE("does.not.exist", 7), 7);
    EXPECT_DOUBLE_EQ(config.GetDoubleUVE("does.not.exist", 2.5), 2.5);
    EXPECT_FALSE(config.GetBoolUVE("does.not.exist", false));
}

TEST(ConfigManagerUVETest, GetXUVE_WrongType_ReturnsDefault) {
    ConfigManagerUVE config;
    config.SetStringUVE("editor.theme", "dark");

    EXPECT_EQ(config.GetIntUVE("editor.theme", 42), 42);
    EXPECT_DOUBLE_EQ(config.GetDoubleUVE("editor.theme", 1.5), 1.5);
    EXPECT_TRUE(config.GetBoolUVE("editor.theme", true));
}

TEST(ConfigManagerUVETest, SetAndGetXUVE_RoundTripsEachScalarType) {
    ConfigManagerUVE config;
    config.SetStringUVE("editor.theme", "dark");
    config.SetIntUVE("window.width", 1600);
    config.SetDoubleUVE("window.scale", 1.25);
    config.SetBoolUVE("server.enabled", true);

    EXPECT_EQ(config.GetStringUVE("editor.theme", ""), "dark");
    EXPECT_EQ(config.GetIntUVE("window.width", 0), 1600);
    EXPECT_DOUBLE_EQ(config.GetDoubleUVE("window.scale", 0.0), 1.25);
    EXPECT_TRUE(config.GetBoolUVE("server.enabled", false));
}

TEST(ConfigManagerUVETest, SetStringUVE_OnBrandNewNestedPath_CreatesIntermediateObjects) {
    ConfigManagerUVE config;
    EXPECT_FALSE(config.HasKeyUVE("a.b.c"));

    config.SetStringUVE("a.b.c", "leaf");

    EXPECT_TRUE(config.HasKeyUVE("a.b.c"));
    EXPECT_EQ(config.GetStringUVE("a.b.c", ""), "leaf");
}

TEST(ConfigManagerUVETest, SaveThenLoad_RoundTripsThroughDisk) {
    const std::filesystem::path savePath = "uve_config_tests_roundtrip.uvesettings";
    std::filesystem::remove(savePath);

    {
        ConfigManagerUVE writer;
        writer.SetStringUVE("editor.theme", "dark");
        writer.SetIntUVE("window.width", 1440);
        writer.SetDoubleUVE("window.scale", 2.0);
        writer.SetBoolUVE("server.enabled", true);
        ASSERT_TRUE(writer.SaveUVE(savePath));
    }

    ConfigManagerUVE reader;
    ASSERT_TRUE(reader.LoadUVE(savePath));
    EXPECT_EQ(reader.GetStringUVE("editor.theme", ""), "dark");
    EXPECT_EQ(reader.GetIntUVE("window.width", 0), 1440);
    EXPECT_DOUBLE_EQ(reader.GetDoubleUVE("window.scale", 0.0), 2.0);
    EXPECT_TRUE(reader.GetBoolUVE("server.enabled", false));

    std::filesystem::remove(savePath);
}

TEST(ConfigManagerUVETest, SaveUVE_NoArg_WritesToMostRecentlyLoadedPath) {
    const std::filesystem::path savePath = "uve_config_tests_save_no_arg.uvesettings";
    std::filesystem::remove(savePath);

    ConfigManagerUVE config;
    config.LoadUVE(savePath); // fails (file doesn't exist yet) but remembers savePath
    config.SetStringUVE("editor.theme", "dark");
    ASSERT_TRUE(config.SaveUVE());

    ConfigManagerUVE reader;
    ASSERT_TRUE(reader.LoadUVE(savePath));
    EXPECT_EQ(reader.GetStringUVE("editor.theme", ""), "dark");

    std::filesystem::remove(savePath);
}

TEST(ConfigManagerUVETest, LoadUVE_MalformedJson_ReturnsFalseAndLogsError) {
    const std::filesystem::path fixturePath = "uve_config_tests_malformed.uvesettings";
    WriteFixtureFileUVE(fixturePath, "{ not valid json");

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ConfigManagerUVE config;
    EXPECT_FALSE(config.LoadUVE(fixturePath));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundParseError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("failed to parse") != std::string::npos;
        });
    EXPECT_TRUE(foundParseError);

    logger.Shutdown();
    std::filesystem::remove(fixturePath);
}

TEST(ConfigManagerUVETest, LoadUVE_MissingFile_LogsWarning) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ConfigManagerUVE config;
    EXPECT_FALSE(config.LoadUVE("uve_config_tests_still_nonexistent.uvesettings"));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundWarning =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Warning &&
                   message.message.find("not found") != std::string::npos;
        });
    EXPECT_TRUE(foundWarning);

    logger.Shutdown();
}

TEST(ConfigManagerUVETest, HasKeyUVE_TrueForLeafFalseForMissingAndForIntermediateObject) {
    ConfigManagerUVE config;
    config.SetStringUVE("editor.theme", "dark");

    EXPECT_TRUE(config.HasKeyUVE("editor.theme"));
    EXPECT_FALSE(config.HasKeyUVE("editor.missing"));
    EXPECT_FALSE(config.HasKeyUVE("editor")); // reaches an intermediate object, not a leaf
}

TEST(ConfigManagerUVETest, ConcurrentReads_FromManyThreads_NoDataRacesOrCrashes) {
    ConfigManagerUVE config;
    config.SetStringUVE("editor.theme", "dark");
    config.SetIntUVE("window.width", 1920);

    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 500;
    std::atomic<int> mismatchCount{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
        threads.emplace_back([&config, &mismatchCount] {
            for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
                if (config.GetStringUVE("editor.theme", "") != "dark" ||
                    config.GetIntUVE("window.width", 0) != 1920 || !config.HasKeyUVE("editor.theme") ||
                    config.HasKeyUVE("editor.missing")) {
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
} // namespace UVE::Config::Tests
