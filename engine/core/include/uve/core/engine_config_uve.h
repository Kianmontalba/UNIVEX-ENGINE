//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "uve/debug/log_level_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Core {

/// Configuration passed into EngineCoreUVE's constructor. Every field has a
/// sensible default, so most callers (including tests) can construct one
/// with zero overrides. New fields can be added here in the future without
/// changing EngineCoreUVE's constructor signature.
/// Thread-safety: value type; read only during EngineCoreUVE::Init().
struct EngineConfigUVE {
    /// Target frames per second for the frame pipeline. Informational this
    /// increment — nothing throttles to it yet, since there is no frame
    /// limiter — but consumed by future systems comparing FrameStatsUVE's
    /// actual FPS against this target.
    double targetFps = 60.0;

    /// Frequency, in Hz, of the fixed-timestep simulation accumulator (see
    /// UVE::Utilities::ITimerUVE::SetFixedTimestepUVE()). Converted to
    /// seconds-per-step by EngineCoreUVE during Init().
    double fixedUpdateFps = 60.0;

    /// Maximum delta time, in seconds, a single frame may report (see
    /// UVE::Utilities::ITimerUVE::SetMaxDeltaTimeUVE()) — guards against a
    /// spiral of death after a debugger pause or long stall.
    double maxDeltaTimeSeconds = 0.25;

    /// Minimum severity a log message must have to reach any sink.
    Debug::LogLevelUVE minLogLevel = Debug::LogLevelUVE::Trace;

    /// Path the FileSinkUVE attached during Init() will append to.
    std::filesystem::path logFilePath = "uve_engine.log";

    /// Whether a ConsoleSinkUVE is attached during Init(), in addition to
    /// the FileSinkUVE (which is always attached regardless of this flag).
    bool enableConsoleLogging = true;

    /// Number of ThreadPoolUVE worker threads to spawn during Init(). `0`
    /// (the default) means "auto" — see
    /// UVE::Threading::ThreadPoolUVE::ThreadPoolUVE() for the exact
    /// resolution policy.
    std::size_t threadPoolWorkerCount = 0;

    /// Path ConfigManagerUVE::LoadUVE() is called with during Init(). A
    /// missing file at this path is not an error (see
    /// IConfigManagerUVE::LoadUVE()) — a first-run engine has no settings
    /// file yet.
    std::filesystem::path settingsFilePath = ".uvesettings";

    /// Raw startup argument tokens (excluding the program path) that
    /// CommandLineUVE parses during Init(). Populated by main() from
    /// argv[1..argc); left empty by default so tests can construct an
    /// EngineConfigUVE without a real process argv.
    std::vector<std::string> commandLineArgs = {};

    /// Path AssetDatabaseUVE::LoadUVE() is called with during Init(). A
    /// missing file at this path is not an error (see
    /// IAssetDatabaseUVE::LoadUVE()) — a first-run project has no asset
    /// registry yet.
    std::filesystem::path assetDatabaseFilePath = ".uveassetdb";

    /// Root directory the read-only ProjectFileIndexUVE explicitly scans for
    /// editor Asset Browser presentation. Missing or empty roots are valid
    /// empty snapshots and are never created by the index. The root does not
    /// change AssetDatabaseUVE ownership, import behavior, or VFS mounts.
    std::filesystem::path projectContentRootUVE = "assets/";

    /// Project-local root reserved for derived import metadata only. DerivedArtifactCacheUVE
    /// creates this directory lazily on successful cache writes; it never creates source or
    /// destination directories and never changes AssetDatabaseUVE ownership.
    std::filesystem::path derivedArtifactCacheRootUVE = "DerivedData/Import/";

    /// Whether Update() calls HotReloadUVE::PollUVE() each frame.
    /// AssetManagerUVE still tracks/untracks loaded assets with HotReloadUVE
    /// regardless of this flag — it only gates whether the poll itself
    /// runs, so flipping it at runtime (by mutating a running
    /// EngineCoreUVE's config — not currently exposed, but the field itself
    /// is read fresh every Update()) takes effect immediately.
    bool hotReloadEnabledUVE = true;

    /// Poll interval, in seconds, HotReloadUVE waits between checking every
    /// tracked asset's on-disk modification time (see
    /// IHotReloadUVE::PollUVE()).
    double hotReloadPollIntervalSecondsUVE = 1.0;

