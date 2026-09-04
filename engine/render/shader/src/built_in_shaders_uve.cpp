// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


// Every constant below is a raw string literal transcribed byte-for-byte from its corresponding
// .glsl file under engine/render/shader/built_in/ - kept in sync by convention, enforced by
// tests/render/shader/built_in_shaders_parity_uve_tests.cpp (reads the real file and asserts
// exact equality against the constant here). Used automatically by ShaderManagerUVE as a fallback
// when the corresponding virtual path isn't reachable (see ShaderProgramDescUVE's doc comment).

#include "uve/render/shader/built_in_shaders_uve.h"

namespace UVE::Render::Shader::BuiltIn {

const std::string_view kBasic2DSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uProjection;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uModel * vec4(aPosition, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vTexCoord;
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
#endif
)GLSLSRC";

const std::string_view kBasic3DSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uViewProjection;

void main() {
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

uniform vec3 uColor;

void main() {
    FragColor = vec4(uColor, 1.0);
}
#endif
)GLSLSRC";

const std::string_view kBasic3DTexturedSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uViewProjection;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vTexCoord;
out vec4 FragColor;

// Placeholder only - no CreateTextureUVE-backed binding exists yet this increment
// (MaterialSystemUVE, a future increment, is what actually binds a real texture here).
uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, vTexCoord);
}
#endif
)GLSLSRC";

const std::string_view kFullscreenQuadSource = R"GLSLSRC(#version 450 core

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

uniform sampler2D uSourceTexture;

vec3 AcesToneMapUVE(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdrColor = max(texture(uSourceTexture, vTexCoord).rgb, vec3(0.0));
    FragColor = vec4(AcesToneMapUVE(hdrColor), 1.0);
}
#endif
)GLSLSRC";

const std::string_view kShadowDepthSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;

void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
void main() {
    // Depth-only pass: no color attachment bound, nothing to write.
}
#endif
)GLSLSRC";

const std::string_view kLitShadowed3DSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aTangent;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vWorldTangent;
out float vTangentHandedness;
out vec2 vTexCoord;
out vec4 vLightSpacePosition;
out vec4 vLightSpacePositions[3];

uniform mat4 uModel;
// Transpose(inverse(uModel)): correctly transforms normals under non-uniform scale, unlike
// uModel itself (which only preserves normal direction for uniform scale / rigid transforms).
// Tangents are still transformed with uModel directly - that IS the correct convention for a
// surface-parameterization vector, unlike a normal.
uniform mat4 uNormalMatrix;
uniform mat4 uViewProjection;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uLightSpaceMatrices[3];

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = mat3(uNormalMatrix) * aNormal;
    vWorldTangent = mat3(uModel) * aTangent.xyz;
    vTangentHandedness = aTangent.w;
    vTexCoord = aTexCoord;
    vLightSpacePosition = uLightSpaceMatrix * worldPosition;
    for (int cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex) {
        vLightSpacePositions[cascadeIndex] = uLightSpaceMatrices[cascadeIndex] * worldPosition;
    }
    gl_Position = uViewProjection * worldPosition;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vWorldTangent;
in float vTangentHandedness;
in vec2 vTexCoord;
in vec4 vLightSpacePosition;
in vec4 vLightSpacePositions[3];
out vec4 FragColor;

struct LightUVE {
    int type; // 0 = Directional, 1 = Point, 2 = Spot
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float spotAngleDegrees;
};

uniform LightUVE uLights[4];
uniform vec3 uAmbientColor;
uniform vec3 uViewPosition;
uniform vec3 uAlbedoColor;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uEmissiveColor;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uAOTexture;
// Legacy Increment 27 pair retained for project-authored shaders and direct single-map tests.
uniform sampler2D uShadowMapTexture;
uniform mat4 uLightSpaceMatrix;
uniform int uShadowPcfKernelRadius;
// Increment 30 fixed three-cascade directional shadow contract.
uniform sampler2D uShadowMapTextures[3];
uniform float uShadowCascadeSplits[3];
uniform int uShadowCascadeCount;
// Increment 31: fraction of each non-final cascade depth interval used to cross-fade into the next.
uniform float uShadowCascadeBlendRatio;

const float kPiUVE = 3.14159265359;
const float kBrdfEpsilonUVE = 0.0001;

