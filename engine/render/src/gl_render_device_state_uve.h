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
#include <unordered_map>
#include <vector>

#include "gl_functions_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/window/i_window_manager_uve.h"

namespace UVE::Render::Detail {

/// The GL resource maps and function table shared between GlRenderDeviceUVE and
/// GlCommandBufferUVE (module-private — never under include/). GlRenderDeviceUVE owns exactly one
/// of these; every GlCommandBufferUVE it hands out (via CreateCommandBufferUVE()) holds a
/// reference to it for the lifetime of one frame's recording, resolving UVE resource handles
/// (which are opaque synthetic uint32_t counters, exactly like NullRenderDeviceUVE's own handles)
/// to the real GL object names they map to.
struct GlDeviceStateUVE {
    Window::IWindowManagerUVE* windowManager;
    GlFunctionsUVE gl;

    struct BufferRecordUVE {
        GLuint glBuffer = 0;
        GLenum target = 0;
        std::uint64_t sizeBytes = 0;
    };
    std::unordered_map<std::uint32_t, BufferRecordUVE> buffers;
    std::uint32_t nextBufferHandle = 1;

    struct TextureRecordUVE {
        GLuint glTexture = 0;
        TextureDescUVE desc;
    };
    std::unordered_map<std::uint32_t, TextureRecordUVE> textures;
    std::uint32_t nextTextureHandle = 1;

    struct ShaderRecordUVE {
        GLuint glShader = 0;
    };
    std::unordered_map<std::uint32_t, ShaderRecordUVE> shaders;
    std::uint32_t nextShaderHandle = 1;

    struct PipelineRecordUVE {
        GLuint glProgram = 0;
        GLuint glVao = 0; // One VAO per pipeline; vertex attrib pointers are configured against
                           // it lazily, when BindVertexBufferUVE() runs (GL ties
                           // glVertexAttribPointer to whichever buffer is bound at the time it's
                           // called, which this RHI's Bind-then-Draw command buffer flow only
                           // knows once BindVertexBufferUVE() actually executes).
        std::vector<VertexAttributeUVE> vertexLayout;
        std::uint32_t vertexStride = 0;
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
    };
    std::unordered_map<std::uint32_t, PipelineRecordUVE> pipelines;
    std::uint32_t nextPipelineHandle = 1;
};

} // namespace UVE::Render::Detail