    /// Fixed dimensions, in pixels, of Renderer3DUVE's offscreen color/depth render target
    /// (see Render::Renderer3DUVE). No WindowManagerUVE/swapchain exists yet in this sandbox to
    /// resize against, so these are set once at Init() and cannot change for a running
    /// EngineCoreUVE's lifetime.
    std::uint32_t renderTargetWidth = 1280;
    std::uint32_t renderTargetHeight = 720;

    /// Global flat ambient light term Renderer3DUVE adds to every rendered item's final color
    /// (Render::Renderer3DUVE's `ambientColor` constructor parameter), independent of whether any
    /// LightComponentUVE-bearing entity exists this frame (see Render::LightSystemUVE, Increment
    /// 23). No "scene settings" component/system exists in this engine yet, so — exactly like
    /// `gravity` below — an engine-config-level constant is this engine's only existing precedent
    /// for a whole-scene tunable. Low, desaturated default so an unlit scene isn't pitch black.
    Math::Vector3UVE ambientColor{0.05F, 0.05F, 0.05F};

    /// Width/height, in texels, of the persistent depth-only texture Renderer3DUVE renders its
    /// directional-light shadow depth pre-pass into (see Render::Renderer3DUVE, Increment 26). A
    /// square texture, sized once at construction — like renderTargetWidth/Height above, this
    /// can't change for a running EngineCoreUVE's lifetime.
    std::uint32_t shadowMapResolution = 2048;

    /// Half-size, in world units, of the fixed orthographic box Renderer3DUVE's shadow depth
    /// pre-pass projects through, centered on the shadow-casting light entity's world position.
    /// Not fitted to the camera's visible frustum (that's future work, e.g. cascaded shadow
    /// maps) — a fixed box is the simplest choice that needs no new frustum-fitting/AABB math,
    /// matching this engine's "no unnecessary new math" precedent.
    float shadowMapHalfExtent = 20.0F;

    /// Near/far planes of that same orthographic shadow projection, in world units measured from
    /// the shadow-casting light entity along its forward direction.
    float shadowMapNearPlane = 0.1F;
    float shadowMapFarPlane = 100.0F;

    /// Extra extent, in light-view world units, added to each fitted directional shadow-frustum
    /// bound. Prevents edge clipping caused by small camera movement or floating-point rounding;
    /// negative caller values are clamped to zero by Renderer3DUVE.
    float shadowFrustumPadding = 1.0F;

    /// Practical-split blend for the fixed three directional shadow cascades. `0` distributes
    /// splits uniformly over the camera depth range, `1` uses fully logarithmic spacing, and
    /// Renderer3DUVE clamps caller values to `[0, 1]`. The default favors near-camera detail
    /// without starving the far cascade.
    float shadowCascadeSplitLambda = 0.5F;

    /// Fraction of each non-final cascade range used to cross-fade into the next cascade. The
    /// renderer clamps this to `[0, 0.25]`; zero preserves the Increment 30 hard cascade boundary.
    float shadowCascadeBlendRatio = 0.1F;

    /// Radius, in shadow-map texels, of the square percentage-closer-filtering kernel the
    /// canonical directional-shadow material shader uses. `0` preserves a single hard comparison;
    /// `1` (the default) produces a 3x3 soft-shadow kernel. Renderer3DUVE clamps larger values to
    /// `2` so the per-fragment sampling cost remains bounded at 25 depth comparisons.
    std::uint32_t shadowPcfKernelRadius = 1;

    /// Acceleration PhysicsSystemUVE (see Physics::PhysicsSystemUVE) applies to every
    /// non-kinematic RigidBodyComponentUVE each fixed step, scaled by its own gravityScale.
    /// Earth-like default, Y-up (matching this engine's convention throughout).
    Math::Vector3UVE gravity{0.0F, -9.81F, 0.0F};

    /// Directory Save::SaveGameSystemUVE reads/writes numbered save slots (and
    /// Save::CheckpointManagerUVE's reserved auto-save/checkpoint slot) under. Created lazily on
    /// first write — a first-run engine has no save directory yet, mirroring
    /// settingsFilePath/assetDatabaseFilePath's own "missing is not an error" contract.
    std::filesystem::path saveDirectoryPath = "saves/";

