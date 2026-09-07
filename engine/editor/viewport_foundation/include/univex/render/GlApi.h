// univex/render/GlApi.h
// -----------------------------------------------------------------------
// Single include point for the OpenGL function loader, so swapping loaders
// is a one-file change instead of an edit in every translation unit.
//
// Defaults to GLEW because it is what this module was built and tested
// against. If UNIVEX already initialises GLAD (or its own loader), define
// UNIVEX_GL_LOADER_HEADER to that header at build time:
//
//     target_compile_definitions(univex_viewport_gl PUBLIC
//         UNIVEX_GL_LOADER_HEADER="glad/gl.h")
//
// Nothing else in this module includes a GL header directly.
// -----------------------------------------------------------------------
#pragma once

#if defined(UNIVEX_GL_LOADER_HEADER)
#include UNIVEX_GL_LOADER_HEADER
#else
#include <GL/glew.h>
#endif
