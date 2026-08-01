//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/shader_data_type_uve.h"

#include "uve/debug/assert_uve.h"

namespace UVE::Render {

std::size_t GetShaderDataTypeSizeBytesUVE(ShaderDataTypeUVE type) noexcept {
    switch (type) {
        case ShaderDataTypeUVE::Float:
            return 4U;
        case ShaderDataTypeUVE::Vec2:
            return 8U;
        case ShaderDataTypeUVE::Vec3:
            return 12U;
        case ShaderDataTypeUVE::Vec4:
            return 16U;
        case ShaderDataTypeUVE::Mat3:
            return 36U;
        case ShaderDataTypeUVE::Mat4:
            return 64U;
        case ShaderDataTypeUVE::Int:
            return 4U;
        case ShaderDataTypeUVE::Bool:
            return 4U;
    }
    UVE_ASSERT(false && "Unhandled ShaderDataTypeUVE");
    return 0U;
}

} // namespace UVE::Render
