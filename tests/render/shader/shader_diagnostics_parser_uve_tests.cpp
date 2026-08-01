//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "shader_diagnostics_parser_uve.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Render::Shader::Detail::Tests {
namespace {

TEST(ShaderDiagnosticsParserUVETest, MesaFormatWithColumn_ParsesLineAndMessage) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("0:12(5): error: 'foo' undeclared", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].filePath, "shaders/main.glsl");
    EXPECT_EQ(diagnostics[0].lineNumber, 12U);
    EXPECT_EQ(diagnostics[0].columnNumber, 5U);
    EXPECT_NE(diagnostics[0].message.find("foo"), std::string::npos);
    EXPECT_FALSE(diagnostics[0].isWarning);
}

TEST(ShaderDiagnosticsParserUVETest, MesaFormatWithoutColumn_ParsesLineAndMessage) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("0:7: error: syntax error", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].filePath, "shaders/main.glsl");
    EXPECT_EQ(diagnostics[0].lineNumber, 7U);
}

TEST(ShaderDiagnosticsParserUVETest, NvidiaFormat_ParsesLineAndMessage) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("ERROR: 0:12: 'foo' undeclared identifier", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].filePath, "shaders/main.glsl");
    EXPECT_EQ(diagnostics[0].lineNumber, 12U);
}

TEST(ShaderDiagnosticsParserUVETest, MultipleLines_ProducesOneDiagnosticPerLineInOrder) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("0:3: error: first problem\n0:9: error: second problem\n", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].lineNumber, 3U);
    EXPECT_EQ(diagnostics[1].lineNumber, 9U);
}

TEST(ShaderDiagnosticsParserUVETest, UnrecognizedFormat_StillProducesDiagnosticWithZeroLineNumber) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("this driver emitted something completely unstructured", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].lineNumber, 0U);
    EXPECT_NE(diagnostics[0].message.find("unstructured"), std::string::npos);
}

TEST(ShaderDiagnosticsParserUVETest, OutOfRangeFileIndex_LeavesFilePathEmptyRatherThanCrashing) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    // Source string index 5 does not exist in the (single-entry) table.
    const std::vector<ShaderCompileErrorUVE> diagnostics = ParseGlInfoLogUVE("5:12: error: oops", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_TRUE(diagnostics[0].filePath.empty());
    EXPECT_EQ(diagnostics[0].lineNumber, 12U);
}

TEST(ShaderDiagnosticsParserUVETest, EmptyFileIndexTable_LeavesFilePathEmptyRatherThanCrashing) {
    const std::vector<ShaderCompileErrorUVE> diagnostics = ParseGlInfoLogUVE("0:1: error: oops", {});

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_TRUE(diagnostics[0].filePath.empty());
}

TEST(ShaderDiagnosticsParserUVETest, EmptyLog_ProducesNoDiagnostics) {
    EXPECT_TRUE(ParseGlInfoLogUVE("", {"shaders/main.glsl"}).empty());
}

TEST(ShaderDiagnosticsParserUVETest, WarningKeyword_MarksDiagnosticAsWarningNotError) {
    const std::vector<std::string> fileIndexTable = {"shaders/main.glsl"};
    const std::vector<ShaderCompileErrorUVE> diagnostics =
        ParseGlInfoLogUVE("0:4: warning: unused variable 'x'", fileIndexTable);

    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_TRUE(diagnostics[0].isWarning);
}

} // namespace
} // namespace UVE::Render::Shader::Detail::Tests
