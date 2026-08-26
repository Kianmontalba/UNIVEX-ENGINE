// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>

namespace UVE::Render {

/// Engine-native description of a reflected shader uniform's data type — never a raw GL enum, so
/// this stays meaningful across any future non-GL backend. Covers the uniform shapes
/// ShaderProgramUVE's SetFloatUVE/SetIntUVE/SetBoolUVE/SetVector3UVE/SetMatrix4x4UVE API sets and
/// keeps an explicit marker for reflected shapes that have no setter yet. Unsupported shapes must
/// not be silently collapsed into Int, because that would issue an invalid GL uniform call.
enum class ShaderDataTypeUVE : std::uint8_t {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    Int,
    Bool,
    Unsupported,
};

/// Size, in bytes, of one value of `type` (array stride for a reflected uniform array). Every
/// enumerator is handled explicitly — no default case — so a future addition to
/// ShaderDataTypeUVE fails to compile here until this function is updated too.
[[nodiscard]] std::size_t GetShaderDataTypeSizeBytesUVE(ShaderDataTypeUVE type) noexcept;

} // namespace UVE::Render