vec3 SafeNormalizeUVE(vec3 value) {
    return value / max(length(value), kBrdfEpsilonUVE);
}

float DistributionGgxUVE(float normalDotHalf, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float normalDotHalfSquared = normalDotHalf * normalDotHalf;
    float denominator = normalDotHalfSquared * (alphaSquared - 1.0) + 1.0;
    return alphaSquared / max(kPiUVE * denominator * denominator, kBrdfEpsilonUVE);
}

float GeometrySchlickGgxUVE(float normalDotDirection, float roughness) {
    float alpha = roughness * roughness;
    float k = ((alpha + 1.0) * (alpha + 1.0)) * 0.125;
    return normalDotDirection / max(normalDotDirection * (1.0 - k) + k, kBrdfEpsilonUVE);
}

float GeometrySmithUVE(float normalDotView, float normalDotLight, float roughness) {
    return GeometrySchlickGgxUVE(normalDotView, roughness) *
           GeometrySchlickGgxUVE(normalDotLight, roughness);
}

vec3 FresnelSchlickUVE(float halfDotView, vec3 baseReflectance) {
    return baseReflectance + (vec3(1.0) - baseReflectance) * pow(1.0 - halfDotView, 5.0);
}

float SampleCascadeDepthUVE(int cascadeIndex, vec2 texCoord) {
    if (cascadeIndex == 0) {
        return texture(uShadowMapTextures[0], texCoord).r;
    }
    if (cascadeIndex == 1) {
        return texture(uShadowMapTextures[1], texCoord).r;
    }
    return texture(uShadowMapTextures[2], texCoord).r;
}

vec2 CascadeTexelSizeUVE(int cascadeIndex) {
    if (cascadeIndex == 0) {
        return 1.0 / vec2(textureSize(uShadowMapTextures[0], 0));
    }
    if (cascadeIndex == 1) {
        return 1.0 / vec2(textureSize(uShadowMapTextures[1], 0));
    }
    return 1.0 / vec2(textureSize(uShadowMapTextures[2], 0));
}

vec4 CascadeLightSpacePositionUVE(int cascadeIndex) {
    if (cascadeIndex == 0) {
        return vLightSpacePositions[0];
    }
    if (cascadeIndex == 1) {
        return vLightSpacePositions[1];
    }
    return vLightSpacePositions[2];
}

float ShadowFactorFromPositionUVE(vec4 lightSpacePosition, vec3 normal, vec3 lightDirection, int cascadeIndex) {
    if (abs(lightSpacePosition.w) <= 0.0001) {
        return 1.0;
    }

    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0 ||
        projected.z <= 0.0 || projected.z >= 1.0) {
        return 1.0;
    }

    int kernelRadius = clamp(uShadowPcfKernelRadius, 0, 2);
    vec2 texelSize = cascadeIndex < 0 ? 1.0 / vec2(textureSize(uShadowMapTexture, 0))
                                      : CascadeTexelSizeUVE(cascadeIndex);
    float currentDepth = projected.z;
    float bias = max(0.0025 * (1.0 - max(dot(normal, lightDirection), 0.0)), 0.0005);
    float visibleSamples = 0.0;
    int sampleCount = 0;

    for (int offsetY = -2; offsetY <= 2; ++offsetY) {
        for (int offsetX = -2; offsetX <= 2; ++offsetX) {
            if (abs(offsetX) > kernelRadius || abs(offsetY) > kernelRadius) {
                continue;
            }
            vec2 sampleCoord = projected.xy + vec2(offsetX, offsetY) * texelSize;
            float sampledDepth = cascadeIndex < 0 ? texture(uShadowMapTexture, sampleCoord).r
                                                  : SampleCascadeDepthUVE(cascadeIndex, sampleCoord);
            visibleSamples += currentDepth - bias > sampledDepth ? 0.0 : 1.0;
            ++sampleCount;
        }
    }

    return visibleSamples / float(sampleCount);
}

