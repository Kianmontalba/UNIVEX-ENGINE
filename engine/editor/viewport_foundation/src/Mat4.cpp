#include "univex/math/Mat4.h"

#include <cmath>

namespace univex::math {

// UVE ADAPTATION (the one deliberate change from the original univex_viewport_gl source): the
// z-row of both projections below targets a [0, 1] clip-space z instead of classic OpenGL's
// [-1, 1], to match this engine's Matrix4x4UVE convention (docs/CODING_STANDARDS.md, "Matrix
// convention") and its glClipControl(GL_ZERO_TO_ONE) call (gl_render_device_uve.cpp) - the two
// have to agree, since the grid/gizmo passes built from these matrices share one depth buffer
// with every mesh the rest of the engine draws. Everything else in this file - LookAt, Multiply,
// Inverse, TransformDirection - is untouched.
Mat4 Mat4::Perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
    const float f = 1.f / std::tan(fovYRadians * 0.5f);
    const float nf = 1.f / (nearZ - farZ);
    return Mat4(std::array<float, 16>{
        f / aspect, 0.f, 0.f, 0.f,
        0.f, f, 0.f, 0.f,
        0.f, 0.f, farZ * nf, -1.f,
        0.f, 0.f, farZ * nearZ * nf, 0.f,
    });
}

Mat4 Mat4::Orthographic(float halfHeight, float aspect, float nearZ, float farZ) {
    const float halfWidth = halfHeight * aspect;
    const float rl = 1.f / (halfWidth * 2.f);
    const float tb = 1.f / (halfHeight * 2.f);
    const float fn = 1.f / (farZ - nearZ);
    return Mat4(std::array<float, 16>{
        2.f * rl, 0.f, 0.f, 0.f,
        0.f, 2.f * tb, 0.f, 0.f,
        0.f, 0.f, -fn, 0.f,
        0.f, 0.f, -nearZ * fn, 1.f,
    });
}

Vec3 Mat4::TransformDirection(const Vec3& v) const {
    return Vec3{
        At(0, 0) * v.x + At(0, 1) * v.y + At(0, 2) * v.z,
        At(1, 0) * v.x + At(1, 1) * v.y + At(1, 2) * v.z,
        At(2, 0) * v.x + At(2, 1) * v.y + At(2, 2) * v.z,
    };
}

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const Vec3 zAxis = Normalize(eye - target); // camera looks down -Z in view space
    const Vec3 xAxis = Normalize(Cross(up, zAxis));
    const Vec3 yAxis = Cross(zAxis, xAxis);
    return Mat4(std::array<float, 16>{
        xAxis.x, yAxis.x, zAxis.x, 0.f,
        xAxis.y, yAxis.y, zAxis.y, 0.f,
        xAxis.z, yAxis.z, zAxis.z, 0.f,
        -Dot(xAxis, eye), -Dot(yAxis, eye), -Dot(zAxis, eye), 1.f,
    });
}

Mat4 Mat4::Multiply(const Mat4& a, const Mat4& b) {
    Mat4 out;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k) sum += a.At(r, k) * b.At(k, c);
            out.Set(r, c, sum);
        }
    }
    return out;
}

Vec4 Mat4::Transform(const Vec4& v) const {
    return Vec4{
        At(0, 0) * v.x + At(0, 1) * v.y + At(0, 2) * v.z + At(0, 3) * v.w,
        At(1, 0) * v.x + At(1, 1) * v.y + At(1, 2) * v.z + At(1, 3) * v.w,
        At(2, 0) * v.x + At(2, 1) * v.y + At(2, 2) * v.z + At(2, 3) * v.w,
        At(3, 0) * v.x + At(3, 1) * v.y + At(3, 2) * v.z + At(3, 3) * v.w,
    };
}

