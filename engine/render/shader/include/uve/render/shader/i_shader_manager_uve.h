// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/render/shader/shader_program_desc_uve.h"
#include "uve/render/shader/shader_program_stages_desc_uve.h"
#include "uve/render/shader/shader_program_uve.h"
#include "uve/render/shader/shader_source_compile_desc_uve.h"
#include "uve/render/shader/shader_source_uve.h"

namespace UVE::Render::Shader {

/// IShaderManagerUVE is the backend-agnostic interface for runtime shader loading, `#include`
/// resolution, conditional-compilation preprocessing, hot-reload, uniform reflection, and
/// on-disk program-binary caching (Increment 21). Built entirely on top of IRenderDeviceUVE — it
/// never talks to any GL type directly, reusing IRenderDeviceUVE's own
/// CreateShaderUVE/CreatePipelineUVE compile+link+error-capture logic instead of duplicating it —
/// so it works identically against NullRenderDeviceUVE (headless) and GlRenderDeviceUVE (real
/// GL), with no separate Null/Gl ShaderManager implementation needed.
/// Thread-safety: CreateSourceUVE()/CreateProgramUVE() are safe to call from the main thread only
/// (they submit background work but return immediately); the underlying preprocessing runs on an
/// IThreadPoolUVE worker. UpdateUVE() must be called once per frame from the main thread — it is
/// the only place a real IRenderDeviceUVE compile/link call happens, honoring
/// IRenderDeviceUVE's own main-thread-only contract.
class IShaderManagerUVE {
public:
    virtual ~IShaderManagerUVE() = default;

    /// Begins compiling one shader stage. Returns immediately with a not-yet-ready
    /// ShaderSourceUVE — the real compile happens asynchronously; poll IsReadyUVE()/IsValidUVE()
    /// or subscribe to hot-reload events to observe completion.
    [[nodiscard]] virtual std::shared_ptr<ShaderSourceUVE> CreateSourceUVE(
        const ShaderSourceCompileDescUVE& desc) = 0;

    /// Begins compiling and linking a vertex+fragment program from the same resolved source.
    /// Returns immediately with a not-yet-ready ShaderProgramUVE, exactly like CreateSourceUVE().
    [[nodiscard]] virtual std::shared_ptr<ShaderProgramUVE> CreateProgramUVE(const ShaderProgramDescUVE& desc) = 0;

    /// Begins compiling and linking a vertex+fragment program from two independent source
    /// descriptors. This is the managed path for MaterialAssetUVE's separate vertexShader and
    /// fragmentShader assets; it has the same asynchronous readiness and program-level hot-reload
    /// contract as CreateProgramUVE().
    [[nodiscard]] virtual std::shared_ptr<ShaderProgramUVE> CreateProgramFromStagesUVE(
        const ShaderProgramStagesDescUVE& desc) = 0;

    /// Drains any completed background preprocessing (running the real GL compile/link/cache
    /// lookup synchronously on the calling thread), and - if hot-reload is enabled - polls every
    /// tracked program's dependency closure for on-disk changes, triggering a recompile when one
    /// is found. Call exactly once per frame from the main thread.
    virtual void UpdateUVE(double deltaTimeSeconds) = 0;
};

} // namespace UVE::Render::Shader
