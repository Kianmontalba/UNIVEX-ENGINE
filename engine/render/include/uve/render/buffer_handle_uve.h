//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <functional>

namespace UVE::Render {

/// Opaque handle to a GPU buffer resource created via IRenderDeviceUVE::CreateBufferUVE(). A
/// small wrapper struct (not a bare std::uint32_t alias) so a BufferHandleUVE can never be
/// silently passed where a TextureHandleUVE/ShaderHandleUVE/PipelineHandleUVE was meant —
/// matching AssetGuidUVE's precedent for opaque ids that must not be confused with each other,
/// which matters here since the RHI has four distinct resource kinds.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct BufferHandleUVE {
    std::uint32_t value = 0;
};

/// The sentinel "no buffer" value. Never returned by a successful CreateBufferUVE() call.
inline constexpr BufferHandleUVE kInvalidBufferHandleUVE{};

[[nodiscard]] constexpr bool operator==(const BufferHandleUVE& lhs, const BufferHandleUVE& rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(const BufferHandleUVE& lhs, const BufferHandleUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Render

template <>
struct std::hash<UVE::Render::BufferHandleUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Render::BufferHandleUVE& handle) const noexcept {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};