std::optional<Mat4> Mat4::Inverse(const Mat4& mat) {
    // Standard cofactor-expansion inverse, written against the flat
    // column-major array. Index i = c*4 + r throughout.
    const std::array<float, 16>& a = mat.m;
    std::array<float, 16> inv{};

    inv[0]  =  a[5]*a[10]*a[15] - a[5]*a[11]*a[14] - a[9]*a[6]*a[15]
             + a[9]*a[7]*a[14] + a[13]*a[6]*a[11] - a[13]*a[7]*a[10];
    inv[4]  = -a[4]*a[10]*a[15] + a[4]*a[11]*a[14] + a[8]*a[6]*a[15]
             - a[8]*a[7]*a[14] - a[12]*a[6]*a[11] + a[12]*a[7]*a[10];
    inv[8]  =  a[4]*a[9]*a[15] - a[4]*a[11]*a[13] - a[8]*a[5]*a[15]
             + a[8]*a[7]*a[13] + a[12]*a[5]*a[11] - a[12]*a[7]*a[9];
    inv[12] = -a[4]*a[9]*a[14] + a[4]*a[10]*a[13] + a[8]*a[5]*a[14]
             - a[8]*a[6]*a[13] - a[12]*a[5]*a[10] + a[12]*a[6]*a[9];
    inv[1]  = -a[1]*a[10]*a[15] + a[1]*a[11]*a[14] + a[9]*a[2]*a[15]
             - a[9]*a[3]*a[14] - a[13]*a[2]*a[11] + a[13]*a[3]*a[10];
    inv[5]  =  a[0]*a[10]*a[15] - a[0]*a[11]*a[14] - a[8]*a[2]*a[15]
             + a[8]*a[3]*a[14] + a[12]*a[2]*a[11] - a[12]*a[3]*a[10];
    inv[9]  = -a[0]*a[9]*a[15] + a[0]*a[11]*a[13] + a[8]*a[1]*a[15]
             - a[8]*a[3]*a[13] - a[12]*a[1]*a[11] + a[12]*a[3]*a[9];
    inv[13] =  a[0]*a[9]*a[14] - a[0]*a[10]*a[13] - a[8]*a[1]*a[14]
             + a[8]*a[2]*a[13] + a[12]*a[1]*a[10] - a[12]*a[2]*a[9];
    inv[2]  =  a[1]*a[6]*a[15] - a[1]*a[7]*a[14] - a[5]*a[2]*a[15]
             + a[5]*a[3]*a[14] + a[13]*a[2]*a[7] - a[13]*a[3]*a[6];
    inv[6]  = -a[0]*a[6]*a[15] + a[0]*a[7]*a[14] + a[4]*a[2]*a[15]
             - a[4]*a[3]*a[14] - a[12]*a[2]*a[7] + a[12]*a[3]*a[6];
    inv[10] =  a[0]*a[5]*a[15] - a[0]*a[7]*a[13] - a[4]*a[1]*a[15]
             + a[4]*a[3]*a[13] + a[12]*a[1]*a[7] - a[12]*a[3]*a[5];
    inv[14] = -a[0]*a[5]*a[14] + a[0]*a[6]*a[13] + a[4]*a[1]*a[14]
             - a[4]*a[2]*a[13] - a[12]*a[1]*a[6] + a[12]*a[2]*a[5];
    inv[3]  = -a[1]*a[6]*a[11] + a[1]*a[7]*a[10] + a[5]*a[2]*a[11]
             - a[5]*a[3]*a[10] - a[9]*a[2]*a[7] + a[9]*a[3]*a[6];
    inv[7]  =  a[0]*a[6]*a[11] - a[0]*a[7]*a[10] - a[4]*a[2]*a[11]
             + a[4]*a[3]*a[10] + a[8]*a[2]*a[7] - a[8]*a[3]*a[6];
    inv[11] = -a[0]*a[5]*a[11] + a[0]*a[7]*a[9] + a[4]*a[1]*a[11]
             - a[4]*a[3]*a[9] - a[8]*a[1]*a[7] + a[8]*a[3]*a[5];
    inv[15] =  a[0]*a[5]*a[10] - a[0]*a[6]*a[9] - a[4]*a[1]*a[10]
             + a[4]*a[2]*a[9] + a[8]*a[1]*a[6] - a[8]*a[2]*a[5];

    const float det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];
    if (std::abs(det) < 1e-12f) return std::nullopt;

    const float invDet = 1.f / det;
    for (float& v : inv) v *= invDet;
    return Mat4(inv);
}

} // namespace univex::math
