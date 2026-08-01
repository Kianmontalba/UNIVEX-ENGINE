//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>

#include "uve/asset/i_file_system_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/shader/i_shader_manager_uve.h"
#include "uve/render/shader/shader_manager_config_uve.h"
#include "uve/threading/i_thread_pool_uve.h"

namespace UVE::Render::Shader {

/// The concrete, backend-agnostic IShaderManagerUVE implementation (Increment 21) — see
/// IShaderManagerUVE's doc comment for the full architecture rationale. Public (like
/// GlRenderDeviceUVE/RenderSystemUVE/Renderer3DUVE) because EngineCoreUVE constructs it directly
/// by concrete type.
class ShaderManagerUVE final : public IShaderManagerUVE {
public:
    /// `threadPool`/`eventSystem`/`renderDevice`/`fileSystem` must all outlive this
    /// ShaderManagerUVE. `renderDevice` is used only from the main thread (inside UpdateUVE()),
    /// honoring its own single-threaded contract.
    ShaderManagerUVE(Threading::IThreadPoolUVE& threadPool, Events::IEventSystemUVE& eventSystem,
                      IRenderDeviceUVE& renderDevice, Asset::IFileSystemUVE& fileSystem,
                      ShaderManagerConfigUVE config);
    ~ShaderManagerUVE() override;

    ShaderManagerUVE(const ShaderManagerUVE&) = delete;
    ShaderManagerUVE& operator=(const ShaderManagerUVE&) = delete;

    [[nodiscard]] std::shared_ptr<ShaderSourceUVE> CreateSourceUVE(const ShaderSourceCompileDescUVE& desc) override;
    [[nodiscard]] std::shared_ptr<ShaderProgramUVE> CreateProgramUVE(const ShaderProgramDescUVE& desc) override;
    void UpdateUVE(double deltaTimeSeconds) override;

    /// Test-only hook (not part of IShaderManagerUVE): how many ShaderSourceUVE/ShaderProgramUVE
    /// background preprocessing jobs are currently in flight (submitted, not yet drained by
    /// UpdateUVE()).
    [[nodiscard]] std::size_t GetPendingJobCountUVE() const noexcept;

    /// Test-only hook: whether the most recently completed CreateProgramUVE()'s GL compile
    /// resolved via the on-disk program-binary cache (true) or a full from-source compile
    /// (false) — lets a test assert the cache-hit path was actually taken without inspecting GL
    /// state directly.
    [[nodiscard]] bool GetLastCompileUsedCacheUVE() const noexcept;

    /// Not part of the public API surface - only nameable from shader_manager_uve.cpp, where it's
    /// fully defined. Declared accessible (rather than private) purely so that translation unit's
    /// own free helper functions (which aren't members/friends of ShaderManagerUVE) can name this
    /// type and its nested job/tracking record types; every other translation unit only ever sees
    /// the forward declaration below and can do nothing with it regardless.
    struct ImplUVE;

private:
    // Every method below is implemented entirely in shader_manager_uve.cpp and exists purely so
    // its body can access ShaderSourceUVE/ShaderProgramUVE's private fields (both `friend class
    // ShaderManagerUVE` — a grant only members of this class inherit, not free functions in the
    // same translation unit). Each signature deliberately never names an ImplUVE nested type
    // (only `ImplUVE&` itself, plus already-public types) so it stays declarable here even though
    // ImplUVE itself is only forward-declared at this point.
    [[nodiscard]] static std::shared_ptr<ShaderSourceUVE> MakeSourceUVE(IRenderDeviceUVE& renderDevice,
                                                                         ShaderStageUVE stage);
    [[nodiscard]] static std::shared_ptr<ShaderProgramUVE> MakeProgramUVE(IRenderDeviceUVE& renderDevice);
    static void SubmitSourceCompileJobUVE(ImplUVE& impl, const std::shared_ptr<ShaderSourceUVE>& target,
                                           const ShaderSourceCompileDescUVE& desc);
    static void DrainCompletedSourceJobsUVE(ImplUVE& impl);
    static void ApplyPendingProgramLinksUVE(ImplUVE& impl);
    static void PollHotReloadUVE(ImplUVE& impl);

    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render::Shader