float DirectionalShadowFactorUVE(vec3 normal, vec3 lightDirection) {
    if (uShadowCascadeCount <= 0) {
        return ShadowFactorFromPositionUVE(vLightSpacePosition, normal, lightDirection, -1);
    }

    float viewDepth = length(vWorldPosition - uViewPosition);
    int cascadeIndex = clamp(uShadowCascadeCount - 1, 0, 2);
    for (int candidateIndex = 0; candidateIndex < 2; ++candidateIndex) {
        if (candidateIndex < uShadowCascadeCount && viewDepth <= uShadowCascadeSplits[candidateIndex]) {
            cascadeIndex = candidateIndex;
            break;
        }
    }
    float shadowFactor = ShadowFactorFromPositionUVE(CascadeLightSpacePositionUVE(cascadeIndex), normal,
                                                      lightDirection, cascadeIndex);
    int finalCascadeIndex = clamp(uShadowCascadeCount - 1, 0, 2);
    if (cascadeIndex >= finalCascadeIndex) {
        return shadowFactor;
    }

    float cascadeNearDepth = cascadeIndex == 0 ? 0.0 : uShadowCascadeSplits[cascadeIndex - 1];
    float cascadeFarDepth = uShadowCascadeSplits[cascadeIndex];
    float cascadeDepthRange = max(cascadeFarDepth - cascadeNearDepth, 0.0001);
    float blendWidth = cascadeDepthRange * clamp(uShadowCascadeBlendRatio, 0.0, 0.25);
    float blendStartDepth = cascadeFarDepth - blendWidth;
    if (blendWidth <= 0.0 || viewDepth <= blendStartDepth) {
        return shadowFactor;
    }

    float nextCascadeShadowFactor = ShadowFactorFromPositionUVE(
        CascadeLightSpacePositionUVE(cascadeIndex + 1), normal, lightDirection, cascadeIndex + 1);
    float blendWeight = smoothstep(blendStartDepth, cascadeFarDepth, viewDepth);
    return mix(shadowFactor, nextCascadeShadowFactor, blendWeight);
}

void main() {
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb * uAlbedoColor;
    float ambientOcclusion = texture(uAOTexture, vTexCoord).r;
    vec3 normal = SafeNormalizeUVE(vWorldNormal);
    vec3 tangent = vWorldTangent - normal * dot(normal, vWorldTangent);
    if (dot(tangent, tangent) <= 0.00000001) {
        vec3 fallbackAxis = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        tangent = cross(fallbackAxis, normal);
    }
    tangent = SafeNormalizeUVE(tangent);
    vec3 bitangent = SafeNormalizeUVE(cross(normal, tangent));
    bitangent *= vTangentHandedness < 0.0 ? -1.0 : 1.0;
    vec3 tangentSpaceNormal = texture(uNormalTexture, vTexCoord).xyz * 2.0 - 1.0;
    normal = SafeNormalizeUVE(mat3(tangent, bitangent, normal) * tangentSpaceNormal);
    vec3 viewDirection = SafeNormalizeUVE(uViewPosition - vWorldPosition);
    float metallic = clamp(uMetallic, 0.0, 1.0);
    float roughness = clamp(uRoughness, 0.04, 1.0);
    vec3 lighting = albedo * uAmbientColor * ambientOcclusion + uEmissiveColor;

    for (int lightIndex = 0; lightIndex < 4; ++lightIndex) {
        LightUVE light = uLights[lightIndex];
        if (light.intensity <= 0.0) {
            continue;
        }

        vec3 lightDirection;
        float attenuation = 1.0;
        if (light.type == 0) {
            lightDirection = SafeNormalizeUVE(-light.direction);
        } else {
            vec3 toLight = light.position - vWorldPosition;
            float distanceToLight = max(length(toLight), 0.0001);
            lightDirection = toLight / distanceToLight;
            attenuation = 1.0 / max(distanceToLight * distanceToLight, 0.0001);
            if (light.range > 0.0 && distanceToLight > light.range) {
                attenuation = 0.0;
            }
            if (light.type == 2) {
                float coneAlignment = dot(-lightDirection, SafeNormalizeUVE(light.direction));
                if (coneAlignment < cos(radians(light.spotAngleDegrees))) {
                    attenuation = 0.0;
                }
            }
        }

        float normalDotLight = max(dot(normal, lightDirection), 0.0);
        float normalDotView = max(dot(normal, viewDirection), 0.0);
        if (normalDotLight <= 0.0 || normalDotView <= 0.0 || attenuation <= 0.0) {
            continue;
        }

        vec3 halfDirection = SafeNormalizeUVE(lightDirection + viewDirection);
        float normalDotHalf = max(dot(normal, halfDirection), 0.0);
        float halfDotView = max(dot(halfDirection, viewDirection), 0.0);
        vec3 baseReflectance = mix(vec3(0.04), albedo, metallic);
        vec3 fresnel = FresnelSchlickUVE(halfDotView, baseReflectance);
        float distribution = DistributionGgxUVE(normalDotHalf, roughness);
        float geometry = GeometrySmithUVE(normalDotView, normalDotLight, roughness);
        vec3 specular = (distribution * geometry * fresnel) /
                        max(4.0 * normalDotView * normalDotLight, kBrdfEpsilonUVE);
        vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
        vec3 diffuse = diffuseWeight * albedo / kPiUVE;
        vec3 radiance = light.color * light.intensity * attenuation;
        vec3 directContribution = (diffuse + specular) * radiance * normalDotLight;

        if (light.type == 0) {
            directContribution *= DirectionalShadowFactorUVE(normal, lightDirection);
        }
        lighting += directContribution;
    }

    FragColor = vec4(lighting, 1.0);
}
#endif
)GLSLSRC";



const std::string_view kEditorViewportEnvironmentSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uViewportMin;
uniform vec3 uViewportMax;
uniform vec3 uSurfaceSize;
uniform vec3 uGridOrigin;
uniform float uCameraTanHalfFov;
uniform float uViewportAspect;
uniform float uGridSpacing;
uniform int uProjectionMode;
uniform float uOrthographicScale;
uniform int uEnvironmentPreviewEnabled;
uniform int uSunPreviewEnabled;

float GridLineCoverageUVE(vec2 coordinate, float spacing) {
    vec2 scaled = coordinate / max(spacing, 0.000001);
    vec2 distanceToLine = abs(fract(scaled + 0.5) - 0.5);
    vec2 antiAlias = clamp(fwidth(scaled) * 0.35, vec2(0.004), vec2(0.055));
    vec2 line = 1.0 - smoothstep(vec2(0.0), antiAlias, distanceToLine);
    return max(line.x, line.y);
}

float SafeLog10UVE(float value) {
    return log(max(value, 0.000001)) / log(10.0);
}

void main() {
    vec2 surfaceUv = gl_FragCoord.xy / max(uSurfaceSize.xy, vec2(1.0));
    vec2 viewportExtent = max(uViewportMax.xy - uViewportMin.xy, vec2(0.001));
    vec2 viewportUv = clamp((surfaceUv - uViewportMin.xy) / viewportExtent, vec2(0.0), vec2(1.0));
    vec2 ndc = vec2(viewportUv.x * 2.0 - 1.0, viewportUv.y * 2.0 - 1.0);

    float tanHalfFov = max(uCameraTanHalfFov, 0.0001);
    float forwardLength = length(uCameraForward);
    vec3 forward = forwardLength > 0.0001 ? uCameraForward / forwardLength : vec3(0.0, 0.0, -1.0);
    float rightLength = length(uCameraRight);
    vec3 right = rightLength > 0.0001 ? uCameraRight / rightLength : vec3(1.0, 0.0, 0.0);
    float upLength = length(uCameraUp);
    vec3 up = upLength > 0.0001 ? uCameraUp / upLength : vec3(0.0, 1.0, 0.0);

    vec3 rayOrigin = uCameraPosition;
    vec3 rayDirection = forward;
    float orthographicScale = max(uOrthographicScale, 0.0001);
    if (uProjectionMode != 0) {
        rayOrigin += right * (ndc.x * uViewportAspect * orthographicScale) + up * (ndc.y * orthographicScale);
    } else {
        rayDirection = normalize(forward + right * (ndc.x * uViewportAspect * tanHalfFov) + up * (ndc.y * tanHalfFov));
    }

    const vec3 skyColor = vec3(0.246, 0.282, 0.322);       // #3F4852
    const vec3 groundColor = vec3(0.145, 0.165, 0.184);    // #252A2F
    const vec3 horizonColor = vec3(0.282, 0.322, 0.361);   // #48525C
    const vec3 gridColor = vec3(0.349, 0.388, 0.427);      // #59636D
    const vec3 axisXColor = vec3(1.0, 0.365, 0.365);       // Navigation Gizmo X #FF5D5D
    const vec3 axisYColor = vec3(0.290, 0.871, 0.502);      // Navigation Gizmo Y #4ADE80
    const vec3 axisZColor = vec3(0.231, 0.612, 1.0);       // Navigation Gizmo Z #3B9CFF

    float skyHeight = clamp(rayDirection.y, -1.0, 1.0);
    float horizonBlend = smoothstep(-0.16, 0.18, skyHeight);
    vec3 color = vec3(0.355, 0.365, 0.380); // neutral preview-off gray
    if (uEnvironmentPreviewEnabled != 0) {
        color = mix(groundColor, horizonColor, smoothstep(0.0, 0.35, skyHeight));
        color = mix(color, skyColor, smoothstep(0.18, 0.72, skyHeight));
    }
    if (uSunPreviewEnabled != 0) {
        const vec3 sunDirection = normalize(vec3(-0.38, 0.82, 0.30));
        float sunDisc = pow(max(dot(rayDirection, sunDirection), 0.0), 256.0);
        color += vec3(1.0, 0.83, 0.55) * sunDisc * 0.82;
    }

    float groundDenominator = rayDirection.y;
    if (groundDenominator < -0.0001) {
        float groundDistance = -rayOrigin.y / groundDenominator;
        if (groundDistance > 0.0 && groundDistance < 1000000.0) {
            vec3 groundHit = rayOrigin + rayDirection * groundDistance;
            vec2 relative = groundHit.xz - uGridOrigin.xz;

            float viewportPixelHeight = max(viewportExtent.y * uSurfaceSize.y, 1.0);
            float worldUnitsPerPixel = uProjectionMode != 0
                                           ? (2.0 * orthographicScale) / viewportPixelHeight
                                           : (groundDistance * 2.0 * tanHalfFov) / viewportPixelHeight;
            worldUnitsPerPixel = max(worldUnitsPerPixel, 0.000001);

            // Select the decade whose major cells remain comfortably readable. The 1/10 minor
            // level is about eight pixels at the crossover, and the logarithmic fraction blends
            // adjacent major levels continuously while zooming.
            float baseSpacing = max(uGridSpacing, 0.000001);
            float desiredMajorSpacing = max(worldUnitsPerPixel * 80.0, baseSpacing * 0.01);
            float decade = floor(SafeLog10UVE(desiredMajorSpacing / baseSpacing));
            float decadeFraction = fract(SafeLog10UVE(desiredMajorSpacing / baseSpacing));
            float majorSpacing = baseSpacing * pow(10.0, decade);
            float nextMajorSpacing = majorSpacing * 10.0;
            float majorTransition = smoothstep(0.18, 0.82, decadeFraction);

            float currentMinor = GridLineCoverageUVE(relative, majorSpacing * 0.1);
            float currentMajor = GridLineCoverageUVE(relative, majorSpacing);
            float nextMajor = GridLineCoverageUVE(relative, nextMajorSpacing);
            float minorWeight = (1.0 - majorTransition) * 0.42;
            float majorWeight = (1.0 - majorTransition) * 0.74 + majorTransition * 0.66;
            float gridFade = 1.0 - smoothstep(majorSpacing * 12.0, majorSpacing * 240.0, groundDistance);
            float gridCoverage = (currentMinor * minorWeight + currentMajor * majorWeight +
                                  nextMajor * majorTransition * 0.74) * gridFade;
            color = mix(color, gridColor, clamp(gridCoverage * 0.52, 0.0, 0.52));

            // X and Z are world-space ground axes and therefore must pass through the real origin.
            // The small width is antialiased in world units, so the axes stay stronger than the
            // grid without turning into a thick overlay.
            float axisWidth = clamp(max(worldUnitsPerPixel * 1.6, majorSpacing * 0.008),
                                    majorSpacing * 0.006, majorSpacing * 0.045);
            float axisX = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, abs(relative.y));
            float axisZ = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, abs(relative.x));
            float axisFade = gridFade * (0.62 + 0.38 * (1.0 - majorTransition));
            color = mix(color, axisXColor, clamp(axisX * axisFade * 0.72, 0.0, 0.78));
            color = mix(color, axisZColor, clamp(axisZ * axisFade * 0.72, 0.0, 0.78));

            // Project the world Y axis into the same camera ray. This is a line in XZ at the
            // origin, not a screen-space decoration, and naturally disappears when edge-on.
            vec2 horizontalRay = rayDirection.xz;
            float horizontalLengthSquared = dot(horizontalRay, horizontalRay);
            if (horizontalLengthSquared > 0.000001) {
                float axisParameter = -dot(rayOrigin.xz - uGridOrigin.xz, horizontalRay) /
                                      horizontalLengthSquared;
                if (axisParameter > 0.0) {
                    vec3 closestAxisPoint = rayOrigin + rayDirection * axisParameter;
                    float yAxisDistance = length(closestAxisPoint.xz - uGridOrigin.xz);
                    float yAxis = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, yAxisDistance);
                    color = mix(color, axisYColor, clamp(yAxis * axisFade * 0.72, 0.0, 0.78));
                }
            }
        }
    }

    // Keep the transition continuous at grazing angles instead of producing a hard horizon strip.
    color = mix(color, horizonColor, (1.0 - horizonBlend) * 0.08);
    FragColor = vec4(color, 1.0);
}
#endif
)GLSLSRC";

