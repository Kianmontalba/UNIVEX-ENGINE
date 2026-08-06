// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>

namespace UVE::Render {

/// Opaque handle to a pipeline state object created via IRenderDeviceUVE::CreatePipelineUVE().
/// See BufferHandleUVE's doc comment for why this is a small wrapper struct rather than a bare
/// std::uint32_t alias.
/// Thread-safety: value type; safe to copy/compare/hash freely, no shared state.
struct PipelineHandleUVE {
    std::uint32_t value = 0;
};

/// The sentinel "no pipeline" value. Never returned by a successful CreatePipelineUVE() call.
inline constexpr PipelineHandleUVE kInvalidPipelineHandleUVE{};

[[nodiscard]] constexpr bool operator==(const PipelineHandleUVE& lhs, const PipelineHandleUVE& rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(const PipelineHandleUVE& lhs, const PipelineHandleUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Render

template <>
struct std::hash<UVE::Render::PipelineHandleUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Render::PipelineHandleUVE& handle) const noexcept {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};
