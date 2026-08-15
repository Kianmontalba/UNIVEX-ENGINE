// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "uve/render/render_resource_descs_uve.h"

namespace UVE::Render::Shader {

/// Describes a linked vertex+fragment program to create via IShaderManagerUVE::CreateProgramUVE().
/// Both stages come from the *same* resolved source (see BuiltIn's single-file-per-program
/// convention) — ShaderManagerUVE compiles it twice, injecting VERTEX_SHADER/FRAGMENT_SHADER
/// respectively (see Detail::ApplyPreprocessorUVE()) so each stage's `#ifdef` block is the only
/// one active. `virtualFilePath`/`embeddedFallbackSourceCode` follow the same automatic-fallback
/// contract as ShaderSourceCompileDescUVE.
struct ShaderProgramDescUVE {
    std::string virtualFilePath;
    std::string embeddedFallbackSourceCode;
    std::vector<std::pair<std::string, std::string>> extraDefines;
    std::string entryPointName = "main";

    std::vector<VertexAttributeUVE> vertexLayout;
    std::uint32_t vertexStride = 0;
    PrimitiveTopologyUVE topology = PrimitiveTopologyUVE::Triangles;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    PipelineBlendModeUVE blendMode = PipelineBlendModeUVE::Opaque;

    bool hotReloadEnabledUVE = true;
    std::string debugNameUVE;
};

} // namespace UVE::Render::Shader
