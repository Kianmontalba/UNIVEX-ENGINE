// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "uve/render/texture_handle_uve.h"

namespace UVE::Render {

class ICommandBufferUVE;

/// A graph-local identifier for an externally owned texture. RenderGraphUVE never creates or
/// destroys imported textures; Renderer3DUVE remains responsible for their lifetime.
struct RenderGraphResourceHandleUVE {
    std::uint32_t value = UINT32_MAX;
    std::uint64_t generation = 0U;
    [[nodiscard]] constexpr bool IsValidUVE() const noexcept { return value != UINT32_MAX; }
    friend constexpr bool operator==(RenderGraphResourceHandleUVE, RenderGraphResourceHandleUVE) = default;
};

enum class RenderGraphResourceAccessUVE : std::uint8_t { Read, Write };

struct RenderGraphResourceUseUVE {
    RenderGraphResourceHandleUVE resource;
    RenderGraphResourceAccessUVE access = RenderGraphResourceAccessUVE::Read;
};

/// One recorded pass. Passes execute in stable insertion order after resource declarations are
/// validated. Dependencies are explicit through reads/writes, while parallel scheduling, aliasing,
/// and barriers are deliberately deferred beyond the foundation increment.
struct RenderGraphPassDescUVE {
    std::string debugNameUVE;
    std::vector<RenderGraphResourceUseUVE> resources;
    std::function<void(ICommandBufferUVE&)> recordCallbackUVE;
};

class RenderGraphUVE final {
public:
    [[nodiscard]] RenderGraphResourceHandleUVE ImportTextureUVE(TextureHandleUVE texture,
                                                                  std::string debugNameUVE);
    void AddPassUVE(RenderGraphPassDescUVE desc);

    /// Adds a pass from caller-owned resource-use storage. RenderGraphUVE copies the span into
    /// retained pass storage before returning; the span and callback arguments need only remain
    /// valid for this call. This avoids a temporary descriptor vector for stable per-frame graphs.
    void AddPassUVE(std::string debugNameUVE, std::span<const RenderGraphResourceUseUVE> resources,
                    std::function<void(ICommandBufferUVE&)> recordCallbackUVE);

    /// Reserves storage for a known graph topology. ClearUVE() preserves outer and nested storage
    /// capacity so a renderer can rebuild its per-frame callbacks without repeatedly reallocating
    /// pass/resource arrays.
    void ReserveUVE(std::size_t resourceCapacity, std::size_t passCapacity);

    /// Validates resource declarations then invokes each callback in deterministic insertion order.
    /// Returns false and records no callbacks for invalid declarations.
    [[nodiscard]] bool ExecuteUVE(ICommandBufferUVE& commandBuffer) const;
    void ClearUVE() noexcept;

    [[nodiscard]] std::size_t GetPassCountUVE() const noexcept;

private:
    struct ImportedResourceUVE {
        TextureHandleUVE texture;
        std::string debugNameUVE;
    };

    std::vector<ImportedResourceUVE> m_resources;
    std::vector<RenderGraphPassDescUVE> m_passes;
    std::size_t m_resourceCount = 0U;
    std::size_t m_passCount = 0U;
    std::uint64_t m_generation = 0U;
};

} // namespace UVE::Render
