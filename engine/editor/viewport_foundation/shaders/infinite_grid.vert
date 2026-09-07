#version 330 core
// infinite_grid.vert
// ---------------------------------------------------------------------------
// One fullscreen triangle. Rather than pushing a big ground quad through the
// pipeline (which is only ever "large", never infinite, and always has an edge
// to hide), this rebuilds a world-space view ray per pixel and intersects it
// with the ground plane in the fragment shader. That makes the plane
// mathematically infinite: it ends at the true horizon, not at a quad boundary.
//
// The two unprojected points below are exact under linear interpolation: every
// vertex of the fullscreen triangle has gl_Position.w == 1, so the rasterizer's
// perspective-correct interpolation reduces to linear, and both the near-plane
// and far-plane hit points are affine functions of screen position.
// ---------------------------------------------------------------------------

layout(location = 0) in vec2 aClipPos;   // fullscreen triangle, clip-space XY

uniform mat4 uInvViewProj;

out vec3 vNearPoint;   // where this pixel's ray pierces the near plane, in world space
out vec3 vFarPoint;    // ... and the far plane

vec3 Unproject(vec2 clipXY, float clipZ) {
    vec4 p = uInvViewProj * vec4(clipXY, clipZ, 1.0);
    return p.xyz / p.w;
}

void main() {
    // UVE ADAPTATION: near = 0.0, not -1.0 - this engine's projection matrices (Mat4::Perspective/
    // Orthographic, adapted in Mat4.cpp) target a [0, 1] clip-space z, matching Matrix4x4UVE and
    // this engine's glClipControl(GL_ZERO_TO_ONE) call, so the near plane sits at clip z = 0.
    vNearPoint = Unproject(aClipPos, 0.0);
    vFarPoint  = Unproject(aClipPos, 1.0);
    gl_Position = vec4(aClipPos, 0.0, 1.0);
}
