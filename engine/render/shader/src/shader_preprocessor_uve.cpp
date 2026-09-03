// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


// # Algorithm summary
//
// PreprocessShaderSourceUVE() reads the root source (a virtual file, or the caller's embedded
// fallback string if the file doesn't exist) and expands it via a single recursive, line-based
// pass shared by every #include'd file: directive lines (#include/#define/#undef/#ifdef/#ifndef/
// #else/#endif) are consumed and never copied to the output; every other line, while inside an
// active conditional branch, has every currently-#define'd macro name substituted for its value
// (token-boundary-aware) before being appended to the resolved source. #version/#extension/#line
// lines are always passed through verbatim, never macro-substituted.
//
// Every recursive #include is wrapped with GL-native `#line` directives so the driver's own
// compile-error line numbers - and this module's own diagnostics parser - map back to the
// *originally authored* file and line, not the flattened blob actually handed to glCompileShader.
// An #include cycle is detected via a visited-file stack (never infinite-loops); a file already
// expanded once this compile is silently skipped on a second #include (a once-per-compile
// include-guard convenience, since GLSL has no native #pragma once).

#include "shader_preprocessor_uve.h"

#include <cctype>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render::Shader::Detail {

namespace {

struct ConditionFrameUVE {
    bool parentActive = true;
    bool branchActive = true; // true while the currently-taken branch (if or else) should emit
    [[nodiscard]] bool IsActiveUVE() const noexcept { return parentActive && branchActive; }
};

struct PreprocessContextUVE {
    Asset::IFileSystemUVE& fileSystem;
    std::unordered_map<std::string, std::string> definedFlags;
    std::vector<ConditionFrameUVE> conditionStack;
    std::vector<std::string> visitedStack;
    std::unordered_set<std::string> alreadyEmitted;
    std::vector<std::string>& fileIndexTable;
    std::vector<std::string>& dependencyClosure;
    bool hadError = false;
    std::string errorMessage;
};

[[nodiscard]] std::vector<std::string> SplitLinesUVE(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t newlinePos = text.find('\n', start);
        if (newlinePos == std::string::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, newlinePos - start));
        start = newlinePos + 1;
    }
    return lines;
}

