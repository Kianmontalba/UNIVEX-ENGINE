// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>

namespace UVE::Render {

/// Opaque handle to a GPU texture resource created via IRenderDeviceUVE::CreateTextureUVE().
/// See BufferHandleUVE's doc comment for why this is a small wrapper struct rather than a bare
/// std::uint32_t alias.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct TextureHandleUVE {
    std::uint32_t value = 0;
};

/// The sentinel "no texture" value. Never returned by a successful CreateTextureUVE() call; also
/// used as RenderPassDescUVE::depthAttachment for a color-only render pass.
inline constexpr TextureHandleUVE kInvalidTextureHandleUVE{};

[[nodiscard]] constexpr bool operator==(const TextureHandleUVE& lhs, const TextureHandleUVE& rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(const TextureHandleUVE& lhs, const TextureHandleUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Render

template <>
struct std::hash<UVE::Render::TextureHandleUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Render::TextureHandleUVE& handle) const noexcept {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};
