// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


// Every constant below is a raw string literal transcribed byte-for-byte from its corresponding
// .glsl file under engine/render/shader/built_in/ - kept in sync by convention, enforced by
// tests/render/shader/built_in_shaders_parity_uve_tests.cpp (reads the real file and asserts
// exact equality against the constant here). Used automatically by ShaderManagerUVE as a fallback
// when the corresponding virtual path isn't reachable (see ShaderProgramDescUVE's doc comment).

#include "uve/render/shader/built_in_shaders_uve.h"

namespace UVE::Render::Shader::BuiltIn {

const std::string_view kBasic2DSource = R"GLSLSRC(#version 330 core

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

const std::string_view kBasic3DSource = R"GLSLSRC(#version 330 core

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

const std::string_view kBasic3DTexturedSource = R"GLSLSRC(#version 330 core

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

const std::string_view kFullscreenQuadSource = R"GLSLSRC(#version 330 core

#ifdef VERTEX_SHADER
// Fullscreen triangle via the vertex-ID trick - no vertex buffer needed. Placeholder only, not
// wired into any pipeline this increment (post-process is a future increment's work).
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

uniform sampler2D uSourceTexture;

void main() {
    FragColor = texture(uSourceTexture, gl_FragCoord.xy);
}
#endif
)GLSLSRC";

const std::string_view kShadowDepthSource = R"GLSLSRC(#version 330 core

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

const std::string_view kLitShadowed3DSource = R"GLSLSRC(#version 330 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec4 vLightSpacePosition;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat4 uLightSpaceMatrix;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = mat3(uModel) * aNormal;
    vTexCoord = aTexCoord;
    vLightSpacePosition = uLightSpaceMatrix * worldPosition;
    gl_Position = uViewProjection * worldPosition;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec4 vLightSpacePosition;
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
uniform sampler2D uShadowMapTexture;
uniform int uShadowPcfKernelRadius;

vec3 SafeNormalizeUVE(vec3 value) {
    return value / max(length(value), 0.0001);
}

float DirectionalShadowFactorUVE(vec3 normal, vec3 lightDirection) {
    if (abs(vLightSpacePosition.w) <= 0.0001) {
        return 1.0;
    }

    vec3 projected = vLightSpacePosition.xyz / vLightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0 ||
        projected.z <= 0.0 || projected.z >= 1.0) {
        return 1.0;
    }

    int kernelRadius = clamp(uShadowPcfKernelRadius, 0, 2);
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMapTexture, 0));
    float currentDepth = projected.z;
    float bias = max(0.0025 * (1.0 - max(dot(normal, lightDirection), 0.0)), 0.0005);
    float visibleSamples = 0.0;
    int sampleCount = 0;

    for (int offsetY = -2; offsetY <= 2; ++offsetY) {
        for (int offsetX = -2; offsetX <= 2; ++offsetX) {
            if (abs(offsetX) > kernelRadius || abs(offsetY) > kernelRadius) {
                continue;
            }
            float sampledDepth = texture(uShadowMapTexture, projected.xy + vec2(offsetX, offsetY) * texelSize).r;
            visibleSamples += currentDepth - bias > sampledDepth ? 0.0 : 1.0;
            ++sampleCount;
        }
    }

    return visibleSamples / float(sampleCount);
}

void main() {
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb * uAlbedoColor;
    float ambientOcclusion = texture(uAOTexture, vTexCoord).r;
    vec3 normal = SafeNormalizeUVE(vWorldNormal);
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
)GLSLSRC";

} // namespace UVE::Render::Shader::BuiltIn
