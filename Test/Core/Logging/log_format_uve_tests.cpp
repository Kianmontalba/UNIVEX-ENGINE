// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/debug/log_format_uve.h"

#include <gtest/gtest.h>

namespace UVE::Debug::Tests {
namespace {

TEST(LogFormatUVETest, NoArguments_ReturnsLiteralString) {
    EXPECT_EQ(FormatLogMessageUVE("hello world"), "hello world");
}

TEST(LogFormatUVETest, SingleIntegerArgument_Substituted) {
    EXPECT_EQ(FormatLogMessageUVE("value={}", 42), "value=42");
}

TEST(LogFormatUVETest, MultipleMixedArguments_SubstitutedInOrder) {
    EXPECT_EQ(FormatLogMessageUVE("{} and {} and {}", 1, "two", 3.5), "1 and two and 3.5");
}

TEST(LogFormatUVETest, FloatingPointPrecisionSpec_Formatted) {
    EXPECT_EQ(FormatLogMessageUVE("pi={:.2f}", 3.14159), "pi=3.14");
}

} // namespace
} // namespace UVE::Debug::Tests
