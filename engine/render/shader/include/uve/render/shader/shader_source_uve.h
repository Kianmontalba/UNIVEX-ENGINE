// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "uve/render/render_resource_descs_uve.h"
#include "uve/render/shader/shader_compile_diagnostics_uve.h"
#include "uve/render/shader_handle_uve.h"

namespace UVE::Render::Shader {

class ShaderManagerUVE;

/// One compiled shader stage — a thin wrapper over a ShaderHandleUVE plus the metadata
/// ShaderManagerUVE needs for hot-reload/diagnostics (Increment 21). Always handed out as a
/// std::shared_ptr<ShaderSourceUVE> by IShaderManagerUVE::CreateSourceUVE() — never constructed
/// directly (private constructor, only ShaderManagerUVE may build one). A hot-reload swap mutates
/// this same object's fields in place, so every holder observes the reload automatically.
/// Thread-safety: not thread-safe. Read-only accessors are safe to call from the main thread at
/// any time (they may transiently report stale data mid-reload); mutation happens only from
/// ShaderManagerUVE::UpdateUVE(), which is itself main-thread-only.
class ShaderSourceUVE final {
public:
    [[nodiscard]] ShaderStageUVE GetStageUVE() const noexcept { return m_stage; }
    [[nodiscard]] ShaderHandleUVE GetHandleUVE() const noexcept { return m_handle; }

    /// True once the background preprocessing job has completed and the shader has (successfully
    /// or not) been through its first real GL compile attempt. False for a freshly created source
    /// still waiting on its background job.
    [[nodiscard]] bool IsReadyUVE() const noexcept { return m_ready; }

    /// True iff the most recent compile attempt succeeded — GetHandleUVE() only refers to a real,
    /// usable shader when this is true.
    [[nodiscard]] bool IsValidUVE() const noexcept { return m_valid; }

    /// The fully #include-expanded, macro-substituted source text that was actually compiled
    /// (empty until IsReadyUVE()).
    [[nodiscard]] const std::string& GetResolvedSourceUVE() const noexcept { return m_resolvedSource; }

    [[nodiscard]] const ShaderCompileDiagnosticsUVE& GetDiagnosticsUVE() const noexcept { return m_diagnostics; }

    /// FNV-1a hash of the resolved source + stage + entry point + backend/GL-version identity —
    /// the program-binary cache key (see Detail's shader_binary_cache_uve.h).
    [[nodiscard]] std::uint64_t GetContentHashUVE() const noexcept { return m_contentHash; }

private:
    friend class ShaderManagerUVE;
    ShaderSourceUVE() = default;

    ShaderStageUVE m_stage = ShaderStageUVE::Vertex;
    ShaderHandleUVE m_handle = kInvalidShaderHandleUVE;
    bool m_ready = false;
    bool m_valid = false;
    std::string m_resolvedSource;
    ShaderCompileDiagnosticsUVE m_diagnostics;
    std::uint64_t m_contentHash = 0;

    /// Every virtual path read while resolving this source's #include closure (root + every
    /// transitively included file) — ShaderManagerUVE-internal bookkeeping for hot-reload
    /// dependency tracking, not part of the public API.
    std::vector<std::string> m_dependencyClosure;
};

} // namespace UVE::Render::Shader
