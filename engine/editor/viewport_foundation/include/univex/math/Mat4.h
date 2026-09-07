// univex/math/Mat4.h
// -----------------------------------------------------------------------
// Column-major 4x4 matrix — element (row r, column c) lives at m[c*4 + r],
// the exact layout glUniformMatrix4fv(..., transpose = GL_FALSE, ...)
// expects, so a Mat4 can be handed to the driver with no repacking.
//
// The infinite grid needs a real inverse (it reconstructs a world-space
// ray per pixel by unprojecting the clip-space corners), so this is a
// general 4x4 inverse, not the "affine transpose" shortcut that only works
// for rigid transforms.
// -----------------------------------------------------------------------
#pragma once

#include <array>
#include <optional>

#include "univex/math/Vec.h"

namespace univex::math {

class Mat4 {
public:
    // Column-major storage: m_[c * 4 + r].
    std::array<float, 16> m{};

    constexpr Mat4() = default;
    constexpr explicit Mat4(const std::array<float, 16>& columnMajor) : m(columnMajor) {}

    [[nodiscard]] static constexpr Mat4 Identity() {
        return Mat4(std::array<float, 16>{1.f, 0.f, 0.f, 0.f,
                                          0.f, 1.f, 0.f, 0.f,
                                          0.f, 0.f, 1.f, 0.f,
                                          0.f, 0.f, 0.f, 1.f});
    }

    [[nodiscard]] constexpr float At(int row, int col) const { return m[static_cast<std::size_t>(col * 4 + row)]; }
    constexpr void Set(int row, int col, float v) { m[static_cast<std::size_t>(col * 4 + row)] = v; }

    [[nodiscard]] const float* Data() const { return m.data(); }

    // Right-handed perspective projection mapping the view frustum to
    // OpenGL clip space (z in [-w, w]). fovY in radians.
    [[nodiscard]] static Mat4 Perspective(float fovYRadians, float aspect, float nearZ, float farZ);

    // Right-handed orthographic projection. `halfHeight` is half the
    // vertical extent of the view volume in world units; the horizontal
    // extent follows from `aspect`. Same clip-space convention as
    // Perspective(), so both feed the same shaders unchanged.
    [[nodiscard]] static Mat4 Orthographic(float halfHeight, float aspect, float nearZ, float farZ);

    // Right-handed look-at view matrix; the camera looks down -Z in view
    // space, matching the Perspective() above.
    [[nodiscard]] static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

    // Returns a * b, i.e. "apply b first, then a" — matching the usual
    // `gl_Position = proj * view * vec4(worldPos, 1.0)` reading order.
    [[nodiscard]] static Mat4 Multiply(const Mat4& a, const Mat4& b);

    // General 4x4 inverse. Returns nullopt for a singular matrix rather
    // than silently producing infinities — the caller decides what to do.
    [[nodiscard]] static std::optional<Mat4> Inverse(const Mat4& mat);

    [[nodiscard]] Vec4 Transform(const Vec4& v) const;

    // Applies the rotation/scale part only (ignores translation) — for
    // turning a direction, such as a nav-gizmo axis, into view space.
    [[nodiscard]] Vec3 TransformDirection(const Vec3& v) const;
};

[[nodiscard]] inline Mat4 operator*(const Mat4& a, const Mat4& b) { return Mat4::Multiply(a, b); }

} // namespace univex::math