const std::string_view kParticleSource = R"GLSLSRC(#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

out vec4 vColor;

uniform mat4 uViewProjection;

void main() {
    vColor = aColor;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec4 vColor;

out vec4 FragColor;

void main() {
    FragColor = vColor;
}
#endif
)GLSLSRC";


const std::string_view kBloomBrightPassSource = R"GLSLSRC(#version 450 core

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

uniform sampler2D uSourceTexture;
uniform float uBloomThreshold;

void main() {
    vec3 hdrColor = max(texture(uSourceTexture, vTexCoord).rgb, vec3(0.0));
    float luminance = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
    float contribution = max(luminance - uBloomThreshold, 0.0) / max(luminance, 0.0001);
    FragColor = vec4(hdrColor * contribution, 1.0);
}
#endif
)GLSLSRC";

const std::string_view kBloomBlurSource = R"GLSLSRC(#version 450 core

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

uniform sampler2D uSourceTexture;
// (1,0) for a horizontal pass, (0,1) for a vertical pass - the same shader serves both halves of
// the separable Gaussian blur, one draw each, ping-ponging between two same-sized targets.
// Individual floats, not a vec2: ShaderProgramUVE currently only exposes Float/Int/Bool/Vec3/Mat4
// uniform setters (see shader_program_uve.h), so this avoids adding a new uniform-value type for
// a single consumer.
uniform float uBlurDirectionX;
uniform float uBlurDirectionY;
uniform float uTexelSizeX;
uniform float uTexelSizeY;