    /// Auto-save interval, in seconds, CheckpointManagerUVE::UpdateUVE() waits between writes to
    /// its reserved auto-save slot (see Save::kAutoSaveSlotIndexUVE). The spec's "every 5
    /// minutes" example; test suites override this to a small value to avoid a real wait.
    double autoSaveIntervalSecondsUVE = 300.0;

    /// When true, EngineCoreUVE::Init() constructs Window::NullWindowManagerUVE and
    /// Render::NullRenderDeviceUVE instead of the real GLFW3/OpenGL backends — no window, no GL
    /// context, safe to run with no display attached (CI, this project's own test suite). Also
    /// settable via the `--headless` CLI flag (CommandLineUVE::HasFlagUVE("headless")), read and
    /// OR'd into this field during Init() right after CommandLineUVE is constructed.
    bool headlessUVE = false;

    /// Title bar text WindowManagerUVE's real backend creates its window with. Unused when
    /// headlessUVE is true.
    std::string windowTitle = "UniVex Engine";

    /// Requested window size, in pixels, at creation time (the OS/window manager may still clamp
    /// or override it). Unused when headlessUVE is true.
    std::uint32_t windowWidth = 1280;
    std::uint32_t windowHeight = 720;

    /// Whether the real window is user-resizable.
    bool windowResizableUVE = true;

    /// Whether the real window's GL context starts with vertical sync enabled.
    bool vsyncEnabledUVE = true;

    /// Requested OpenGL context version, forwarded to Window::WindowDescUVE::glVersionMajor/Minor.
    /// The production default is OpenGL 4.6 Core, per the approved architecture decision.
    /// Configurable (not hardcoded) because GL context availability is a real driver/platform
    /// fact — this project's own development sandbox (Mesa llvmpipe under Xvfb) caps at 4.5 Core
    /// and fails to create a 4.6 context; overriding these two fields is how a caller targets
    /// that sandbox specifically without changing what real hardware/CI requests by default.
    std::uint32_t windowGlVersionMajor = 4;
    std::uint32_t windowGlVersionMinor = 6;

    /// Directory Shader::ShaderManagerUVE persists compiled GL program binaries under (see its
    /// on-disk cache, keyed by a hash of each program's fully resolved — post-#include,
    /// post-macro — source). A per-platform subdirectory is appended automatically. Created
    /// lazily on first write, mirroring saveDirectoryPath's own "missing is not an error"
    /// contract.
    std::filesystem::path shaderCachePath = "shader_cache/";

    /// Whether ShaderManagerUVE::UpdateUVE() polls loaded shader programs' source file (and
    /// #include closure) modification times each frame and hot-swaps a recompiled program when
    /// one changed. Debug builds default to on (iteration speed); Release defaults to off (no
    /// reason a shipped build ever re-reads shader source off disk).
#if UVE_DEBUG
    bool shaderHotReloadEnabledUVE = true;
#else
    bool shaderHotReloadEnabledUVE = false;
#endif

    /// Poll interval, in seconds, ShaderManagerUVE waits between checking every hot-reload-tracked
    /// program's dependency closure for an on-disk modification time change — the shader-specific
    /// analogue of hotReloadPollIntervalSecondsUVE (kept as its own field since the two subsystems
    /// are otherwise unrelated and may want independent tuning later).
    double shaderHotReloadPollIntervalSecondsUVE = 1.0;

    /// Virtual-filesystem mount prefix ShaderManagerUVE's #include resolver and built-in shader
    /// loader resolve paths against (e.g. "shaders/basic_3d.glsl"), and the real directory it's
    /// mounted from during Init(). A shader file missing at this location is not an error — every
    /// built-in also carries an embedded C++ string fallback (see
    /// Render::Shader::BuiltIn::kBasic3DSource) used automatically when the mount doesn't
    /// resolve, so the engine still runs (without hot-reload) if launched from a working
    /// directory where the source tree isn't reachable.
    std::string shaderSourceMountPrefixUVE = "shaders";
    std::filesystem::path shaderSourceRealDirectoryUVE = "engine/render/shader/built_in/";
};

} // namespace UVE::Core
