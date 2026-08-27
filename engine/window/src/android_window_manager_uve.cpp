#include "uve/window/android_window_manager_uve.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/log.h>

#include <utility>

#include "uve/events/i_event_system_uve.h"
#include "uve/window/window_desc_validation_uve.h"

namespace UVE::Window {

namespace {
constexpr char kLogTag[] = "UVEAndroidWindow";

void LogEglError(const char* const operation) noexcept {
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s failed (EGL error 0x%04x)", operation,
                        static_cast<unsigned int>(eglGetError()));
}
} // namespace

struct AndroidWindowManagerUVE::ImplUVE final {
    void* nativeWindow = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config = nullptr;
    EGLint nativeVisualId = 0;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    bool valid = false;
    bool recoveryFailureLogged = false;
    bool firstSwapLogged = false;
    bool vsyncEnabled = true;
    bool fullscreen = true;
    bool closeRequested = false;
};

AndroidWindowManagerUVE::AndroidWindowManagerUVE(Events::IEventSystemUVE& eventSystem, void* nativeWindow,
                                                 const WindowDescUVE& description)
    : m_impl(std::make_unique<ImplUVE>()) {
    static_cast<void>(eventSystem);
    m_impl->nativeWindow = nativeWindow;
    m_impl->vsyncEnabled = description.vsyncEnabled;

    if (!ValidateWindowDescUVE(description) || nativeWindow == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", "invalid Android window description or handle");
        return;
    }

    ANativeWindow* const androidWindow = static_cast<ANativeWindow*>(nativeWindow);
    m_impl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_impl->display == EGL_NO_DISPLAY) {
        LogEglError("eglGetDisplay");
        return;
    }

    EGLint majorVersion = 0;
    EGLint minorVersion = 0;
    if (eglInitialize(m_impl->display, &majorVersion, &minorVersion) == EGL_FALSE) {
        LogEglError("eglInitialize");
        return;
    }
    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        LogEglError("eglBindAPI");
        return;
    }

    const EGLint strictConfigAttributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE,
    };
    const EGLint lowDeviceConfigAttributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE,
    };
    EGLConfig config = nullptr;
    EGLint configCount = 0;
    if (eglChooseConfig(m_impl->display, strictConfigAttributes, &config, 1, &configCount) == EGL_FALSE ||
        configCount == 0) {
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "%s", "strict GLES3 depth/stencil config unavailable; retrying low-device config");
        configCount = 0;
        if (eglChooseConfig(m_impl->display, lowDeviceConfigAttributes, &config, 1, &configCount) == EGL_FALSE ||
            configCount == 0) {
            LogEglError("eglChooseConfig GLES3 low-device fallback");
            return;
        }
    }

    if (eglGetConfigAttrib(m_impl->display, config, EGL_NATIVE_VISUAL_ID, &nativeVisualId) == EGL_FALSE ||
        nativeVisualId == 0) {
        LogEglError("eglGetConfigAttrib EGL_NATIVE_VISUAL_ID");
        return;
    }
    // Android's BufferQueue can reject or silently mishandle a surface whose producer format does
    // not match the EGLConfig selected above. Explicitly negotiate the config's native visual
    // before creating the EGL window surface; this is especially important on vendor GLES stacks.
    const int geometryResult = ANativeWindow_setBuffersGeometry(androidWindow, 0, 0, nativeVisualId);
    if (geometryResult != 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "ANativeWindow_setBuffersGeometry failed (%d), visual id %d",
                            geometryResult, nativeVisualId);
        return;
    }

    m_impl->config = config;
    m_impl->nativeVisualId = nativeVisualId;
    m_impl->surface = eglCreateWindowSurface(m_impl->display, config, androidWindow, nullptr);
    if (m_impl->surface == EGL_NO_SURFACE) {
        LogEglError("eglCreateWindowSurface");
        return;
    }

    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    m_impl->context = eglCreateContext(m_impl->display, config, EGL_NO_CONTEXT, contextAttributes);
    if (m_impl->context == EGL_NO_CONTEXT) {
        LogEglError("eglCreateContext");
        return;
    }
    if (eglMakeCurrent(m_impl->display, m_impl->surface, m_impl->surface, m_impl->context) == EGL_FALSE) {
        LogEglError("eglMakeCurrent");
        return;
    }

    EGLint surfaceWidth = 0;
    EGLint surfaceHeight = 0;
    if (eglQuerySurface(m_impl->display, m_impl->surface, EGL_WIDTH, &surfaceWidth) == EGL_FALSE ||
        eglQuerySurface(m_impl->display, m_impl->surface, EGL_HEIGHT, &surfaceHeight) == EGL_FALSE ||
        surfaceWidth <= 0 || surfaceHeight <= 0) {
        LogEglError("eglQuerySurface");
        return;
    }
    m_impl->width = static_cast<std::uint32_t>(surfaceWidth);
    m_impl->height = static_cast<std::uint32_t>(surfaceHeight);
    eglSwapInterval(m_impl->display, m_impl->vsyncEnabled ? 1 : 0);
    m_impl->valid = true;

    const char* const vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* const renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* const version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* const shadingLanguageVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "AndroidWindowManagerUVE: EGL %d.%d, GLES3 surface %ux%u vendor=%s renderer=%s version=%s glsl=%s",
                        majorVersion, minorVersion, m_impl->width, m_impl->height,
                        vendor != nullptr ? vendor : "<null>", renderer != nullptr ? renderer : "<null>",
                        version != nullptr ? version : "<null>",
                        shadingLanguageVersion != nullptr ? shadingLanguageVersion : "<null>");
}

