#version 330 core

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

    float sampledDepth = texture(uShadowMapTexture, projected.xy).r;
    float currentDepth = projected.z;
    float bias = max(0.0025 * (1.0 - max(dot(normal, lightDirection), 0.0)), 0.0005);
    return currentDepth - bias > sampledDepth ? 0.0 : 1.0;
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
