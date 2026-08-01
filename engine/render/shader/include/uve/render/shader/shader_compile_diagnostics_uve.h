//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Render::Shader {

/// One structured diagnostic recovered from a GL compile/link info log by
/// Detail::ParseGlInfoLogUVE() (engine/render/shader/src/shader_diagnostics_parser_uve.cpp,
/// module-private). `filePath` and `lineNumber` refer to the originally authored source file the
/// #include-expansion's `#line` directives re-based the driver's own line numbers back to — see
/// Detail::ExpandIncludesUVE()'s doc comment. A diagnostic line the parser couldn't match against
/// any known format still produces one of these (with `lineNumber == 0` and the raw text in
/// `message`) rather than being dropped.
struct ShaderCompileErrorUVE {
    std::string filePath;
    std::uint32_t lineNumber = 0;
    std::uint32_t columnNumber = 0;
    std::string message;
    std::string rawLogLine;
    bool isWarning = false;
};

/// The full result of one ShaderSourceUVE/ShaderProgramUVE compile or link attempt.
struct ShaderCompileDiagnosticsUVE {
    bool success = false;
    std::vector<ShaderCompileErrorUVE> diagnostics;
    std::string rawInfoLog;
};

} // namespace UVE::Render::Shader
