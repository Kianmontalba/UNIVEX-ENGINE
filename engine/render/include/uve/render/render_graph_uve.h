// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "uve/render/texture_handle_uve.h"

namespace UVE::Render {

class ICommandBufferUVE;

/// A graph-local identifier for an externally owned texture. RenderGraphUVE never creates or
/// destroys imported textures; Renderer3DUVE remains responsible for their lifetime.
struct RenderGraphResourceHandleUVE {
    std::uint32_t value = UINT32_MAX;
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
};

} // namespace UVE::Render
