// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

// See gl_functions_uve.h's own header comment for why these are the correct includes for both
// platforms and why including them here does not pull in a loader library.
#if defined(__ANDROID__)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <string_view>

#include "gl_functions_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Render::Detail {

/// Registers `GlDebugMessageCallbackUVE` as the driver's `GL_KHR_debug` callback, if the function
/// pointer loaded (desktop GL 4.3+ and most GLES 3.2+ drivers; this engine's Android baseline is a
/// fixed GLES 3.0 context, so `functions.glDebugMessageCallback` is expected to be null there -
/// see UVE_GL_CHECK_ERROR_UVE()'s doc comment for the fallback that covers that gap). A no-op
/// outside debug builds and when the pointer failed to load. Call once, right after the GL context
/// is confirmed usable (`GlFunctionsUVE::IsCompleteUVE()`), same as this engine's other one-time
/// per-context setup (see the max-limits `glGetIntegerv` calls in GlRenderDeviceUVE's constructor).
void RegisterGlDebugCallbackUVE(const GlFunctionsUVE& functions) noexcept;

} // namespace UVE::Render::Detail

/// Debug builds (UVE_DEBUG == 1): drains every currently-flagged GL error via a `glGetError()`
/// loop (a single query can mask more than one accumulated error - glGetError()'s own contract is
/// "return one flagged error and clear it", so this keeps calling it until GL_NO_ERROR) and logs
/// each one via UVE_ERROR, tagged with `label` so the log names which call site raised it.
///
/// This is specifically the fallback path RegisterGlDebugCallbackUVE()'s doc comment refers to:
/// GL_KHR_debug's callback is the primary, comprehensive mechanism this engine relies on when it's
/// available, but this engine's Android target is a fixed GLES 3.0 context, which does not
/// guarantee that extension - polling glGetError() manually at a handful of call sites already
/// known to have zero error checking (CreateBufferUVE/CreateTextureUVE below; shader
/// compile/link failures are already surfaced through their own info-log checks, a genuinely
/// different and already-correct case) is what actually catches runtime GL API misuse there.
/// Running it unconditionally in debug builds even where the callback IS also registered is
/// intentional, not redundant by mistake: the two mechanisms query independent GL state (the
/// callback and the implicit error-flag queue coexist without interfering), so this is simply
/// defense in depth on desktop, at zero cost in release builds either way.
///
/// Release builds (UVE_DEBUG == 0): expands to a warning-clean no-op, matching UVE_ASSERT's own
/// call-site contract (see assert_uve.h) - never queries glGetError() in release, so this carries
/// no per-call overhead there.
#if UVE_DEBUG
#define UVE_GL_CHECK_ERROR_UVE(label) ::UVE::Render::Detail::DrainGlErrorsUVE(label)
#else
#define UVE_GL_CHECK_ERROR_UVE(label) \
    do {                              \
    } while (false)
#endif

namespace UVE::Render::Detail {

#if UVE_DEBUG
inline void DrainGlErrorsUVE(const std::string_view label) noexcept {
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        const char* errorName = "GL_UNKNOWN_ERROR";
        switch (error) {
            case GL_INVALID_ENUM:
                errorName = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorName = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorName = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorName = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorName = "GL_OUT_OF_MEMORY";
                break;
            default:
                break;
        }
        UVE_ERROR("GlRenderDeviceUVE: {} raised {} (0x{:04X})", label, errorName, static_cast<unsigned int>(error));
    }
}
#endif

} // namespace UVE::Render::Detail
