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

uniform sampler2D uSourceTexture;
uniform float uBloomThreshold;

void main() {
    vec3 hdrColor = max(texture(uSourceTexture, vTexCoord).rgb, vec3(0.0));
    float luminance = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
    float contribution = max(luminance - uBloomThreshold, 0.0) / max(luminance, 0.0001);
    FragColor = vec4(hdrColor * contribution, 1.0);
}
#endif
