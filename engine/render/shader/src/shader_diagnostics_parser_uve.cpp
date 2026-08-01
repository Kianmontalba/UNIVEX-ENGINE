//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "shader_diagnostics_parser_uve.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>

namespace UVE::Render::Shader::Detail {

namespace {

[[nodiscard]] std::string ToLowerUVE(std::string_view text) {
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return result;
}

[[nodiscard]] bool ContainsWarningUVE(std::string_view text) {
    return ToLowerUVE(text).find("warning") != std::string::npos;
}

[[nodiscard]] std::vector<std::string> SplitLinesUVE(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t newlinePos = text.find('\n', start);
        if (newlinePos == std::string_view::npos) {
            lines.emplace_back(text.substr(start));
            break;
        }
        lines.emplace_back(text.substr(start, newlinePos - start));
        start = newlinePos + 1;
    }
    return lines;
}

} // namespace

std::vector<ShaderCompileErrorUVE> ParseGlInfoLogUVE(std::string_view rawLog,
                                                      const std::vector<std::string>& fileIndexTable) {
    std::vector<ShaderCompileErrorUVE> diagnostics;
    if (rawLog.empty()) {
        return diagnostics;
    }

    // Group 1: file/source-string index. Group 2: line number. Group 3 (optional): column, in
    // parens (Mesa-style). Group 4: the rest of the message.
    static const std::regex kLinePattern(R"(^(?:ERROR:\s*)?(\d+):(\d+)(?:\((\d+)\))?:\s*(.*)$)");

    for (const std::string& rawLine : SplitLinesUVE(rawLog)) {
        if (rawLine.empty()) {
            continue;
        }

        std::smatch match;
        ShaderCompileErrorUVE error;
        error.rawLogLine = rawLine;

        if (std::regex_match(rawLine, match, kLinePattern)) {
            const auto fileIndex = static_cast<std::size_t>(std::strtoul(match[1].str().c_str(), nullptr, 10));
            error.filePath = fileIndex < fileIndexTable.size() ? fileIndexTable[fileIndex] : std::string();
            error.lineNumber = static_cast<std::uint32_t>(std::strtoul(match[2].str().c_str(), nullptr, 10));
            error.columnNumber =
                match[3].matched ? static_cast<std::uint32_t>(std::strtoul(match[3].str().c_str(), nullptr, 10)) : 0;
            error.message = match[4].str();
            error.isWarning = ContainsWarningUVE(error.message);
        } else {
            error.message = rawLine;
            error.isWarning = ContainsWarningUVE(rawLine);
        }

        diagnostics.push_back(std::move(error));
    }

    return diagnostics;
}

} // namespace UVE::Render::Shader::Detail
