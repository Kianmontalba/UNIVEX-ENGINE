// univex/math/Vec.h
// -----------------------------------------------------------------------
// Minimal constexpr vector types (C++20). Deliberately small and
// dependency-free: if UNIVEX already has its own vector math, delete this
// header and typedef Vec2/Vec3/Vec4 onto the engine's own types — nothing
// else in this module depends on the implementation, only on the member
// names x/y/z/w and the free functions below.
// -----------------------------------------------------------------------
#pragma once

#include <cmath>
#include <concepts>

namespace univex::math {

struct Vec2 {
    float x{}, y{};
};

struct Vec3 {
    float x{}, y{}, z{};

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Vec4 {
    float x{}, y{}, z{}, w{};

    constexpr Vec4() = default;
    constexpr Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    [[nodiscard]] constexpr Vec3 xyz() const { return {x, y, z}; }
};

// ---- Vec3 arithmetic -----------------------------------------------------
[[nodiscard]] constexpr Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] constexpr Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] constexpr Vec3 operator-(const Vec3& a) { return {-a.x, -a.y, -a.z}; }
[[nodiscard]] constexpr Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
[[nodiscard]] constexpr Vec3 operator*(float s, const Vec3& a) { return a * s; }
[[nodiscard]] constexpr Vec3 operator/(const Vec3& a, float s) { return {a.x / s, a.y / s, a.z / s}; }

constexpr Vec3& operator+=(Vec3& a, const Vec3& b) { a = a + b; return a; }
constexpr Vec3& operator-=(Vec3& a, const Vec3& b) { a = a - b; return a; }
constexpr Vec3& operator*=(Vec3& a, float s) { a = a * s; return a; }

[[nodiscard]] constexpr float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

[[nodiscard]] inline float Length(const Vec3& a) { return std::sqrt(Dot(a, a)); }

[[nodiscard]] inline Vec3 Normalize(const Vec3& a) {
    const float len = Length(a);
    return (len > 1e-8f) ? a / len : Vec3{0.f, 0.f, 0.f};
}

// ---- Vec4 arithmetic -----------------------------------------------------
[[nodiscard]] constexpr Vec4 operator*(const Vec4& a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
[[nodiscard]] constexpr Vec4 operator+(const Vec4& a, const Vec4& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

// Perspective divide. Returns {0,0,0} for a degenerate w, so callers never
// propagate a NaN into a matrix or a draw call.
[[nodiscard]] inline Vec3 PerspectiveDivide(const Vec4& clip) {
    if (std::abs(clip.w) < 1e-8f) return {0.f, 0.f, 0.f};
    const float invW = 1.f / clip.w;
    return {clip.x * invW, clip.y * invW, clip.z * invW};
}

} // namespace univex::math
