#version 450 core

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
