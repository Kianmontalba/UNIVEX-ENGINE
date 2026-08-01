//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "gl_functions_uve.h"

namespace UVE::Render::Detail {

namespace {

template <typename TFunctionPointer>
[[nodiscard]] TFunctionPointer LoadOneUVE(void* (*getProcAddress)(const char*), const char* name) {
    return reinterpret_cast<TFunctionPointer>(getProcAddress(name));
}

} // namespace

bool GlFunctionsUVE::IsCompleteUVE() const noexcept {
    return glGenBuffers != nullptr && glDeleteBuffers != nullptr && glBindBuffer != nullptr &&
           glBufferData != nullptr && glBufferSubData != nullptr && glBindBufferBase != nullptr &&
           glGenVertexArrays != nullptr && glDeleteVertexArrays != nullptr && glBindVertexArray != nullptr &&
           glVertexAttribPointer != nullptr && glEnableVertexAttribArray != nullptr && glCreateShader != nullptr &&
           glDeleteShader != nullptr && glShaderSource != nullptr && glCompileShader != nullptr &&
           glGetShaderiv != nullptr && glGetShaderInfoLog != nullptr && glCreateProgram != nullptr &&
           glDeleteProgram != nullptr && glAttachShader != nullptr && glLinkProgram != nullptr &&
           glGetProgramiv != nullptr && glGetProgramInfoLog != nullptr && glUseProgram != nullptr &&
           glGenFramebuffers != nullptr && glDeleteFramebuffers != nullptr && glBindFramebuffer != nullptr &&
           glFramebufferTexture2D != nullptr && glCheckFramebufferStatus != nullptr && glActiveTexture != nullptr;
}

GlFunctionsUVE LoadGlFunctionsUVE(void* (*getProcAddress)(const char*)) {
    GlFunctionsUVE functions;

    functions.glGenBuffers = LoadOneUVE<PFNGLGENBUFFERSPROC>(getProcAddress, "glGenBuffers");
    functions.glDeleteBuffers = LoadOneUVE<PFNGLDELETEBUFFERSPROC>(getProcAddress, "glDeleteBuffers");
    functions.glBindBuffer = LoadOneUVE<PFNGLBINDBUFFERPROC>(getProcAddress, "glBindBuffer");
    functions.glBufferData = LoadOneUVE<PFNGLBUFFERDATAPROC>(getProcAddress, "glBufferData");
    functions.glBufferSubData = LoadOneUVE<PFNGLBUFFERSUBDATAPROC>(getProcAddress, "glBufferSubData");
    functions.glBindBufferBase = LoadOneUVE<PFNGLBINDBUFFERBASEPROC>(getProcAddress, "glBindBufferBase");

    functions.glGenVertexArrays = LoadOneUVE<PFNGLGENVERTEXARRAYSPROC>(getProcAddress, "glGenVertexArrays");
    functions.glDeleteVertexArrays = LoadOneUVE<PFNGLDELETEVERTEXARRAYSPROC>(getProcAddress, "glDeleteVertexArrays");
    functions.glBindVertexArray = LoadOneUVE<PFNGLBINDVERTEXARRAYPROC>(getProcAddress, "glBindVertexArray");
    functions.glVertexAttribPointer =
        LoadOneUVE<PFNGLVERTEXATTRIBPOINTERPROC>(getProcAddress, "glVertexAttribPointer");
    functions.glEnableVertexAttribArray =
        LoadOneUVE<PFNGLENABLEVERTEXATTRIBARRAYPROC>(getProcAddress, "glEnableVertexAttribArray");

    functions.glCreateShader = LoadOneUVE<PFNGLCREATESHADERPROC>(getProcAddress, "glCreateShader");
    functions.glDeleteShader = LoadOneUVE<PFNGLDELETESHADERPROC>(getProcAddress, "glDeleteShader");
    functions.glShaderSource = LoadOneUVE<PFNGLSHADERSOURCEPROC>(getProcAddress, "glShaderSource");
    functions.glCompileShader = LoadOneUVE<PFNGLCOMPILESHADERPROC>(getProcAddress, "glCompileShader");
    functions.glGetShaderiv = LoadOneUVE<PFNGLGETSHADERIVPROC>(getProcAddress, "glGetShaderiv");
    functions.glGetShaderInfoLog = LoadOneUVE<PFNGLGETSHADERINFOLOGPROC>(getProcAddress, "glGetShaderInfoLog");

    functions.glCreateProgram = LoadOneUVE<PFNGLCREATEPROGRAMPROC>(getProcAddress, "glCreateProgram");
    functions.glDeleteProgram = LoadOneUVE<PFNGLDELETEPROGRAMPROC>(getProcAddress, "glDeleteProgram");
    functions.glAttachShader = LoadOneUVE<PFNGLATTACHSHADERPROC>(getProcAddress, "glAttachShader");
    functions.glLinkProgram = LoadOneUVE<PFNGLLINKPROGRAMPROC>(getProcAddress, "glLinkProgram");
    functions.glGetProgramiv = LoadOneUVE<PFNGLGETPROGRAMIVPROC>(getProcAddress, "glGetProgramiv");
    functions.glGetProgramInfoLog = LoadOneUVE<PFNGLGETPROGRAMINFOLOGPROC>(getProcAddress, "glGetProgramInfoLog");
    functions.glUseProgram = LoadOneUVE<PFNGLUSEPROGRAMPROC>(getProcAddress, "glUseProgram");

    functions.glGenFramebuffers = LoadOneUVE<PFNGLGENFRAMEBUFFERSPROC>(getProcAddress, "glGenFramebuffers");
    functions.glDeleteFramebuffers = LoadOneUVE<PFNGLDELETEFRAMEBUFFERSPROC>(getProcAddress, "glDeleteFramebuffers");
    functions.glBindFramebuffer = LoadOneUVE<PFNGLBINDFRAMEBUFFERPROC>(getProcAddress, "glBindFramebuffer");
    functions.glFramebufferTexture2D =
        LoadOneUVE<PFNGLFRAMEBUFFERTEXTURE2DPROC>(getProcAddress, "glFramebufferTexture2D");
    functions.glCheckFramebufferStatus =
        LoadOneUVE<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(getProcAddress, "glCheckFramebufferStatus");

    functions.glActiveTexture = LoadOneUVE<PFNGLACTIVETEXTUREPROC>(getProcAddress, "glActiveTexture");

    return functions;
}

} // namespace UVE::Render::Detail