[[nodiscard]] std::string_view TrimUVE(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

[[nodiscard]] bool IsAllActiveUVE(const std::vector<ConditionFrameUVE>& stack) {
    for (const ConditionFrameUVE& frame : stack) {
        if (!frame.IsActiveUVE()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool StartsWithUVE(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] std::string_view RemoveUtf8BomUVE(std::string_view text) noexcept {
    constexpr std::string_view utf8Bom{"\xEF\xBB\xBF"};
    if (text.size() >= utf8Bom.size() && text.substr(0, utf8Bom.size()) == utf8Bom) {
        return text.substr(utf8Bom.size());
    }
    return text;
}

[[nodiscard]] bool IsVersionDirectiveUVE(std::string_view text) noexcept {
    text = TrimUVE(RemoveUtf8BomUVE(text));
    if (!StartsWithUVE(text, "#version")) {
        return false;
    }
    return text.size() == 8U || std::isspace(static_cast<unsigned char>(text[8])) != 0;
}

/// Finds a leading GLSL #version line while allowing only blank lines and comments before it. This
/// keeps generated #line directives behind #version, as required by GLSL, even when authored files
/// begin with a UTF-8 BOM or a copyright/comment header.
[[nodiscard]] std::optional<std::size_t> FindLeadingVersionLineUVE(const std::vector<std::string>& lines) {
    bool insideBlockComment = false;
    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const std::string_view line = lineIndex == 0U ? RemoveUtf8BomUVE(lines[lineIndex]) : lines[lineIndex];
        std::size_t cursor = 0U;
        while (cursor < line.size()) {
            if (insideBlockComment) {
                const std::size_t commentEnd = line.find("*/", cursor);
                if (commentEnd == std::string_view::npos) {
                    cursor = line.size();
                    break;
                }
                cursor = commentEnd + 2U;
                insideBlockComment = false;
                continue;
            }
            while (cursor < line.size() && std::isspace(static_cast<unsigned char>(line[cursor])) != 0) {
                ++cursor;
            }
            if (cursor == line.size() ||
                (cursor + 1U < line.size() && line[cursor] == '/' && line[cursor + 1U] == '/')) {
                break;
            }
            if (cursor + 1U < line.size() && line[cursor] == '/' && line[cursor + 1U] == '*') {
                cursor += 2U;
                insideBlockComment = true;
                continue;
            }
            if (IsVersionDirectiveUVE(line.substr(cursor))) {
                return lineIndex;
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

#ifdef __ANDROID__
/// Matches any `#version <digits> core` line, not a single hardcoded number - this project's
/// desktop baseline has already moved once (330 -> 450, Phase 2a) and this Android translation
/// must not need a matching edit every time the desktop number changes again. It intentionally
/// does not distinguish *which* desktop version was requested: every desktop-authored built-in
/// and project shader in this engine sticks to GLSL syntax that is also valid under GLSL ES 3.00
/// (no 4.x-only language features are actually used yet - see the Phase 2a capability-gating note
/// on compute shaders in gl_render_device_uve.cpp), so any desktop `core` version line maps to the
/// same fixed "#version 300 es" target regardless of the specific number on it.
[[nodiscard]] bool IsDesktopCoreVersionUVE(std::string_view versionLine) noexcept {
    versionLine = TrimUVE(RemoveUtf8BomUVE(versionLine));
    if (!IsVersionDirectiveUVE(versionLine)) {
        return false;
    }
    std::string_view remainder = TrimUVE(versionLine.substr(8));
    std::size_t digitCount = 0U;
    while (digitCount < remainder.size() && std::isdigit(static_cast<unsigned char>(remainder[digitCount])) != 0) {
        ++digitCount;
    }
    if (digitCount == 0U ||
        (remainder.size() > digitCount && std::isspace(static_cast<unsigned char>(remainder[digitCount])) == 0)) {
        return false;
    }
    remainder = TrimUVE(remainder.substr(digitCount));
    if (!StartsWithUVE(remainder, "core") ||
        (remainder.size() > 4U && remainder[4] != '/' &&
         std::isspace(static_cast<unsigned char>(remainder[4])) == 0)) {
        return false;
    }
    remainder = TrimUVE(remainder.substr(4));
    return remainder.empty() || StartsWithUVE(remainder, "//") || StartsWithUVE(remainder, "/*");
}
#endif

/// Token-boundary-aware macro substitution: replaces every maximal `[A-Za-z_][A-Za-z0-9_]*`
/// identifier in `line` that exactly matches a key in `definedFlags` (with a non-empty value —
/// a flag-only #define has nothing to substitute) with its value.
[[nodiscard]] std::string ApplyDefinesUVE(const std::string& line,
                                           const std::unordered_map<std::string, std::string>& definedFlags) {
    std::string result;
    result.reserve(line.size());
    std::size_t index = 0;
    while (index < line.size()) {
        const unsigned char currentChar = static_cast<unsigned char>(line[index]);
        if (std::isalpha(currentChar) != 0 || currentChar == '_') {
            std::size_t tokenEnd = index + 1;
            while (tokenEnd < line.size() &&
                   (std::isalnum(static_cast<unsigned char>(line[tokenEnd])) != 0 || line[tokenEnd] == '_')) {
                ++tokenEnd;
            }
            const std::string token = line.substr(index, tokenEnd - index);
            const auto defineIt = definedFlags.find(token);
            if (defineIt != definedFlags.end() && !defineIt->second.empty()) {
                result += defineIt->second;
            } else {
                result += token;
            }
            index = tokenEnd;
        } else {
            result += line[index];
            ++index;
        }
    }
    return result;
}

[[nodiscard]] bool ExpandLinesUVE(const std::vector<std::string>& lines, std::uint32_t startLineNumber1Based,
                                   std::uint32_t fileIndex, PreprocessContextUVE& context, std::string& output);

[[nodiscard]] bool ExpandIncludeUVE(std::string_view directiveArgs, std::uint32_t parentFileIndex,
                                     std::uint32_t nextParentLineNumber1Based, PreprocessContextUVE& context,
                                     std::string& output) {
    const std::string_view trimmedArgs = TrimUVE(directiveArgs);
    if (trimmedArgs.size() < 2 || trimmedArgs.front() != '"' || trimmedArgs.back() != '"') {
        context.hadError = true;
        context.errorMessage = "malformed #include directive (expected #include \"virtual/path.glsl\")";
        return false;
    }
    const std::string childPath(trimmedArgs.substr(1, trimmedArgs.size() - 2));

    for (const std::string& visited : context.visitedStack) {
        if (visited == childPath) {
            std::string chain;
            for (const std::string& entry : context.visitedStack) {
                chain += entry;
                chain += " -> ";
            }
            chain += childPath;
            context.hadError = true;
            context.errorMessage = "#include cycle detected: " + chain;
            return false;
        }
    }

    if (context.alreadyEmitted.contains(childPath)) {
        return true; // once-per-compile include guard: silently skip, nothing to emit.
    }

    const std::optional<std::vector<std::byte>> childBytes = context.fileSystem.ReadFileUVE(childPath);
    if (!childBytes.has_value()) {
        context.hadError = true;
        context.errorMessage = "#include target not found: \"" + childPath + "\"";
        return false;
    }

    const auto childIndex = static_cast<std::uint32_t>(context.fileIndexTable.size());
    context.fileIndexTable.push_back(childPath);
    context.dependencyClosure.push_back(childPath);
    context.alreadyEmitted.insert(childPath);
    context.visitedStack.push_back(childPath);

    const std::string childContent(reinterpret_cast<const char*>(childBytes->data()), childBytes->size());
    const std::vector<std::string> childLines = SplitLinesUVE(childContent);

    output += "#line 1 " + std::to_string(childIndex) + "\n";
    const bool childOk = ExpandLinesUVE(childLines, 1, childIndex, context, output);
    output += "#line " + std::to_string(nextParentLineNumber1Based) + " " + std::to_string(parentFileIndex) + "\n";

    context.visitedStack.pop_back();
    return childOk;
}

bool ExpandLinesUVE(const std::vector<std::string>& lines, std::uint32_t startLineNumber1Based,
                     std::uint32_t fileIndex, PreprocessContextUVE& context, std::string& output) {
    for (std::size_t lineOffset = 0; lineOffset < lines.size(); ++lineOffset) {
        if (context.hadError) {
            return false;
        }
        const std::string& rawLine = lines[lineOffset];
        const std::uint32_t originalLineNumber1Based = startLineNumber1Based + static_cast<std::uint32_t>(lineOffset);
        const std::string_view trimmed = TrimUVE(rawLine);

        if (StartsWithUVE(trimmed, "#ifdef ") || StartsWithUVE(trimmed, "#ifndef ")) {
            const bool isPositive = StartsWithUVE(trimmed, "#ifdef ");
            const std::string name(TrimUVE(trimmed.substr(isPositive ? 7 : 8)));
            const bool defined = context.definedFlags.contains(name);
            ConditionFrameUVE frame;
            frame.parentActive = IsAllActiveUVE(context.conditionStack);
            frame.branchActive = isPositive ? defined : !defined;
            context.conditionStack.push_back(frame);
            continue;
        }
        if (trimmed == "#else") {
            if (context.conditionStack.empty()) {
                context.hadError = true;
                context.errorMessage = "unmatched #else";
                return false;
            }
            context.conditionStack.back().branchActive = !context.conditionStack.back().branchActive;
            continue;
        }
        if (trimmed == "#endif") {
            if (context.conditionStack.empty()) {
                context.hadError = true;
                context.errorMessage = "unmatched #endif";
                return false;
            }
            context.conditionStack.pop_back();
            continue;
        }

        if (!IsAllActiveUVE(context.conditionStack)) {
            continue; // Inside an inactive branch - skip everything except the directives above.
        }

        if (StartsWithUVE(trimmed, "#define ")) {
            const std::string_view rest = TrimUVE(trimmed.substr(8));
            const std::size_t spacePos = rest.find(' ');
            if (spacePos == std::string_view::npos) {
                context.definedFlags[std::string(rest)] = "";
            } else {
                const std::string name(rest.substr(0, spacePos));
                const std::string value(TrimUVE(rest.substr(spacePos + 1)));
                context.definedFlags[name] = value;
            }
            continue;
        }
        if (StartsWithUVE(trimmed, "#undef ")) {
            const std::string name(TrimUVE(trimmed.substr(7)));
            context.definedFlags.erase(name);
            continue;
        }
        if (StartsWithUVE(trimmed, "#include ") || StartsWithUVE(trimmed, "#include\t")) {
            if (!ExpandIncludeUVE(trimmed.substr(8), fileIndex, originalLineNumber1Based + 1, context, output)) {
                return false;
            }
            continue;
        }
        if (StartsWithUVE(trimmed, "#version") || StartsWithUVE(trimmed, "#extension") ||
            StartsWithUVE(trimmed, "#line")) {
            output += rawLine;
            output += "\n";
            continue;
        }

        output += ApplyDefinesUVE(rawLine, context.definedFlags);
        output += "\n";
    }
    return true;
}

} // namespace

PreprocessResultUVE PreprocessShaderSourceUVE(Asset::IFileSystemUVE& fileSystem, const std::string& virtualFilePath,
                                               const std::string& embeddedFallbackSourceCode,
                                               const std::vector<std::pair<std::string, std::string>>& defines) {
    PreprocessResultUVE result;

    std::string rootContent;
    std::string rootIdentifier;
    const bool useVirtualFile = !virtualFilePath.empty() && fileSystem.HasFileUVE(virtualFilePath);
    if (useVirtualFile) {
        const std::optional<std::vector<std::byte>> bytes = fileSystem.ReadFileUVE(virtualFilePath);
        if (!bytes.has_value()) {
            result.errorMessage = "failed to read \"" + virtualFilePath + "\" despite HasFileUVE() reporting true";
            return result;
        }
        rootContent.assign(reinterpret_cast<const char*>(bytes->data()), bytes->size());
        rootIdentifier = virtualFilePath;
    } else {
        rootContent = embeddedFallbackSourceCode;
        rootIdentifier = virtualFilePath.empty() ? "<embedded>" : virtualFilePath + " (embedded fallback)";
    }

    result.fileIndexTable.push_back(rootIdentifier);
    if (useVirtualFile) {
        result.dependencyClosure.push_back(virtualFilePath);
    }

    PreprocessContextUVE context{
        fileSystem, {}, {}, {}, {}, result.fileIndexTable, result.dependencyClosure, false, {}};
    for (const auto& [name, value] : defines) {
        context.definedFlags[name] = value;
    }

    const std::vector<std::string> lines = SplitLinesUVE(rootContent);
    std::string output;

    std::size_t firstContentLineIndex = 0;
    const std::optional<std::size_t> versionLineIndex = FindLeadingVersionLineUVE(lines);
    if (versionLineIndex.has_value()) {
        for (std::size_t lineIndex = 0; lineIndex < *versionLineIndex; ++lineIndex) {
            output += lineIndex == 0U ? RemoveUtf8BomUVE(lines[lineIndex]) : std::string_view(lines[lineIndex]);
            output += "\n";
        }
#ifdef __ANDROID__
        // Android's ES 3.0 context cannot compile desktop GLSL 3.30. Keep this adaptation at the
        // backend boundary so embedded and project-authored shaders share the same GLES handling,
        // including version lines with a trailing comment.
        if (IsDesktopCoreVersionUVE(lines[*versionLineIndex])) {
            output += "#version 300 es\n";
            output += "precision highp float;\n";
            output += "precision highp int;\n";
        } else {
            output += RemoveUtf8BomUVE(lines[*versionLineIndex]);
            output += "\n";
        }
#else
        output += RemoveUtf8BomUVE(lines[*versionLineIndex]);
        output += "\n";
#endif
        firstContentLineIndex = *versionLineIndex + 1U;
    }
    output += "#line " + std::to_string(firstContentLineIndex + 1) + " 0\n";

    const std::vector<std::string> remainingLines(lines.begin() + static_cast<std::ptrdiff_t>(firstContentLineIndex),
                                                    lines.end());
    const bool ok =
        ExpandLinesUVE(remainingLines, static_cast<std::uint32_t>(firstContentLineIndex + 1), 0, context, output);

    if (!ok || context.hadError) {
        result.success = false;
        result.errorMessage = context.errorMessage;
        UVE_ERROR("ShaderManagerUVE: preprocessing \"{}\" failed: {}", rootIdentifier, result.errorMessage);
        return result;
    }

    result.success = true;
    result.resolvedSource = std::move(output);
    return result;
}

std::uint64_t ComputeFnv1aHashUVE(std::string_view data) noexcept {
    constexpr std::uint64_t kOffsetBasis = 0xcbf29ce484222325ULL;
    constexpr std::uint64_t kPrime = 0x100000001b3ULL;
    std::uint64_t hash = kOffsetBasis;
    for (const char character : data) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(character));
        hash *= kPrime;
    }
    return hash;
}

} // namespace UVE::Render::Shader::Detail
