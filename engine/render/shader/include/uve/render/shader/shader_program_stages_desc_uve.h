// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/shader/shader_source_compile_desc_uve.h"

namespace UVE::Render::Shader {

/// Describes a managed linked vertex+fragment program whose stages originate from separate source
/// descriptors. This preserves MaterialAssetUVE's existing independent vertexShader/fragmentShader
/// asset GUID contract while reusing ShaderManagerUVE's async preprocessing, diagnostics, program
/// cache, and hot-reload lifecycle. The manager normalizes vertexSource.stage/fragmentSource.stage
/// to the corresponding graphics stages when the request is created; callers should still provide
/// those values for readable diagnostics. Both stages retain their own virtual path, embedded
/// fallback, defines, entry point, and dependency closure.
struct ShaderProgramStagesDescUVE {
    ShaderSourceCompileDescUVE vertexSource;
    ShaderSourceCompileDescUVE fragmentSource;

    std::vector<VertexAttributeUVE> vertexLayout;
    std::uint32_t vertexStride = 0;
    PrimitiveTopologyUVE topology = PrimitiveTopologyUVE::Triangles;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;

    /// Enables program-level dependency tracking over the union of both stage closures. Individual
    /// source flags are honored only for source-only compilation requests; this program-level flag
    /// owns reload behavior for linked programs.
    bool hotReloadEnabledUVE = true;
    std::string debugNameUVE;
};

} // namespace UVE::Render::Shader
