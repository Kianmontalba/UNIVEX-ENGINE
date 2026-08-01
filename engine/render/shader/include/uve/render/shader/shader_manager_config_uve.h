//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <filesystem>

namespace UVE::Render::Shader {

/// Construction-time configuration for ShaderManagerUVE, built from EngineConfigUVE by
/// EngineCoreUVE::Init() (mirrors ShaderManagerConfigUVE's role to WindowDescUVE's for
/// WindowManagerUVE).
struct ShaderManagerConfigUVE {
    /// Directory the on-disk program-binary cache is stored under (a per-platform subdirectory
    /// is appended automatically — see Detail's shader_binary_cache_uve.h).
    std::filesystem::path cachePath = "shader_cache/";

    /// Whether ShaderManagerUVE::UpdateUVE() polls hot-reload-tracked programs' dependency
    /// closures for on-disk changes at all.
    bool hotReloadEnabledUVE = true;

    /// Poll interval, in seconds, between hot-reload mtime checks.
    double hotReloadPollIntervalSecondsUVE = 1.0;

    /// Forwarded into every compile's injected `#define UVE_DEBUG 0|1` block.
    bool injectDebugDefineUVE = true;
};

} // namespace UVE::Render::Shader
