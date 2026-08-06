// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>

namespace UVE::Render {

/// Opaque handle to a shader resource created via IRenderDeviceUVE::CreateShaderUVE(). See
/// BufferHandleUVE's doc comment for why this is a small wrapper struct rather than a bare
/// std::uint32_t alias.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct ShaderHandleUVE {
    std::uint32_t value = 0;
};

/// The sentinel "no shader" value. Never returned by a successful CreateShaderUVE() call.
inline constexpr ShaderHandleUVE kInvalidShaderHandleUVE{};

[[nodiscard]] constexpr bool operator==(const ShaderHandleUVE& lhs, const ShaderHandleUVE& rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(const ShaderHandleUVE& lhs, const ShaderHandleUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Render

template <>
struct std::hash<UVE::Render::ShaderHandleUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Render::ShaderHandleUVE& handle) const noexcept {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};
