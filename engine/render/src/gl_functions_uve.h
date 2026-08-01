//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

// GL/glext.h supplies the standard Khronos PFNGL*PROC function-pointer typedefs and GL_* enum
// constants this loader needs; it declares types only, never links anything, so including it
// here doesn't pull in GLEW/GLAD or any other loader library. Module-private (engine/render/src/
// only) — no public header under include/ ever includes a GL header, matching this codebase's
// established third-party-header confinement discipline.
#include <GL/gl.h>
#include <GL/glext.h>

namespace UVE::Render::Detail {

/// The GL function pointers GlRenderDeviceUVE needs beyond what the system's legacy OpenGL 1.1
/// <GL/gl.h> already declares directly and requires no loading for (glViewport, glClear,
/// glClearColor, glDrawArrays, glGetError, glGetIntegerv, glEnable/glDisable, ...). This is a
/// genuinely minimal, hand-rolled loader — exactly the ~20 buffer/VAO/shader/program functions
/// this increment's triangle needs — rather than a GLEW/GLAD dependency, matching this codebase's
/// "only build what the current consumer needs" discipline (see docs/CODING_STANDARDS.md,
/// Matrix4x4UVE's minimal API for the established precedent).
struct GlFunctionsUVE {
    PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC glBufferData = nullptr;
    PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
    PFNGLBINDBUFFERBASEPROC glBindBufferBase = nullptr;

    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;

    PFNGLCREATESHADERPROC glCreateShader = nullptr;
    PFNGLDELETESHADERPROC glDeleteShader = nullptr;
    PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
    PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;

    PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
    PFNGLATTACHSHADERPROC glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC glUseProgram = nullptr;

    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;

    PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

    /// True iff every function pointer above loaded successfully (non-null).
    [[nodiscard]] bool IsCompleteUVE() const noexcept;
};

/// Loads every GlFunctionsUVE member by calling `getProcAddress(name)` once per function and
/// reinterpret_cast-ing the result to the matching PFNGL*PROC type — the universal pattern every
/// GL loader (hand-rolled or generated) uses, since a GL driver only exposes post-1.1 entry
/// points through this kind of dynamic lookup. `getProcAddress` is injected rather than calling
/// glfwGetProcAddress directly so this loader itself never includes GLFW; the one caller
/// (GlRenderDeviceUVE, constructed only after WindowManagerUVE has already made a GL context
/// current) passes a small wrapper around glfwGetProcAddress.
[[nodiscard]] GlFunctionsUVE LoadGlFunctionsUVE(void* (*getProcAddress)(const char*));

} // namespace UVE::Render::Detail
