#version 330 core

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
uniform mat4 uViewProjection;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uLightSpaceMatrices[3];

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = mat3(uModel) * aNormal;
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

vec3 SafeNormalizeUVE(vec3 value) {
    return value / max(length(value), 0.0001);
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

        float diffuseStrength = max(dot(normal, lightDirection), 0.0);
        vec3 halfDirection = SafeNormalizeUVE(lightDirection + viewDirection);
        float shininess = mix(64.0, 4.0, roughness);
        float specularStrength = pow(max(dot(normal, halfDirection), 0.0), shininess);
        vec3 baseReflectance = mix(vec3(0.04), albedo, metallic);
        vec3 directContribution =
            (albedo * diffuseStrength * (1.0 - metallic) + baseReflectance * specularStrength) * light.color *
            light.intensity * attenuation;

        if (light.type == 0) {
            directContribution *= DirectionalShadowFactorUVE(normal, lightDirection);
        }
        lighting += directContribution;
    }

    FragColor = vec4(lighting, 1.0);
}
#endif