void main() {
    // 9-tap Gaussian, weights normalized to sum to 1 (sigma ~= 2 texels).
    const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec2 direction = vec2(uBlurDirectionX, uBlurDirectionY);
    vec2 texelSize = vec2(uTexelSizeX, uTexelSizeY);
    vec3 result = texture(uSourceTexture, vTexCoord).rgb * weights[0];
    for (int tap = 1; tap < 5; ++tap) {
        vec2 offset = direction * texelSize * float(tap);
        result += texture(uSourceTexture, vTexCoord + offset).rgb * weights[tap];
        result += texture(uSourceTexture, vTexCoord - offset).rgb * weights[tap];
    }
    FragColor = vec4(result, 1.0);
}
#endif
)GLSLSRC";

const std::string_view kFullscreenCopySource = R"GLSLSRC(#version 450 core

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

// Unmodified passthrough: the actual effect is the pipeline's blend mode this shader is used
// with, not anything computed here - Additive to composite the blurred bloom texture onto the
// HDR scene color, Multiply to composite the SSAO occlusion term onto it.
uniform sampler2D uSourceTexture;

void main() {
    FragColor = texture(uSourceTexture, vTexCoord);
}
#endif
)GLSLSRC";

const std::string_view kSsaoSource = R"GLSLSRC(#version 450 core

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
)GLSLSRC";

} // namespace UVE::Render::Shader::BuiltIn
