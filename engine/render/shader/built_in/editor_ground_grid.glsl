#version 450 core

#ifdef VERTEX_SHADER
// Fullscreen triangle via the vertex-ID trick, matching fullscreen_quad.glsl: no vertex buffer is
// required. Rather than pushing a large ground quad through the pipeline - which is only ever
// "large", never infinite, and always has an edge to hide - this rebuilds a world-space view ray
// per pixel and intersects it with the ground plane in the fragment shader. The plane is then
// mathematically infinite: it ends at the true horizon, not at a quad boundary.
//
// The two unprojected points are exact under linear interpolation: every vertex of the fullscreen
// triangle has gl_Position.w == 1, so perspective-correct interpolation reduces to linear, and
// both hit points are affine functions of screen position.
//
// The near/far clip depths unprojected here are 0.0 and 1.0, not -1.0 and 1.0: this engine's
// projection matrices map the near plane to depth 0 and the far plane to depth 1 (see
// Matrix4x4UVE::PerspectiveUVE's documented Vulkan-style range), so `t` below lands in (0, 1)
// exactly when the ray meets the ground between the two clip planes.
out vec3 vNearPoint;
out vec3 vFarPoint;

uniform mat4 uInverseViewProjection;

vec3 UnprojectUVE(vec2 clipXY, float clipZ) {
    vec4 unprojected = uInverseViewProjection * vec4(clipXY, clipZ, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vec2 clipPosition = position * 2.0 - 1.0;
    vNearPoint = UnprojectUVE(clipPosition, 0.0);
    vFarPoint = UnprojectUVE(clipPosition, 1.0);
    gl_Position = vec4(clipPosition, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
// Ray/plane infinite ground grid with auto-adjusting (decade-LOD) spacing.
//
// Two things make this behave like an editor grid rather than a textured plane:
//
//  1. TRUE INFINITY WITH CORRECT DEPTH. Each pixel's world ray is intersected with y = 0. Pixels
//     whose ray never meets the plane in front of the camera are discarded, so the grid terminates
//     exactly at the horizon. The hit point is re-projected to write gl_FragDepth, so the grid
//     depth-tests against scene geometry properly instead of floating over or under it.
//
//  2. AUTO-ADJUSTING SPACING. The grid never draws a fixed 1-unit cell. It measures how much world
//     space one pixel covers (screen-space derivatives) and picks the decade of spacing that keeps
//     cells near uTargetCellPixels on screen. Four decades are drawn at once and cross-faded by the
//     fractional part of the LOD, so zooming slides the tiers continuously (1 m -> 10 m -> 100 m)
//     with no popping and constant on-screen density from centimetres to kilometres.
//
//     A useful side effect: because spacing grows with distance, the argument to fract() stays in a
//     small numeric range even far from the origin, which keeps fp32 precision from making distant
//     lines wobble.
//
// The building blocks - a ray-cast infinite ground plane, procedural lines via fract() with
// derivative-based anti-aliasing, and decade LOD with cross-fade - are the standard approaches
// described in public shader literature, not a copy of any single engine's source.
in vec3 vNearPoint;
in vec3 vFarPoint;

out vec4 FragColor;

uniform mat4 uViewProjection;
uniform vec3 uCameraPosition;

uniform float uBaseSpacing;
uniform float uTargetCellPixels;
uniform float uLineWidthPixels;
uniform float uAxisWidthPixels;

uniform vec3 uThinColor;
uniform vec3 uMidColor;
uniform vec3 uThickColor;
uniform float uThinIntensity;
uniform float uMidIntensity;
uniform float uThickIntensity;

uniform vec3 uAxisColorX;
uniform vec3 uAxisColorZ;

uniform float uFadeStart;
uniform float uFadeEnd;
uniform float uOpacity;

const float kInverseLogTenUVE = 0.43429448190325176;

// Anti-aliased coverage of the nearest grid line at `groundPosition`, for one spacing.
// `worldPerPixel` is how much world space a single pixel covers along each axis, so a line keeps a
// constant pixel width however far away or however obliquely the ground is viewed.
float GridCoverageUVE(vec2 groundPosition, float spacing, vec2 worldPerPixel, float widthPixels) {
    vec2 halfWidth = worldPerPixel * widthPixels * 0.5;
    vec2 distanceToLine = abs(fract(groundPosition / spacing - 0.5) - 0.5) * spacing;
    vec2 coverage = 1.0 - clamp(distanceToLine / max(halfWidth, vec2(1e-9)), 0.0, 1.0);
    return max(coverage.x, coverage.y);
}

// Coverage of the single line at coordinate 0 - the world axes.
float AxisCoverageUVE(float coordinate, float worldPerPixel, float widthPixels) {
    float halfWidth = worldPerPixel * widthPixels * 0.5;
    return 1.0 - clamp(abs(coordinate) / max(halfWidth, 1e-9), 0.0, 1.0);
}

vec4 OverUVE(vec4 destination, vec3 sourceColor, float sourceAlpha) {
    sourceAlpha = clamp(sourceAlpha, 0.0, 1.0);
    return vec4(mix(destination.rgb, sourceColor, sourceAlpha),
                destination.a + sourceAlpha * (1.0 - destination.a));
}

void main() {
    vec3 rayDirection = vFarPoint - vNearPoint;

    // Ray/plane intersection with y = 0. Two deliberate details: the division is guarded so
    // worldPosition stays finite for EVERY pixel, including rays parallel to the ground - an inf or
    // NaN would otherwise leak sideways through dFdx/dFdy into a perfectly valid neighbouring pixel
    // and paint garbage along the horizon. And nothing is discarded until after the derivatives have
    // been taken, because derivatives are only well defined while the whole 2x2 quad is still live;
    // an early discard would make the LOD undefined exactly where the grid is most stretched.
    float denominator = rayDirection.y;
    float safeDenominator = abs(denominator) < 1e-9 ? 1e-9 : denominator;
    float t = clamp(-vNearPoint.y / safeDenominator, -1e6, 1e6);
    bool hitsGround = abs(denominator) >= 1e-9 && t > 0.0 && t < 1.0;

    vec3 worldPosition = vNearPoint + t * rayDirection;

    vec2 worldPerPixel = vec2(length(vec2(dFdx(worldPosition.x), dFdy(worldPosition.x))),
                              length(vec2(dFdx(worldPosition.z), dFdy(worldPosition.z))));
    float pixelWorld = max(max(worldPerPixel.x, worldPerPixel.y), 1e-9);

    // Pick the decade of spacing and how far through it we are. The upper clamp keeps
    // pow(10, floor(lod)) finite for the stretched pixels right at the horizon; 10^20 world units is
    // far past any scene, so it never constrains a spacing anyone will actually see.
    float lod = clamp(log(pixelWorld * uTargetCellPixels / uBaseSpacing) * kInverseLogTenUVE, 0.0, 20.0);
    float lodFade = fract(lod);
    float spacing0 = uBaseSpacing * pow(10.0, floor(lod));
    float spacing1 = spacing0 * 10.0;
    float spacing2 = spacing1 * 10.0;
    float spacing3 = spacing2 * 10.0;

    float coverage0 = GridCoverageUVE(worldPosition.xz, spacing0, worldPerPixel, uLineWidthPixels);
    float coverage1 = GridCoverageUVE(worldPosition.xz, spacing1, worldPerPixel, uLineWidthPixels);
    float coverage2 = GridCoverageUVE(worldPosition.xz, spacing2, worldPerPixel, uLineWidthPixels);
    float coverage3 = GridCoverageUVE(worldPosition.xz, spacing3, worldPerPixel, uLineWidthPixels);

    // Each tier slides one step finer in appearance as lodFade goes 0 -> 1, so at the moment the LOD
    // ticks over, tier N looks exactly like tier N-1 did an instant earlier and nothing pops.
    vec3 color0 = uThinColor;
    float alpha0 = uThinIntensity * (1.0 - lodFade);
    vec3 color1 = mix(uMidColor, uThinColor, lodFade);
    float alpha1 = mix(uMidIntensity, uThinIntensity, lodFade);
    vec3 color2 = mix(uThickColor, uMidColor, lodFade);
    float alpha2 = mix(uThickIntensity, uMidIntensity, lodFade);
    vec3 color3 = uThickColor;
    float alpha3 = uThickIntensity * lodFade;

    vec4 accumulated = vec4(uThinColor, 0.0);
    accumulated = OverUVE(accumulated, color0, coverage0 * alpha0);
    accumulated = OverUVE(accumulated, color1, coverage1 * alpha1);
    accumulated = OverUVE(accumulated, color2, coverage2 * alpha2);
    accumulated = OverUVE(accumulated, color3, coverage3 * alpha3);

    float axisX = AxisCoverageUVE(worldPosition.z, worldPerPixel.y, uAxisWidthPixels);
    float axisZ = AxisCoverageUVE(worldPosition.x, worldPerPixel.x, uAxisWidthPixels);
    accumulated = OverUVE(accumulated, uAxisColorX, axisX);
    accumulated = OverUVE(accumulated, uAxisColorZ, axisZ);

    float groundDistance = length(worldPosition.xz - uCameraPosition.xz);
    float fade = 1.0 - smoothstep(uFadeStart, uFadeEnd, groundDistance);

    float finalAlpha = accumulated.a * fade * uOpacity;
    if (!hitsGround || finalAlpha < 0.002) {
        discard;
    }

    // Real depth, so the grid composites with scene geometry. gl_FragDepth is a window-space depth,
    // and with the default glDepthRange the rasterizer maps NDC z through the same (z * 0.5 + 0.5)
    // transform used here - so grid fragments and mesh fragments land on one comparable scale.
    vec4 clipPosition = uViewProjection * vec4(worldPosition, 1.0);
    gl_FragDepth = (clipPosition.z / clipPosition.w) * 0.5 + 0.5;

    FragColor = vec4(accumulated.rgb, finalAlpha);
}
#endif
