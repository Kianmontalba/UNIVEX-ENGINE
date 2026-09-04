#version 450 core

#ifdef VERTEX_SHADER
// Fullscreen triangle via the vertex-ID trick: no vertex buffer is required.
out vec2 vTexCoord;

void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vTexCoord = position * 0.5;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uDepthTexture;
// The projection matrix and its inverse (Math::TryInverseUVE(), Phase 2d), NOT the combined
// view-projection or its inverse - reconstruction below stays entirely in view space, so only the
// projection step needs undoing (uInverseProjection) and redoing (uProjection, to re-project each
// hemisphere sample and look up its screen position - computed once on the CPU per frame rather
// than inverting uInverseProjection again per pixel).
uniform mat4 uInverseProjection;
uniform mat4 uProjection;
uniform float uRadius;
uniform float uBias;
uniform float uIntensity;

// A fixed 12-tap hemisphere kernel (offline-generated, hemisphere-distributed, biased toward the
// origin so more samples land close to the shaded point) stands in for the noise-texture-driven
// per-pixel kernel rotation real engines typically use - HashUVE() below provides the per-pixel
// rotation instead, trading a small amount of dither/banding for not needing a vendored noise
// texture asset. Reasonable for this engine's scope; a dedicated rotation-noise texture is a
// future quality upgrade, not a correctness requirement.
const int kKernelSizeUVE = 12;
const vec3 kKernelUVE[12] = vec3[](
    vec3(-0.0557, 0.0476, 0.0681),
    vec3(0.0117, -0.0727, 0.0766),
    vec3(0.0931, -0.0750, 0.0365),
    vec3(0.1465, -0.0523, 0.0149),
    vec3(0.1313, 0.0982, 0.1146),
    vec3(0.0901, 0.2384, 0.0267),
    vec3(-0.2553, -0.1976, 0.0374),
    vec3(-0.2171, -0.3242, 0.1129),
    vec3(0.2547, -0.2537, 0.3475),
    vec3(0.4424, 0.3262, 0.2557),
    vec3(0.3698, -0.5432, 0.3062),
    vec3(-0.1842, 0.7316, 0.4049)
);

// Reconstructs a view-space position from a depth-buffer sample. GL's own fixed depth-range
// mapping (depth = 0.5 * ndc.z + 0.5, default glDepthRange(0,1)) is inverted first to recover the
// actual clip.z/clip.w ratio the projection matrix produced (this engine's PerspectiveUVE uses a
// [0,1] "Vulkan-style" clip-space z, not OpenGL's traditional [-1,1] - see its doc comment), then
// the standard homogeneous-divide trick recovers view-space xyz regardless of that convention.
vec3 ReconstructViewPositionUVE(vec2 uv, float depthSample) {
    vec3 ndc = vec3(uv * 2.0 - 1.0, 2.0 * depthSample - 1.0);
    vec4 clipPos = vec4(ndc, 1.0);
    vec4 viewPos = uInverseProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

float HashUVE(vec2 value) {
    return fract(sin(dot(value, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    float centerDepth = texture(uDepthTexture, vTexCoord).r;
    if (centerDepth >= 1.0) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Background/far plane: never occluded.
        return;
    }
    vec3 centerViewPos = ReconstructViewPositionUVE(vTexCoord, centerDepth);

    // View-space normal from screen-space derivatives of the reconstructed position - avoids
    // needing a dedicated normal G-buffer in this forward renderer.
    // dFdx/dFdy face the +Z view direction (camera looks down -Z) for a screen-space quad that
    // covers increasing x left-to-right and increasing y bottom-to-top, matching this engine's
    // vTexCoord/gl_Position convention - no winding-based flip needed, unlike a mesh normal.
    vec3 viewNormal = normalize(cross(dFdx(centerViewPos), dFdy(centerViewPos)));
    if (viewNormal.z < 0.0) {
        viewNormal = -viewNormal;
    }

    float rotationAngle = HashUVE(vTexCoord) * 6.28318530718;
    float cosAngle = cos(rotationAngle);
    float sinAngle = sin(rotationAngle);
    vec3 randomTangent = normalize(vec3(cosAngle, sinAngle, 0.0));
    vec3 tangent = normalize(randomTangent - viewNormal * dot(randomTangent, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 tbn = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    for (int sampleIndex = 0; sampleIndex < kKernelSizeUVE; ++sampleIndex) {
        vec3 samplePos = centerViewPos + (tbn * kKernelUVE[sampleIndex]) * uRadius;

        // Re-project the sample point with uProjection to look up what's actually in the depth
        // buffer at that screen location.
        vec4 sampleClip = uProjection * vec4(samplePos, 1.0);
        vec2 sampleUv = (sampleClip.xy / sampleClip.w) * 0.5 + 0.5;
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 || sampleUv.y < 0.0 || sampleUv.y > 1.0) {
            continue;
        }

        float sampledDepth = texture(uDepthTexture, sampleUv).r;
        vec3 sampledViewPos = ReconstructViewPositionUVE(sampleUv, sampledDepth);

        float rangeCheck = smoothstep(0.0, 1.0, uRadius / max(abs(centerViewPos.z - sampledViewPos.z), 0.0001));
        occlusion += (sampledViewPos.z >= samplePos.z + uBias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(kKernelSizeUVE)) * uIntensity;
    FragColor = vec4(vec3(clamp(occlusion, 0.0, 1.0)), 1.0);
}
#endif