AndroidWindowManagerUVE::~AndroidWindowManagerUVE() {
    if (m_impl == nullptr) {
        return;
    }
    if (m_impl->display != EGL_NO_DISPLAY) {
        if (m_impl->context != EGL_NO_CONTEXT) {
            eglMakeCurrent(m_impl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroyContext(m_impl->display, m_impl->context);
        }
        if (m_impl->surface != EGL_NO_SURFACE) {
            eglDestroySurface(m_impl->display, m_impl->surface);
        }
        eglTerminate(m_impl->display);
    }
}

bool AndroidWindowManagerUVE::IsValidUVE() const noexcept { return m_impl != nullptr && m_impl->valid; }

void AndroidWindowManagerUVE::AttachInputSystemUVE(Input::IInputSystemUVE* inputSystem) noexcept {
    static_cast<void>(inputSystem);
}

void AndroidWindowManagerUVE::PollEventsUVE() {}

bool AndroidWindowManagerUVE::TryRecoverSurfaceUVE() noexcept {
    if (m_impl == nullptr || m_impl->display == EGL_NO_DISPLAY || m_impl->context == EGL_NO_CONTEXT ||
        m_impl->nativeWindow == nullptr || m_impl->config == nullptr || m_impl->nativeVisualId == 0) {
        return false;
    }
    if (m_impl->valid) {
        return true;
    }

    auto logRecoveryFailure = [this](const char* const operation) noexcept {
        if (!m_impl->recoveryFailureLogged) {
            LogEglError(operation);
            m_impl->recoveryFailureLogged = true;
        }
        return false;
    };

    if (m_impl->surface != EGL_NO_SURFACE) {
        static_cast<void>(eglMakeCurrent(m_impl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
        static_cast<void>(eglDestroySurface(m_impl->display, m_impl->surface));
        m_impl->surface = EGL_NO_SURFACE;
    }

    ANativeWindow* const androidWindow = static_cast<ANativeWindow*>(m_impl->nativeWindow);
    if (ANativeWindow_setBuffersGeometry(androidWindow, 0, 0, m_impl->nativeVisualId) != 0) {
        return logRecoveryFailure("ANativeWindow_setBuffersGeometry during recovery");
    }

    m_impl->surface = eglCreateWindowSurface(m_impl->display, m_impl->config, androidWindow, nullptr);
    if (m_impl->surface == EGL_NO_SURFACE) {
        return logRecoveryFailure("eglCreateWindowSurface during recovery");
    }
    if (eglMakeCurrent(m_impl->display, m_impl->surface, m_impl->surface, m_impl->context) == EGL_FALSE) {
        static_cast<void>(eglDestroySurface(m_impl->display, m_impl->surface));
        m_impl->surface = EGL_NO_SURFACE;
        return logRecoveryFailure("eglMakeCurrent during recovery");
    }

    EGLint recoveredWidth = 0;
    EGLint recoveredHeight = 0;
    if (eglQuerySurface(m_impl->display, m_impl->surface, EGL_WIDTH, &recoveredWidth) == EGL_FALSE ||
        eglQuerySurface(m_impl->display, m_impl->surface, EGL_HEIGHT, &recoveredHeight) == EGL_FALSE ||
        recoveredWidth <= 0 || recoveredHeight <= 0) {
        static_cast<void>(eglMakeCurrent(m_impl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
        static_cast<void>(eglDestroySurface(m_impl->display, m_impl->surface));
        m_impl->surface = EGL_NO_SURFACE;
        return logRecoveryFailure("eglQuerySurface during recovery");
    }

    m_impl->width = static_cast<std::uint32_t>(recoveredWidth);
    m_impl->height = static_cast<std::uint32_t>(recoveredHeight);
    eglSwapInterval(m_impl->display, m_impl->vsyncEnabled ? 1 : 0);
    m_impl->valid = true;
    m_impl->recoveryFailureLogged = false;
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "AndroidWindowManagerUVE: recovered EGL surface %ux%u",
                        m_impl->width, m_impl->height);
    return true;
}

void AndroidWindowManagerUVE::SwapBuffersUVE() {
    if (!IsValidUVE()) {
        return;
    }
    if (eglSwapBuffers(m_impl->display, m_impl->surface) == EGL_FALSE) {
        LogEglError("eglSwapBuffers");
        m_impl->valid = false;
        m_impl->recoveryFailureLogged = false;
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "%s", "AndroidWindowManagerUVE: presentation surface lost after eglSwapBuffers");
        return;
    }
    if (!m_impl->firstSwapLogged) {
        m_impl->firstSwapLogged = true;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "AndroidWindowManagerUVE: first eglSwapBuffers succeeded for %ux%u",
                            m_impl->width, m_impl->height);
    }
}

bool AndroidWindowManagerUVE::IsCloseRequestedUVE() const noexcept {
    return m_impl != nullptr && m_impl->closeRequested;
}

void AndroidWindowManagerUVE::SetVSyncEnabledUVE(const bool enabled) {
    if (m_impl == nullptr) {
        return;
    }
    m_impl->vsyncEnabled = enabled;
    if (IsValidUVE()) {
        eglSwapInterval(m_impl->display, enabled ? 1 : 0);
    }
}

bool AndroidWindowManagerUVE::IsVSyncEnabledUVE() const noexcept {
    return m_impl != nullptr && m_impl->vsyncEnabled;
}

void AndroidWindowManagerUVE::SetFullscreenUVE(const bool fullscreen) {
    if (m_impl != nullptr) {
        m_impl->fullscreen = fullscreen;
    }
}

bool AndroidWindowManagerUVE::IsFullscreenUVE() const noexcept {
    return m_impl != nullptr && m_impl->fullscreen;
}

std::uint32_t AndroidWindowManagerUVE::GetWidthUVE() const noexcept {
    if (!IsValidUVE()) {
        return 0U;
    }
    EGLint width = 0;
    if (eglQuerySurface(m_impl->display, m_impl->surface, EGL_WIDTH, &width) != EGL_TRUE || width <= 0) {
        m_impl->valid = false;
        return 0U;
    }
    m_impl->width = static_cast<std::uint32_t>(width);
    return m_impl->width;
}

std::uint32_t AndroidWindowManagerUVE::GetHeightUVE() const noexcept {
    if (!IsValidUVE()) {
        return 0U;
    }
    EGLint height = 0;
    if (eglQuerySurface(m_impl->display, m_impl->surface, EGL_HEIGHT, &height) != EGL_TRUE || height <= 0) {
        m_impl->valid = false;
        return 0U;
    }
    m_impl->height = static_cast<std::uint32_t>(height);
    return m_impl->height;
}

std::vector<MonitorInfoUVE> AndroidWindowManagerUVE::EnumerateMonitorsUVE() const { return {}; }

void* AndroidWindowManagerUVE::GetNativeWindowHandleUVE() const noexcept {
    return m_impl == nullptr ? nullptr : m_impl->nativeWindow;
}

std::string_view AndroidWindowManagerUVE::GetBackendNameUVE() const noexcept { return "Android EGL/GLES3"; }

} // namespace UVE::Window
