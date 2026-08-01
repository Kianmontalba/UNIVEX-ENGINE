#version 330 core

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
