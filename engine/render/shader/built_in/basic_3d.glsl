#version 450 core

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
