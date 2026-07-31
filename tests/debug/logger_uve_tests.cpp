//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/debug/logger_uve.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Debug::Tests {
namespace {

class LoggerUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        logger.Init(LogLevelUVE::Trace);
        auto memorySink = std::make_unique<MemorySinkUVE>();
        memorySinkPtr = memorySink.get();
        logger.AddSink(std::move(memorySink));
    }

    void TearDown() override { logger.Shutdown(); }

    LoggerUVE logger;
    MemorySinkUVE* memorySinkPtr = nullptr;
};

TEST_F(LoggerUVETest, ActiveInstance_SetDuringInit) {
    EXPECT_EQ(LoggerUVE::GetActiveInstanceUVE(), &logger);
}

TEST_F(LoggerUVETest, ActiveInstance_ClearedAfterShutdown) {
    logger.Shutdown();
    EXPECT_EQ(LoggerUVE::GetActiveInstanceUVE(), nullptr);
}

TEST_F(LoggerUVETest, MinLevel_FiltersLowerSeverity) {
    logger.SetMinLevel(LogLevelUVE::Warning);
    logger.LogFormatted(LogLevelUVE::Info, "", __FILE__, __LINE__, "info message");
    EXPECT_TRUE(memorySinkPtr->GetMessagesUVE().empty());

    logger.LogFormatted(LogLevelUVE::Error, "", __FILE__, __LINE__, "error message");
    EXPECT_EQ(memorySinkPtr->GetMessagesUVE().size(), 1U);
}

TEST_F(LoggerUVETest, AllLevels_RoundTripCorrectly) {
    constexpr LogLevelUVE kLevels[] = {LogLevelUVE::Trace,   LogLevelUVE::Debug, LogLevelUVE::Info,
                                        LogLevelUVE::Warning, LogLevelUVE::Error, LogLevelUVE::Fatal};
    for (const LogLevelUVE level : kLevels) {
        memorySinkPtr->Clear();
        logger.LogFormatted(level, "", __FILE__, __LINE__, "message");
        ASSERT_EQ(memorySinkPtr->GetMessagesUVE().size(), 1U);
        EXPECT_EQ(memorySinkPtr->GetMessagesUVE().front().level, level);
    }
}

TEST_F(LoggerUVETest, MultipleSinks_AllReceiveSameMessage) {
    auto secondSink = std::make_unique<MemorySinkUVE>();
    MemorySinkUVE* secondSinkPtr = secondSink.get();
    logger.AddSink(std::move(secondSink));

    logger.LogFormatted(LogLevelUVE::Info, "", __FILE__, __LINE__, "hello");

    ASSERT_EQ(memorySinkPtr->GetMessagesUVE().size(), 1U);
    ASSERT_EQ(secondSinkPtr->GetMessagesUVE().size(), 1U);
    EXPECT_EQ(memorySinkPtr->GetMessagesUVE().front().message, "hello");
    EXPECT_EQ(secondSinkPtr->GetMessagesUVE().front().message, "hello");
}

TEST_F(LoggerUVETest, MemorySink_ClearRemovesMessages) {
    logger.LogFormatted(LogLevelUVE::Info, "", __FILE__, __LINE__, "hello");
    ASSERT_FALSE(memorySinkPtr->GetMessagesUVE().empty());
    memorySinkPtr->Clear();
    EXPECT_TRUE(memorySinkPtr->GetMessagesUVE().empty());
}

TEST_F(LoggerUVETest, FileSink_WritesLineToFile) {
    const std::filesystem::path tempPath =
        std::filesystem::temp_directory_path() / "uve_logger_test_file_sink.log";
    std::filesystem::remove(tempPath);

    {
        LoggerUVE fileLogger;
        fileLogger.Init(LogLevelUVE::Trace);
        fileLogger.AddSink(std::make_unique<FileSinkUVE>(tempPath));
        fileLogger.LogFormatted(LogLevelUVE::Info, "", __FILE__, __LINE__, "file sink message");
        fileLogger.Shutdown();
    }

    ASSERT_TRUE(std::filesystem::exists(tempPath));
    std::ifstream fileStream(tempPath);
    std::stringstream contents;
    contents << fileStream.rdbuf();
    EXPECT_NE(contents.str().find("file sink message"), std::string::npos);

    std::filesystem::remove(tempPath);
}

TEST_F(LoggerUVETest, FatalLevel_FlushesAllSinks) {
    class FlushCountingSinkUVE final : public ILogSinkUVE {
    public:
        void Write(const LogMessageUVE&) override {}
        void Flush() override { ++flushCount; }
        int flushCount = 0;
    };

    auto flushSink = std::make_unique<FlushCountingSinkUVE>();
    FlushCountingSinkUVE* flushSinkPtr = flushSink.get();
    logger.AddSink(std::move(flushSink));

    logger.LogFormatted(LogLevelUVE::Fatal, "", __FILE__, __LINE__, "fatal message");
    EXPECT_GT(flushSinkPtr->flushCount, 0);
}

TEST_F(LoggerUVETest, MacroFamily_ProducesCorrectLevelsAndMessages) {
    UVE_TRACE("trace {}", 1);
    UVE_INFO("info {}", 2);
    UVE_WARNING("warning {}", 3);
    UVE_ERROR("error {}", 4);
    UVE_FATAL("fatal {}", 5);

    const std::vector<LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    ASSERT_EQ(messages.size(), 5U);
    EXPECT_EQ(messages[0].level, LogLevelUVE::Trace);
    EXPECT_EQ(messages[1].level, LogLevelUVE::Info);
    EXPECT_EQ(messages[2].level, LogLevelUVE::Warning);
    EXPECT_EQ(messages[3].level, LogLevelUVE::Error);
    EXPECT_EQ(messages[4].level, LogLevelUVE::Fatal);
    EXPECT_EQ(messages[0].message, "trace 1");
    EXPECT_EQ(messages[4].message, "fatal 5");
}

TEST_F(LoggerUVETest, FormattedMessage_SubstitutesArgumentsCorrectly) {
    UVE_INFO("value={} name={}", 42, "abc");
    ASSERT_EQ(memorySinkPtr->GetMessagesUVE().size(), 1U);
    EXPECT_EQ(memorySinkPtr->GetMessagesUVE().front().message, "value=42 name=abc");
}

} // namespace
} // namespace UVE::Debug::Tests
