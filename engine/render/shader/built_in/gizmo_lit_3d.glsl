#version 450 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

out vec3 vWorldNormal;

uniform mat4 uModel;
uniform mat4 uViewProjection;

void main() {
    vWorldNormal = mat3(uModel) * aNormal;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vWorldNormal;
out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uLightDirection;

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDirection = normalize(-uLightDirection);
    float normalDotLight = max(dot(normal, lightDirection), 0.0);
    vec3 shaded = uColor * (0.45 + 0.55 * normalDotLight);
    FragColor = vec4(shaded, 1.0);
}
#endif
