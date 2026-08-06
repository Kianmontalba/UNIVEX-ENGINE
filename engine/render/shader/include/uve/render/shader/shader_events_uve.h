// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>

#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/shader/shader_compile_diagnostics_uve.h"

namespace UVE::Render::Shader {

/// Queued (never Published — may originate from ShaderManagerUVE::UpdateUVE()'s main-thread
/// hot-reload check, always delivered via IEventSystemUVE::QueueEvent) whenever a hot-reload
/// recompile finishes, success or failure — mirrors Asset::AssetReloadedEventUVE's role.
struct ShaderProgramReloadedEventUVE {
    std::string debugName;
    bool success = false;
};

/// Queued alongside ShaderProgramReloadedEventUVE{success=false} with the full structured
/// diagnostics, for callers (e.g. an editor console) that want the detail without re-querying
/// GetDiagnosticsUVE() themselves.
struct ShaderCompileFailedEventUVE {
    std::string debugName;
    ShaderStageUVE stage = ShaderStageUVE::Vertex;
    ShaderCompileDiagnosticsUVE diagnostics;
};

} // namespace UVE::Render::Shader
