#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#include "uve/core/engine_config_uve.h"
#include "uve/core/engine_core_uve.h"
#include "uve/core/engine_state_uve.h"
#include "uve/input/i_mobile_input_system_uve.h"
#include "uve/window/android_surface_size_uve.h"

namespace {

constexpr char kLogTag[] = "UniVexNativeEngine";
constexpr std::uint64_t kInvalidAndroidTouchIdentifier = 0U;
constexpr std::uint32_t kAndroidShadowMapResolutionUVE = 1024U;
constexpr std::size_t kAndroidWorkerCountUVE = 2U;

std::unique_ptr<UVE::Core::EngineCoreUVE> g_engine;
std::filesystem::path g_projectRoot;
std::array<std::uint64_t, UVE::Input::kMaximumTouchCountUVE> g_touchIdentifiers{};

void LogError(const char* const message) noexcept {
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", message);
}

void ResetTouchBridge() noexcept {
    g_touchIdentifiers.fill(kInvalidAndroidTouchIdentifier);
}

std::uint64_t ToUveTouchIdentifier(const std::int32_t androidPointerId) noexcept {
    if (androidPointerId < 0) {
        return kInvalidAndroidTouchIdentifier;
    }
    // UVE reserves zero as "not active", so preserve Android's stable pointer id with a +1 offset.
    return static_cast<std::uint64_t>(androidPointerId) + 1U;
}

std::size_t FindTouchSlot(const std::uint64_t identifier, const bool allocate) noexcept {
    if (identifier == kInvalidAndroidTouchIdentifier) {
        return UVE::Input::kMaximumTouchCountUVE;
    }
    for (std::size_t slot = 0U; slot < g_touchIdentifiers.size(); ++slot) {
        if (g_touchIdentifiers[slot] == identifier) {
            return slot;
        }
    }
    if (!allocate) {
        return UVE::Input::kMaximumTouchCountUVE;
    }
    for (std::size_t slot = 0U; slot < g_touchIdentifiers.size(); ++slot) {
        if (g_touchIdentifiers[slot] == kInvalidAndroidTouchIdentifier) {
            g_touchIdentifiers[slot] = identifier;
            return slot;
        }
    }
    return UVE::Input::kMaximumTouchCountUVE;
}

void SetTouchState(const std::size_t slot, const bool active, const std::uint64_t identifier,
                   const float x, const float y, const float pressure) {
    if (g_engine == nullptr || slot >= g_touchIdentifiers.size()) {
        return;
    }
    g_engine->GetServicesUVE().GetMobileInputSystemUVE().SetTouchStateUVE(
        slot, active, identifier, UVE::Math::Vector2UVE{x, y}, pressure);
}

void ForwardMotionEvent(AInputEvent* const event) {
    if (g_engine == nullptr || event == nullptr) {
        return;
    }

    const int32_t action = AMotionEvent_getAction(event);
    const int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t actionPointerIndex = static_cast<std::size_t>(
        (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const std::size_t pointerCount = std::min(
        static_cast<std::size_t>(AMotionEvent_getPointerCount(event)), g_touchIdentifiers.size());

    if (actionMasked == AMOTION_EVENT_ACTION_CANCEL) {
        for (std::size_t slot = 0U; slot < g_touchIdentifiers.size(); ++slot) {
            if (g_touchIdentifiers[slot] != kInvalidAndroidTouchIdentifier) {
                SetTouchState(slot, false, g_touchIdentifiers[slot], 0.0F, 0.0F, 0.0F);
            }
        }
        ResetTouchBridge();
        return;
    }

    if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (actionPointerIndex < pointerCount) {
            const std::uint64_t identifier = ToUveTouchIdentifier(
                AMotionEvent_getPointerId(event, static_cast<size_t>(actionPointerIndex)));
            const std::size_t slot = FindTouchSlot(identifier, false);
            if (slot < g_touchIdentifiers.size()) {
                SetTouchState(slot, false, identifier, 0.0F, 0.0F, 0.0F);
                g_touchIdentifiers[slot] = kInvalidAndroidTouchIdentifier;
            }
        }
    }

    if (actionMasked == AMOTION_EVENT_ACTION_DOWN || actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN ||
        actionMasked == AMOTION_EVENT_ACTION_MOVE) {
        for (std::size_t pointerIndex = 0U; pointerIndex < pointerCount; ++pointerIndex) {
            const std::uint64_t identifier = ToUveTouchIdentifier(
                AMotionEvent_getPointerId(event, static_cast<size_t>(pointerIndex)));
            const std::size_t slot = FindTouchSlot(identifier, true);
            if (slot >= g_touchIdentifiers.size()) {
                continue;
            }
            SetTouchState(slot, true, identifier, AMotionEvent_getX(event, static_cast<size_t>(pointerIndex)),
                           AMotionEvent_getY(event, static_cast<size_t>(pointerIndex)),
                           AMotionEvent_getPressure(event, static_cast<size_t>(pointerIndex)));
        }
    }
}

bool WriteInitialProject(const std::filesystem::path& projectRoot) {
    std::error_code error;
    std::filesystem::create_directories(projectRoot / "Content", error);
    if (error) {
        LogError("Unable to create project Content directory.");
        return false;
    }
    std::filesystem::create_directories(projectRoot / "Settings", error);
    if (error) {
        LogError("Unable to create project Settings directory.");
        return false;
    }

    std::ofstream project(projectRoot / "native_project.uveditor", std::ios::binary | std::ios::trunc);
    if (!project.is_open()) {
        LogError("Unable to create native .uveditor project.");
        return false;
    }
    project << "{\n"
            << "  \"format\": \"uveditor\",\n"
            << "  \"schemaVersion\": 1,\n"
            << "  \"revision\": 1,\n"
            << "  \"projectId\": \"native_project\",\n"
            << "  \"displayName\": \"Native Project\",\n"
            << "  \"engineVersion\": {\"major\": 0, \"minor\": 1, \"patch\": 0, \"build\": 0},\n"
            << "  \"contentRoot\": \"Content\",\n"
            << "  \"assetDatabasePath\": \"Content/.uveassets\",\n"
            << "  \"settingsPath\": \"Settings/editor.json\"\n"
            << "}\n";

    std::ofstream assets(projectRoot / "Content/.uveassets", std::ios::binary | std::ios::trunc);
    if (!assets.is_open()) {
        LogError("Unable to create native asset database.");
        return false;
    }
    assets << "{\"schemaVersion\":1,\"assets\":[]}\n";

    std::ofstream settings(projectRoot / "Settings/editor.json", std::ios::binary | std::ios::trunc);
    if (!settings.is_open()) {
        LogError("Unable to create native editor settings.");
        return false;
    }
    settings << "{\"schemaVersion\":1,\"platform\":\"android\",\"editor\":\"native\"}\n";
    return true;
}

bool StartNativeEngine(android_app* const app) {
    if (app == nullptr || app->window == nullptr || app->activity == nullptr ||
        app->activity->internalDataPath == nullptr) {
        LogError("Native UVE engine requires an Android window and internal data path.");
        return false;
    }

    ResetTouchBridge();
    try {
        g_projectRoot = std::filesystem::path(app->activity->internalDataPath) / "projects" / "native_project";
        if (!WriteInitialProject(g_projectRoot)) {
            return false;
        }
    } catch (const std::exception& exception) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Android project setup exception: %s", exception.what());
        return false;
    } catch (...) {
        LogError("Android project setup failed with an unknown exception.");
        return false;
    }

    const AndroidSurfaceSizeUVE surfaceSize = ClampAndroidSurfaceSizeUVE(
        ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
    if (surfaceSize.width == 0U || surfaceSize.height == 0U) {
        LogError("Android native window has no positive surface size; deferring engine startup.");
        return false;
    }

    UVE::Core::EngineConfigUVE config;
    config.nativeWindowHandleUVE = app->window;
    config.windowTitle = "UniVex Editor";
    config.windowWidth = surfaceSize.width;
    config.windowHeight = surfaceSize.height;
    config.windowResizableUVE = false;
    config.windowGlVersionMajor = 3;
    config.windowGlVersionMinor = 0;
    config.enableConsoleLogging = false;
    config.threadPoolWorkerCount = kAndroidWorkerCountUVE;
    config.hotReloadEnabledUVE = false;
    config.shaderHotReloadEnabledUVE = false;
    config.logFilePath = g_projectRoot / "Settings/uve_android.log";
    config.settingsFilePath = g_projectRoot / "Settings/editor.json";
    config.assetDatabaseFilePath = g_projectRoot / "Content/.uveassets";
    config.projectContentRootUVE = g_projectRoot / "Content";
    config.derivedArtifactCacheRootUVE = g_projectRoot / "DerivedData/Import";
    config.shaderCachePath = g_projectRoot / "Settings/shader_cache";
    config.saveDirectoryPath = g_projectRoot / "SaveData";
    config.renderTargetWidth = surfaceSize.width;
    config.renderTargetHeight = surfaceSize.height;
    config.shadowMapResolution = kAndroidShadowMapResolutionUVE;
    config.commandLineArgs.clear();

    try {
        g_engine = std::make_unique<UVE::Core::EngineCoreUVE>(std::move(config));
        g_engine->Init();
        if (!g_engine->Load()) {
            LogError("EngineCoreUVE failed to load the Android native project.");
            if (g_engine->GetStateUVE() == UVE::Core::EngineStateUVE::Running) {
                g_engine->Shutdown();
            }
            g_engine.reset();
            ResetTouchBridge();
            return false;
        }
    } catch (const std::exception& exception) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Android engine startup exception: %s", exception.what());
        g_engine.reset();
        ResetTouchBridge();
        return false;
    } catch (...) {
        LogError("Android engine startup failed with an unknown exception.");
        g_engine.reset();
        ResetTouchBridge();
        return false;
    }
    return true;
}

void StopNativeEngine() noexcept {
    if (g_engine != nullptr) {
        if (g_engine->GetStateUVE() == UVE::Core::EngineStateUVE::Running) {
            g_engine->Shutdown();
        }
        g_engine.reset();
    }
    ResetTouchBridge();
}

int32_t HandleInput(android_app* const /*app*/, AInputEvent* const event) {
    if (event == nullptr || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }
    try {
        ForwardMotionEvent(event);
    } catch (const std::exception& exception) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Android input exception: %s", exception.what());
    } catch (...) {
        LogError("Android input failed with an unknown exception.");
    }
    return 1;
}

