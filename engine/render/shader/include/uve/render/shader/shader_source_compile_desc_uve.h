// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>
#include <utility>
#include <vector>

#include "uve/render/render_resource_descs_uve.h"

namespace UVE::Render::Shader {

/// Describes one shader stage to compile via IShaderManagerUVE::CreateSourceUVE().
/// `virtualFilePath` and `embeddedFallbackSourceCode` are both always provided (not an
/// either/or): ShaderManagerUVE tries the virtual file first (enabling hot-reload tracking) and
/// transparently falls back to the embedded string if IFileSystemUVE::HasFileUVE() is false for
/// `virtualFilePath` — see BuiltIn::kBasic3DSource for the convention every built-in follows.
struct ShaderSourceCompileDescUVE {
    ShaderStageUVE stage = ShaderStageUVE::Vertex;
    std::string virtualFilePath;
    std::string embeddedFallbackSourceCode;

    /// Extra `#define NAME VALUE` pairs injected after the engine's own baseline block
    /// (UVE_DEBUG/UVE_MOBILE/UVE_BACKEND_GL) — see Detail::ApplyPreprocessorUVE()'s doc comment.
    std::vector<std::pair<std::string, std::string>> extraDefines;

    std::string entryPointName = "main";

    /// Whether ShaderManagerUVE::UpdateUVE() polls this source's #include dependency closure for
    /// on-disk changes and recompiles it automatically. Ignored (never tracked) when resolved via
    /// `embeddedFallbackSourceCode` — there is no file to poll.
    bool hotReloadEnabledUVE = true;

    /// Purely for log/diagnostic messages — never affects compilation.
    std::string debugNameUVE;
};

} // namespace UVE::Render::Shader
