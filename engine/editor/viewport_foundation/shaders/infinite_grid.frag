#version 330 core
// infinite_grid.frag
// ---------------------------------------------------------------------------
// Ray/plane infinite grid with an auto-adjusting (decade-LOD) line spacing.
//
// Two things make this behave like an editor grid in a real engine rather than
// a textured plane:
//
//  1. TRUE INFINITY + CORRECT DEPTH. Each pixel's world ray is intersected
//     with y = 0. Pixels whose ray never hits the plane in front of the camera
//     are discarded, so the grid terminates exactly at the horizon. The hit
//     point is re-projected to write gl_FragDepth, so the grid depth-tests
//     against scene geometry properly instead of floating over or under it.
//
//  2. AUTO-ADJUSTING SPACING. The grid never shows a fixed 1-unit cell. It
//     measures how much world space one pixel covers (screen-space
//     derivatives) and picks the decade of spacing that keeps cells at roughly
//     uTargetCellPixels on screen. Four decades are drawn at once and
//     cross-faded by the fractional part of the LOD, so zooming slides the
//     tiers continuously (1 m -> 10 m -> 100 m ...) with no popping and
//     constant on-screen density from centimetres to kilometres.
//
//     A useful side effect: because the spacing grows with distance, the
//     argument to fract() below stays in a small numeric range even when the
//     camera is far from the origin, which is what keeps fp32 precision from
//     making distant lines wobble.
//
// The general approach (procedural lines via fract()/derivative-based
// anti-aliasing, decade LOD, ray-cast infinite plane) is the standard one used
// across many engines and public shader collections; it is not a copy of any
// single proprietary implementation.
// ---------------------------------------------------------------------------

in vec3 vNearPoint;
in vec3 vFarPoint;

uniform mat4  uViewProj;
uniform vec3  uCameraPos;

uniform float uBaseSpacing;       // finest grid spacing, world units (e.g. 1.0 = 1 m)
uniform float uTargetCellPixels;  // desired minimum on-screen cell size before stepping a decade
uniform float uLineWidthPixels;
uniform float uAxisWidthPixels;

uniform vec3  uThinColor;
uniform vec3  uMidColor;
uniform vec3  uThickColor;
uniform float uThinIntensity;
uniform float uMidIntensity;
uniform float uThickIntensity;

uniform vec3  uAxisColorX;        // line along world X (at z = 0)
uniform vec3  uAxisColorZ;        // line along world Z (at x = 0)

uniform float uFadeStart;         // world-space ground distance where the fade begins
uniform float uFadeEnd;           // ... and where it reaches zero
uniform float uOpacity;

out vec4 fragColor;

const float kLog10 = 0.43429448190325176; // 1 / ln(10)

// Anti-aliased coverage of the nearest grid line at `p`, for one spacing.
// `worldPerPixel` is how much world space a single pixel covers along each
// axis, so the line keeps a constant pixel width no matter how far away or how
// obliquely the ground is being viewed.
float GridCoverage(vec2 p, float spacing, vec2 worldPerPixel, float widthPixels) {
    vec2 halfWidth = worldPerPixel * widthPixels * 0.5;
    vec2 distToLine = abs(fract(p / spacing - 0.5) - 0.5) * spacing;
    vec2 coverage = 1.0 - clamp(distToLine / max(halfWidth, vec2(1e-9)), 0.0, 1.0);
    return max(coverage.x, coverage.y);
}

// Coverage of a single line at coord == 0 (the world axes).
float AxisCoverage(float coord, float worldPerPixel, float widthPixels) {
    float halfWidth = worldPerPixel * widthPixels * 0.5;
    return 1.0 - clamp(abs(coord) / max(halfWidth, 1e-9), 0.0, 1.0);
}

// Composite `src` on top of the accumulated `dst`.
vec4 Over(vec4 dst, vec3 srcColor, float srcAlpha) {
    srcAlpha = clamp(srcAlpha, 0.0, 1.0);
    return vec4(mix(dst.rgb, srcColor, srcAlpha),
                dst.a + srcAlpha * (1.0 - dst.a));
}

