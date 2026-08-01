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

#include "uve/render/shader_data_type_uve.h"

namespace UVE::Render {

/// One uniform IRenderDeviceUVE::GetPipelineUniformsUVE() discovered by reflecting a linked
/// pipeline. `location` is a backend-internal identifier (a raw GL uniform location for
/// GlRenderDeviceUVE) that callers never need to interpret themselves — it exists purely so a
/// backend can look one up quickly from a name without leaking any GL type; a NullRenderDeviceUVE
/// pipeline never populates this (see GetPipelineUniformsUVE()'s own doc comment).
struct UniformReflectionUVE {
    std::string name;
    ShaderDataTypeUVE type = ShaderDataTypeUVE::Float;
    std::int32_t location = -1;
    std::uint32_t arraySize = 1;
};

} // namespace UVE::Render
