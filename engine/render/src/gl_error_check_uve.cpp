// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "gl_error_check_uve.h"

namespace UVE::Render::Detail {

namespace {

#if UVE_DEBUG
[[nodiscard]] const char* GlDebugSourceNameUVE(const GLenum source) noexcept {
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WindowSystem";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "ShaderCompiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "ThirdParty";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "Application";
        case GL_DEBUG_SOURCE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

[[nodiscard]] const char* GlDebugTypeNameUVE(const GLenum type) noexcept {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DeprecatedBehavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UndefinedBehavior";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "Performance";
        case GL_DEBUG_TYPE_MARKER:
            return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "PushGroup";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "PopGroup";
        case GL_DEBUG_TYPE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

void APIENTRY GlDebugMessageCallbackUVE(const GLenum source, const GLenum type, const GLuint id,
                                         const GLenum severity, const GLsizei length, const GLchar* const message,
                                         const void* const userParam) noexcept {
    static_cast<void>(length);
    static_cast<void>(userParam);
    // GL_DEBUG_SEVERITY_NOTIFICATION is routine driver chatter (e.g. "buffer will use video
    // memory"), not a code-side problem to act on - logged at Info so it stays visible without
    // being mistaken for something broken. HIGH/MEDIUM are genuine runtime API misuse; LOW is
    // typically a real but minor issue (redundant state changes, non-fatal deprecated usage).
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        case GL_DEBUG_SEVERITY_MEDIUM:
            UVE_ERROR("GlRenderDeviceUVE: [{}/{}] GL debug message {}: {}", GlDebugSourceNameUVE(source),
                      GlDebugTypeNameUVE(type), id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            UVE_WARNING("GlRenderDeviceUVE: [{}/{}] GL debug message {}: {}", GlDebugSourceNameUVE(source),
                        GlDebugTypeNameUVE(type), id, message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        default:
            UVE_INFO("GlRenderDeviceUVE: [{}/{}] GL debug message {}: {}", GlDebugSourceNameUVE(source),
                     GlDebugTypeNameUVE(type), id, message);
            break;
    }
}
#endif

} // namespace

void RegisterGlDebugCallbackUVE(const GlFunctionsUVE& functions) noexcept {
#if UVE_DEBUG
    if (functions.glDebugMessageCallback == nullptr) {
        UVE_INFO("GlRenderDeviceUVE: GL_KHR_debug unavailable on this context; relying on "
                 "UVE_GL_CHECK_ERROR_UVE()'s manual glGetError() polling instead");
        return;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    functions.glDebugMessageCallback(GlDebugMessageCallbackUVE, nullptr);
    UVE_INFO("GlRenderDeviceUVE: GL_KHR_debug callback registered");
#else
    static_cast<void>(functions);
#endif
}

} // namespace UVE::Render::Detail