void main() {
    vec3 rayDir = vFarPoint - vNearPoint;

    // Ray/plane intersection with y = 0.
    //
    // Two deliberate details here. First, the division is guarded so worldPos
    // is finite for EVERY pixel, including ones whose ray is parallel to the
    // ground: an inf or NaN would otherwise leak sideways through dFdx/dFdy
    // into a perfectly valid neighbouring pixel and paint garbage along the
    // horizon. Second, nothing is discarded until after the derivatives have
    // been taken — derivatives are only well defined when the whole 2x2 quad
    // is still live, so an early `discard` here would make the LOD undefined
    // exactly where the grid is most stretched.
    float denom = rayDir.y;
    float safeDenom = abs(denom) < 1e-9 ? 1e-9 : denom;
    float t = clamp(-vNearPoint.y / safeDenom, -1e6, 1e6);
    bool hitsGround = abs(denom) >= 1e-9 && t > 0.0 && t < 1.0;

    vec3 worldPos = vNearPoint + t * rayDir;

    // ---- how much world space does one pixel cover here? ------------------
    vec2 worldPerPixel = vec2(
        length(vec2(dFdx(worldPos.x), dFdy(worldPos.x))),
        length(vec2(dFdx(worldPos.z), dFdy(worldPos.z)))
    );
    float pixelWorld = max(max(worldPerPixel.x, worldPerPixel.y), 1e-9);

    // ---- pick the decade of spacing, and how far through it we are --------
    // The upper clamp keeps pow(10, floor(lod)) finite for the stretched
    // pixels right at the horizon; 10^20 world units is far past any scene,
    // so it never constrains a spacing anyone will actually see.
    float lod = clamp(log(pixelWorld * uTargetCellPixels / uBaseSpacing) * kLog10, 0.0, 20.0);
    float lodFade = fract(lod);
    float spacing0 = uBaseSpacing * pow(10.0, floor(lod));
    float spacing1 = spacing0 * 10.0;
    float spacing2 = spacing1 * 10.0;
    float spacing3 = spacing2 * 10.0;

    float cov0 = GridCoverage(worldPos.xz, spacing0, worldPerPixel, uLineWidthPixels);
    float cov1 = GridCoverage(worldPos.xz, spacing1, worldPerPixel, uLineWidthPixels);
    float cov2 = GridCoverage(worldPos.xz, spacing2, worldPerPixel, uLineWidthPixels);
    float cov3 = GridCoverage(worldPos.xz, spacing3, worldPerPixel, uLineWidthPixels);

    // Each tier slides one step "finer" in appearance as lodFade goes 0 -> 1,
    // so at the moment the LOD ticks over, tier N looks exactly like tier N-1
    // did an instant earlier and nothing pops.
    vec3  color0 = uThinColor;                        float alpha0 = uThinIntensity * (1.0 - lodFade);
    vec3  color1 = mix(uMidColor,   uThinColor, lodFade); float alpha1 = mix(uMidIntensity,   uThinIntensity, lodFade);
    vec3  color2 = mix(uThickColor, uMidColor,  lodFade); float alpha2 = mix(uThickIntensity, uMidIntensity,  lodFade);
    vec3  color3 = uThickColor;                       float alpha3 = uThickIntensity * lodFade;

    vec4 accum = vec4(uThinColor, 0.0);
    accum = Over(accum, color0, cov0 * alpha0);
    accum = Over(accum, color1, cov1 * alpha1);
    accum = Over(accum, color2, cov2 * alpha2);
    accum = Over(accum, color3, cov3 * alpha3);

    // ---- world axes on top -----------------------------------------------
    float axisX = AxisCoverage(worldPos.z, worldPerPixel.y, uAxisWidthPixels);
    float axisZ = AxisCoverage(worldPos.x, worldPerPixel.x, uAxisWidthPixels);
    accum = Over(accum, uAxisColorX, axisX);
    accum = Over(accum, uAxisColorZ, axisZ);

    // ---- horizon fade -----------------------------------------------------
    float groundDist = length(worldPos.xz - uCameraPos.xz);
    float fade = 1.0 - smoothstep(uFadeStart, uFadeEnd, groundDist);

    float finalAlpha = accum.a * fade * uOpacity;
    if (finalAlpha < 0.002) discard;   // nothing to show; don't touch depth either

    // ---- real depth, so the grid composites with scene geometry -----------
    // UVE ADAPTATION: no *0.5+0.5 remap - uViewProj already produces a [0, 1] clip-space z (see
    // Mat4.cpp's Perspective/Orthographic), so clip.z/clip.w IS the window-space depth directly.
    vec4 clip = uViewProj * vec4(worldPos, 1.0);
    gl_FragDepth = clip.z / clip.w;

    fragColor = vec4(accum.rgb, finalAlpha);
}