void HandleAppCommand(android_app* const app, const int32_t command) {
    if (app == nullptr) {
        LogError("Android app command received a null app pointer.");
        return;
    }
    try {
        switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr && !StartNativeEngine(app)) {
                LogError("Android engine startup deferred; waiting for a later window event.");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            StopNativeEngine();
            break;
        case APP_CMD_CONFIG_CHANGED:
            StopNativeEngine();
            if (app->window != nullptr && !StartNativeEngine(app)) {
                LogError("Android engine restart deferred; waiting for a later window event.");
            }
            break;
        default:
            break;
        }
    } catch (const std::exception& exception) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Android app-command exception: %s", exception.what());
        StopNativeEngine();
    } catch (...) {
        LogError("Android app command failed with an unknown exception.");
        StopNativeEngine();
    }
}

} // namespace

void android_main(android_app* const app) {
    app->onAppCmd = &HandleAppCommand;
    app->onInputEvent = &HandleInput;

    while (true) {
        int events = 0;
        android_poll_source* source = nullptr;
        const int timeout = g_engine != nullptr ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                StopNativeEngine();
                return;
            }
        }

        if (g_engine != nullptr && g_engine->GetStateUVE() == UVE::Core::EngineStateUVE::Running) {
            try {
                g_engine->TickFrameUVE();
            } catch (const std::exception& exception) {
                __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Android frame exception: %s", exception.what());
                StopNativeEngine();
            } catch (...) {
                LogError("Android frame failed with an unknown exception.");
                StopNativeEngine();
            }
        } else if (g_engine == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}
